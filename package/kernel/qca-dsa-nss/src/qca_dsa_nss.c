// SPDX-License-Identifier: GPL-2.0-only
/*
 * qca-dsa-nss: NSS firmware VLAN interfaces for the user ports of a DSA
 * switch driven by a tag_8021q tagger (qca-8021q, rtl8365mb-8021q).
 *
 * With tag_8021q the switch talks to its CPU port in plain 802.1Q: a
 * standalone port carries its own VID (3072 + port), the ports of a
 * VLAN-unaware bridge share the bridge's VID (3088 + n) and the switch
 * forwards between them on its own. The NSS firmware parses 802.1Q
 * natively, so each of those VIDs can be a firmware VLAN interface hanging
 * off the conduit's phys_if - what qca-nss-vlan does for netifd's
 * eth0.<vid>, except that the netdev bound to the node is the DSA user
 * port itself. ECM treats a DSA user port as plain ethernet and only
 * needs an NSS interface number for it to write rules; this is where the
 * number comes from. RX and TX are untouched: exceptions come back on the
 * conduit with the tag on and go through the tagger, host TX goes
 * lan1 -> tagger -> conduit -> phys_if.
 *
 * A bridge's node is bound to the port the tagger's imprecise RX delivers
 * to (dsa_tag_8021q_find_user), so the host bridge's FDB and ECM's
 * interface hierarchy resolve to the one port that has the number. The set
 * of nodes is recomputed from the live topology after every relevant
 * netdev event; a node whose VID, port or phys_if no longer matches is
 * torn down and rebuilt.
 *
 * A VLAN-aware bridge is different: its frames cross the conduit with the
 * bridge's own VIDs (the CPU port is a tagged member of every bridge VLAN),
 * not with a tag_8021q one, and a port carries as many VIDs as it is a
 * member of. Such a bridge gets one node per (bridge VID, phys_if), bound to
 * the live representative selected by the tagger for its MAC and MTU. A
 * netdev cannot name the node then - one port, several VIDs - so ECM asks
 * for it by VID through qca_dsa_nss_if_num() instead of by netdev. Bridge
 * VLAN changes do not raise netdev events; the switchdev notifier covers them.
 *
 * The firmware hands out dynamic interface numbers next-fit and does not
 * reuse a freed slot until the cursor wraps, which takes a moment: after
 * ~118 allocations since boot an allocation can fail for a few seconds
 * (measured on a GL-B3000, 2026-09-11). A failed allocation is retried
 * with a delay instead of being given up on.
 */
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <linux/if_bridge.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/workqueue.h>
#include <linux/rtnetlink.h>
#include <linux/dsa/8021q.h>
#include <net/dsa.h>
#include <net/switchdev.h>
#include <nss_api_if.h>

/* exported by net/dsa/tag_8021q.c but declared only in its private header */
struct net_device *dsa_tag_8021q_find_user(struct net_device *conduit,
					   int source_port, int switch_id,
					   int vid, int vbid);

/* for ECM (qca-nss-ecm resolves it with symbol_get(), no link dependency) */
int qca_dsa_nss_if_num(struct net_device *conduit, u16 vid);

#define DSA_NSS_MAX_NODES	32
#define DSA_NSS_COALESCE_MS	100
#define DSA_NSS_RETRY_MS	2000
#define DSA_NSS_MAX_RETRIES	10

struct dsa_nss_node {
	u16 vid;
	int if_num;			/* NSS dynamic interface, -1 = none */
	int phys_if;			/* conduit's NSS physical interface */
	struct net_device *ndev;	/* bound DSA user port, held */
	u8 mac[ETH_ALEN];
	unsigned int mtu;
};

static struct dsa_nss_node dsa_nss_nodes[DSA_NSS_MAX_NODES];
static DEFINE_MUTEX(dsa_nss_lock);	/* nodes[] */
static DEFINE_SPINLOCK(dsa_nss_table_lock);	/* if_num publish, for lookups from softirq */
static struct delayed_work dsa_nss_work;
static unsigned int dsa_nss_retries;
static atomic_t dsa_nss_resyncs = ATOMIC_INIT(0);
static atomic_t dsa_nss_alloc_failures = ATOMIC_INIT(0);
static atomic_t dsa_nss_creates = ATOMIC_INIT(0);
static atomic_t dsa_nss_destroys = ATOMIC_INIT(0);
static struct dentry *dsa_nss_dentry;

