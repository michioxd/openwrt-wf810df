include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=qca-dsa-nss
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-only

include $(INCLUDE_DIR)/package.mk

define KernelPackage/qca-dsa-nss
  SECTION:=kernel
  CATEGORY:=Kernel modules
  SUBMENU:=Network Devices
  TITLE:=NSS firmware VLAN interfaces for tag_8021q DSA ports
  DEPENDS:=@TARGET_qualcommax_ipq50xx +kmod-qca-nss-drv +@NSS_DRV_VLAN_ENABLE
  FILES:=$(PKG_BUILD_DIR)/qca-dsa-nss.ko
endef

define KernelPackage/qca-dsa-nss/description
  Gives the user ports of a DSA switch driven by a tag_8021q tagger (qca-8021q, rtl8365mb-8021q) an
  NSS interface number: one firmware VLAN interface (dynamic interface type
  VLAN on the conduit's phys_if) per standalone port and per VLAN-unaware
  bridge, carrying the VID the tagger uses, and one per VLAN of a VLAN-aware
  bridge, carrying that bridge VID (ECM asks for those by VID through
  qca_dsa_nss_if_num). ECM treats a DSA port as plain ethernet and only
  needs that number to accelerate flows through it, so the switch driver
  stays bound and lan/wan keep being real DSA ports.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	+$(MAKE) -C "$(LINUX_DIR)" \
	  CROSS_COMPILE="$(TARGET_CROSS)" \
	  ARCH="$(LINUX_KARCH)" \
	  M="$(PKG_BUILD_DIR)" \
	  NOSTDINC_FLAGS="$(KERNEL_NOSTDINC_FLAGS) -I$(STAGING_DIR)/usr/include/qca-nss-drv" \
	  $(KERNEL_MAKE_FLAGS) \
	  modules
endef

$(eval $(call KernelPackage,qca-dsa-nss))