/*
 * ===== firmware node create / destroy (dsa_nss_lock held) =====
 */

static void dsa_nss_node_destroy(struct dsa_nss_node *n, const char *why)
{
	int if_num;

	spin_lock_bh(&dsa_nss_table_lock);
	if_num = n->if_num;
	n->if_num = -1;
	spin_unlock_bh(&dsa_nss_table_lock);
	if (if_num < 0)
		return;

	pr_info("qca-dsa-nss: %s: drop vlan node if_num %d vid %u (%s)\n",
		netdev_name(n->ndev), if_num, n->vid, why);
	nss_unregister_vlan_if(if_num);
	if (nss_dynamic_interface_dealloc_node(if_num,
			NSS_DYNAMIC_INTERFACE_TYPE_VLAN) != NSS_TX_SUCCESS)
		pr_warn("qca-dsa-nss: %s: dealloc of if_num %d failed\n",
			netdev_name(n->ndev), if_num);
	dev_put(n->ndev);
	n->ndev = NULL;
	atomic_inc(&dsa_nss_destroys);
}

/* -EAGAIN: no interface number right now, worth a retry; -EIO: hard fail */
static int dsa_nss_node_create(struct dsa_nss_node *n, u16 vid, int phys_if,
			       struct net_device *ndev)
{
	int if_num;

	if_num = nss_dynamic_interface_alloc_node(NSS_DYNAMIC_INTERFACE_TYPE_VLAN);
	if (if_num < 0) {
		atomic_inc(&dsa_nss_alloc_failures);
		return -EAGAIN;
	}

	n->vid = vid;
	n->phys_if = phys_if;
	n->ndev = ndev;
	dev_hold(ndev);
	ether_addr_copy(n->mac, ndev->dev_addr);
	n->mtu = ndev->mtu;

	if (!nss_register_vlan_if(if_num, NULL, ndev, 0, n)) {
		pr_warn("qca-dsa-nss: %s: register of if_num %d failed\n",
			netdev_name(ndev), if_num);
		nss_dynamic_interface_dealloc_node(if_num,
					NSS_DYNAMIC_INTERFACE_TYPE_VLAN);
		dev_put(ndev);
		n->ndev = NULL;
		return -EIO;
	}
	if (nss_vlan_tx_set_mac_addr_msg(if_num, n->mac) != NSS_TX_SUCCESS ||
	    nss_vlan_tx_set_mtu_msg(if_num, n->mtu) != NSS_TX_SUCCESS ||
	    nss_vlan_tx_add_tag_msg(if_num, (ETH_P_8021Q << 16) | vid,
				    phys_if, phys_if) != NSS_TX_SUCCESS) {
		pr_warn("qca-dsa-nss: %s: firmware refused vlan node if_num %d vid %u on phys_if %d\n",
			netdev_name(ndev), if_num, vid, phys_if);
		nss_unregister_vlan_if(if_num);
		nss_dynamic_interface_dealloc_node(if_num,
					NSS_DYNAMIC_INTERFACE_TYPE_VLAN);
		dev_put(ndev);
		n->ndev = NULL;
		return -EIO;
	}

	spin_lock_bh(&dsa_nss_table_lock);
	n->if_num = if_num;
	spin_unlock_bh(&dsa_nss_table_lock);
	pr_info("qca-dsa-nss: %s: vlan node if_num %d vid %u on phys_if %d\n",
		netdev_name(ndev), if_num, vid, phys_if);
	atomic_inc(&dsa_nss_creates);
	return 0;
}

/*
 * ===== desired topology (rtnl held) =====
 */

struct dsa_nss_want {
	u16 vid;
	int phys_if;
	struct net_device *ndev;
};

/* A port whose switch runs a tag_8021q tagger (qca-8021q on qca8k,
 * rtl8365mb-8021q on rtl8365mb): the switch talks to the conduit in plain
 * 802.1Q, and the VIDs are the framework's regardless of the switch.
 */
static bool dsa_nss_port_is_8021q(const struct dsa_port *dp)
{
	return dp->cpu_dp && dp->ds && dp->ds->tag_8021q_ctx &&
	       dp->cpu_dp->tag_ops &&
	       dp->cpu_dp->tag_ops->proto == DSA_TAG_PROTO_YT921X_8021Q;
}

/* The tag_8021q DSA port behind @dev: @dev itself, or the real device of an
 * 802.1Q upper of one (wan.35). NULL for anything else.
 */
static struct dsa_port *dsa_nss_port_of(struct net_device *dev, bool *upper)
{
	struct dsa_port *dp;

	*upper = false;
	if (is_vlan_dev(dev)) {
		dev = vlan_dev_real_dev(dev);
		*upper = true;
	}
	if (!dsa_user_dev_check(dev))
		return NULL;
	dp = dsa_port_from_netdev(dev);
	if (IS_ERR_OR_NULL(dp) || !dsa_nss_port_is_8021q(dp))
		return NULL;
	return dp;
}

/* One entry per (vid, phys_if): a second port in the same VLAN keeps the
 * first port's entry. False when the table is full.
 */
static bool dsa_nss_want_add(struct dsa_nss_want *want, int *n, int max,
			     u16 vid, int phys_if, struct net_device *ndev)
{
	int i;

	for (i = 0; i < *n; i++)
		if (want[i].vid == vid && want[i].phys_if == phys_if)
			return true;	/* the bridge already has its entry */
	if (*n == max) {
		pr_warn_once("qca-dsa-nss: more than %d nodes wanted, rest ignored\n", max);
		return false;
	}
	want[*n].vid = vid;
	want[*n].phys_if = phys_if;
	want[*n].ndev = ndev;
	(*n)++;
	return true;
}

/* Match tag_yt921x_8021q's VLAN-aware, imprecise RX port selection. */
static struct net_device *dsa_nss_bridge_vlan_port(struct net_device *conduit,
						  u16 vid)
{
	struct dsa_port *cpu_dp = conduit->dsa_ptr;
	struct bridge_vlan_info vinfo;
	struct dsa_port *dp;

	dsa_tree_for_each_user_port(dp, cpu_dp->dst) {
		if (dp->cpu_dp != cpu_dp || !dp->user || !dp->bridge ||
		    !dsa_port_is_vlan_filtering(dp))
			continue;
		if (dp->stp_state != BR_STATE_LEARNING &&
		    dp->stp_state != BR_STATE_FORWARDING)
			continue;
		if (!br_vlan_get_info(dp->user, vid, &vinfo))
			return dp->user;
	}
	return NULL;
}

/*
 * One entry per standalone tag_8021q port, one per VLAN-unaware bridge on
 * such ports and one per VLAN of a VLAN-aware bridge on such ports.
 * Direct port VLAN uppers must not claim a bridge's conduit VID.
 * Returns the count.
 */
static int dsa_nss_collect(struct dsa_nss_want *want, int max)
{
	struct net_device *dev;
	int n = 0;

	for_each_netdev(&init_net, dev) {
		struct net_device *conduit, *bound;
		struct dsa_port *dp;
		int phys_if;
		bool upper;
		u16 vid;

		dp = dsa_nss_port_of(dev, &upper);
		if (!dp)
			continue;

		conduit = dp->cpu_dp->conduit;
		phys_if = nss_cmn_get_interface_number_by_dev(conduit);
		if (phys_if < 0 || phys_if >= NSS_MAX_PHYSICAL_INTERFACES)
			continue;	/* conduit not armed as a firmware port */

		/* Direct port VLAN uppers are unsupported by the YT tagger. */
		if (upper)
			continue;
		if (dp->bridge &&
			   br_vlan_enabled(dsa_port_bridge_dev_get(dp))) {
			/* Use the same live representative as imprecise RX, not
			 * netdev enumeration order. Without one, keep the VLAN
			 * on the host until a port can receive it.
			 */
			struct bridge_vlan_info vinfo;

			for (vid = 1; vid < VLAN_N_VID && !vid_is_dsa_8021q(vid); vid++) {
				if (br_vlan_get_info(dev, vid, &vinfo))
					continue;
				bound = dsa_nss_bridge_vlan_port(conduit, vid);
				if (!bound)
					continue;
				if (!dsa_nss_want_add(want, &n, max, vid, phys_if, bound))
					return n;
			}
			continue;
		} else if (dp->bridge && dp->bridge->tx_fwd_offload) {
			unsigned int vbid = dsa_port_bridge_num_get(dp);

			vid = dsa_tag_8021q_bridge_vid(vbid);
			/* the port imprecise RX delivers to, if any is live */
			bound = dsa_tag_8021q_find_user(conduit, -1, -1, vid, vbid);
			if (!bound)
				bound = dev;
		} else {
			vid = dsa_tag_8021q_standalone_vid(dp);
			bound = dev;
		}

		if (!dsa_nss_want_add(want, &n, max, vid, phys_if, bound))
			break;
	}
	return n;
}

/*
 * qca_dsa_nss_if_num()
 *	The node of @vid on the phys_if of @conduit, -1 when there is none.
 *
 * For ECM: a port of a VLAN-aware bridge has one node per VID it is in, so
 * a netdev cannot name the node - the flow's VID can. Callable from softirq.
 */
int qca_dsa_nss_if_num(struct net_device *conduit, u16 vid)
{
	int phys_if, i, if_num = -1;

	phys_if = nss_cmn_get_interface_number_by_dev(conduit);
	if (phys_if < 0)
		return -1;

	spin_lock_bh(&dsa_nss_table_lock);
	for (i = 0; i < DSA_NSS_MAX_NODES; i++) {
		const struct dsa_nss_node *n = &dsa_nss_nodes[i];

		if (n->if_num >= 0 && n->vid == vid && n->phys_if == phys_if) {
			if_num = n->if_num;
			break;
		}
	}
	spin_unlock_bh(&dsa_nss_table_lock);
	return if_num;
}
EXPORT_SYMBOL_GPL(qca_dsa_nss_if_num);

static bool dsa_nss_node_matches(const struct dsa_nss_node *n,
				 const struct dsa_nss_want *w)
{
	return n->if_num >= 0 && n->vid == w->vid && n->phys_if == w->phys_if &&
	       n->ndev == w->ndev && n->mtu == w->ndev->mtu &&
	       ether_addr_equal(n->mac, w->ndev->dev_addr);
}

/* returns true when something could not be allocated and a retry is due */
static bool dsa_nss_resync(void)
{
	struct dsa_nss_want want[DSA_NSS_MAX_NODES];
	bool again = false;
	int nwant, i, j;

	atomic_inc(&dsa_nss_resyncs);
	rtnl_lock();
	mutex_lock(&dsa_nss_lock);

	nwant = dsa_nss_collect(want, DSA_NSS_MAX_NODES);

	/* tear down what no longer matches */
	for (i = 0; i < DSA_NSS_MAX_NODES; i++) {
		struct dsa_nss_node *n = &dsa_nss_nodes[i];

		if (n->if_num < 0)
			continue;
		for (j = 0; j < nwant; j++)
			if (dsa_nss_node_matches(n, &want[j]))
				break;
		if (j == nwant)
			dsa_nss_node_destroy(n, "topology changed");
	}

	/* build what is missing */
	for (j = 0; j < nwant; j++) {
		struct dsa_nss_node *free = NULL;
		int ret;

		for (i = 0; i < DSA_NSS_MAX_NODES; i++) {
			struct dsa_nss_node *n = &dsa_nss_nodes[i];

			if (n->if_num >= 0) {
				if (dsa_nss_node_matches(n, &want[j]))
					break;
				continue;
			}
			if (!free)
				free = n;
		}
		if (i < DSA_NSS_MAX_NODES)
			continue;	/* already there */
		if (!free)
			break;
		ret = dsa_nss_node_create(free, want[j].vid, want[j].phys_if,
					  want[j].ndev);
		if (ret == -EAGAIN)
			again = true;
	}

	mutex_unlock(&dsa_nss_lock);
	rtnl_unlock();
	return again;
}

static void dsa_nss_work_fn(struct work_struct *work)
{
	if (dsa_nss_resync()) {
		if (dsa_nss_retries++ < DSA_NSS_MAX_RETRIES) {
			pr_info("qca-dsa-nss: no free firmware interface number, retry %u in %d ms\n",
				dsa_nss_retries, DSA_NSS_RETRY_MS);
			schedule_delayed_work(&dsa_nss_work,
					      msecs_to_jiffies(DSA_NSS_RETRY_MS));
		} else {
			pr_warn("qca-dsa-nss: giving up after %u retries; write 1 to debugfs 'resync' to try again\n",
				dsa_nss_retries);
		}
		return;
	}
	dsa_nss_retries = 0;
}

static void dsa_nss_schedule(void)
{
	dsa_nss_retries = 0;
	mod_delayed_work(system_wq, &dsa_nss_work,
			 msecs_to_jiffies(DSA_NSS_COALESCE_MS));
}

/*
 * ===== netdev events =====
 */

static int dsa_nss_netdev_event(struct notifier_block *nb, unsigned long event,
				void *ptr)
{
	struct net_device *dev = netdev_notifier_info_to_dev(ptr);
	bool upper;
	int i;

	if (!dsa_nss_port_of(dev, &upper))
		return NOTIFY_DONE;

	switch (event) {
	case NETDEV_UNREGISTER:
		/*
		 * The port is going away now; release its node here, with
		 * rtnl held, rather than from the work item, so the netdev
		 * is not kept alive by our reference.
		 */
		mutex_lock(&dsa_nss_lock);
		for (i = 0; i < DSA_NSS_MAX_NODES; i++)
			if (dsa_nss_nodes[i].if_num >= 0 &&
			    dsa_nss_nodes[i].ndev == dev)
				dsa_nss_node_destroy(&dsa_nss_nodes[i],
						     "port unregistered");
		mutex_unlock(&dsa_nss_lock);
		fallthrough;
	case NETDEV_REGISTER:
	case NETDEV_UP:
	case NETDEV_DOWN:
	case NETDEV_CHANGE:
	case NETDEV_CHANGEUPPER:
	case NETDEV_CHANGEADDR:
	case NETDEV_CHANGEMTU:
		dsa_nss_schedule();
		break;
	default:
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block dsa_nss_netdev_nb = {
	.notifier_call = dsa_nss_netdev_event,
};

/*
 * ===== switchdev events =====
 *
 * A bridge VLAN added to or removed from a port, and vlan_filtering
 * toggled on a bridge, reach the switch driver through switchdev and raise
 * no netdev event; both change the nodes a VLAN-aware bridge wants.
 */
static int dsa_nss_switchdev_event(struct notifier_block *nb,
				   unsigned long event, void *ptr)
{
	struct net_device *dev = switchdev_notifier_info_to_dev(ptr);
	bool upper;

	switch (event) {
	case SWITCHDEV_PORT_OBJ_ADD:
	case SWITCHDEV_PORT_OBJ_DEL: {
		const struct switchdev_notifier_port_obj_info *info = ptr;

		if (info->obj->id != SWITCHDEV_OBJ_ID_PORT_VLAN)
			return NOTIFY_DONE;
		break;
	}
	case SWITCHDEV_PORT_ATTR_SET: {
		const struct switchdev_notifier_port_attr_info *info = ptr;

		if (info->attr->id != SWITCHDEV_ATTR_ID_BRIDGE_VLAN_FILTERING &&
		    info->attr->id != SWITCHDEV_ATTR_ID_PORT_STP_STATE)
			return NOTIFY_DONE;
		break;
	}
	default:
		return NOTIFY_DONE;
	}

	/* the port itself, or a bridge (its own VLANs, vlan_filtering) */
	if (dsa_nss_port_of(dev, &upper) || netif_is_bridge_master(dev))
		dsa_nss_schedule();
	return NOTIFY_DONE;
}

static struct notifier_block dsa_nss_switchdev_nb = {
	.notifier_call = dsa_nss_switchdev_event,
};

/*
 * ===== debugfs =====
 */

static int dsa_nss_status_show(struct seq_file *m, void *v)
{
	int i, nodes = 0;

	mutex_lock(&dsa_nss_lock);
	for (i = 0; i < DSA_NSS_MAX_NODES; i++) {
		struct dsa_nss_node *n = &dsa_nss_nodes[i];

		if (n->if_num < 0)
			continue;
		seq_printf(m, "%-16s if_num=%d vid=%u phys_if=%d mtu=%u mac=%pM\n",
			   netdev_name(n->ndev), n->if_num, n->vid, n->phys_if,
			   n->mtu, n->mac);
		nodes++;
	}
	mutex_unlock(&dsa_nss_lock);
	seq_printf(m, "nodes=%d resyncs=%d creates=%d destroys=%d alloc_failures=%d retries_pending=%u\n",
		   nodes, atomic_read(&dsa_nss_resyncs),
		   atomic_read(&dsa_nss_creates), atomic_read(&dsa_nss_destroys),
		   atomic_read(&dsa_nss_alloc_failures), dsa_nss_retries);
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(dsa_nss_status);

static ssize_t dsa_nss_resync_write(struct file *fp, const char __user *ubuf,
				    size_t sz, loff_t *ppos)
{
	dsa_nss_schedule();
	return sz;
}

static const struct file_operations dsa_nss_resync_fops = {
	.owner = THIS_MODULE,
	.write = dsa_nss_resync_write,
	.llseek = noop_llseek,
};

/*
 * ===== module =====
 */

static int __init qca_dsa_nss_init(void)
{
	int i, ret;

	for (i = 0; i < DSA_NSS_MAX_NODES; i++)
		dsa_nss_nodes[i].if_num = -1;
	INIT_DELAYED_WORK(&dsa_nss_work, dsa_nss_work_fn);

	ret = register_netdevice_notifier(&dsa_nss_netdev_nb);
	if (ret)
		return ret;
	ret = register_switchdev_blocking_notifier(&dsa_nss_switchdev_nb);
	if (ret) {
		unregister_netdevice_notifier(&dsa_nss_netdev_nb);
		cancel_delayed_work_sync(&dsa_nss_work);
		mutex_lock(&dsa_nss_lock);
		for (i = 0; i < DSA_NSS_MAX_NODES; i++)
			dsa_nss_node_destroy(&dsa_nss_nodes[i], "init failed");
		mutex_unlock(&dsa_nss_lock);
		return ret;
	}

	dsa_nss_dentry = debugfs_create_dir("qca-dsa-nss", NULL);
	debugfs_create_file("status", 0444, dsa_nss_dentry, NULL,
			    &dsa_nss_status_fops);
	debugfs_create_file("resync", 0200, dsa_nss_dentry, NULL,
			    &dsa_nss_resync_fops);

	pr_info("qca-dsa-nss: firmware vlan nodes for tag_8021q DSA ports\n");
	dsa_nss_schedule();
	return 0;
}

static void __exit qca_dsa_nss_exit(void)
{
	int i;

	unregister_switchdev_blocking_notifier(&dsa_nss_switchdev_nb);
	unregister_netdevice_notifier(&dsa_nss_netdev_nb);
	cancel_delayed_work_sync(&dsa_nss_work);
	debugfs_remove_recursive(dsa_nss_dentry);

	mutex_lock(&dsa_nss_lock);
	for (i = 0; i < DSA_NSS_MAX_NODES; i++)
		dsa_nss_node_destroy(&dsa_nss_nodes[i], "module unload");
	mutex_unlock(&dsa_nss_lock);
}

module_init(qca_dsa_nss_init);
module_exit(qca_dsa_nss_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("NSS firmware VLAN interfaces for tag_8021q DSA user ports");
