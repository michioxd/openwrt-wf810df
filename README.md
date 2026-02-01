![OpenWrt logo](include/logo.png)

Unofficial [**OpenWrt**](https://openwrt.org/) for **FPT AX3000CV2 (also known as Actiontec WF-810DF)**.  
See the official OpenWrt README for general build instructions and project information: [click here](https://github.com/openwrt/openwrt/blob/master/README.md)

This build mainly targets the upcoming [**OpenWrt 25.12 mainstream release**](https://github.com/openwrt/openwrt/tree/openwrt-25.12). We keep all changes as close to upstream OpenWrt as possible.

Additional work may include device bring-up, hardware validation, driver fixes, calibration data integration, LED and GPIO mapping, and performance tuning specific to the AX3000CV2 platform.

## Specifications

- **SoC:** Qualcomm IPQ5018
- **RAM:** [512 MB NYANA `NT5CC256M16ER-EK`](https://www.nanya.com/en/product/4248/NT5CC256M16ER-EK)
- **NAND:** [256 MB GigaDevice `GD5F2GM7REYIG`](https://www.gigadevice.com/product/flash/spi-nand-flash/gd5f2gm7re)
- **Wi-Fi:**
  - Internal (2.4GHz)
  - Qualcomm QCN6102 (5GHz AX)
- **Ethernet:**
  - [3x ports 1GbE Switch Motorcomm YT9215S](https://en.motor-comm.com/download?kw=&category=135&wd=1&tp=1)
  - 1x port 1GbE WAN Internal

## Hardware working status

- [x] Internal IPQ5018 WAN
- [ ] 3x 1GbE Switch Motorcomm YT9215S
- [x] 2.4GHz Wi-Fi (SoC)
- [x] 5GHz Wi-Fi (QCN6102)
- [x] NAND Flash
- [ ] LEDs (Red/White) *(unknown state)*
- [x] Buttons (Reset/WPS (aka Mesh button))

### Known issues

- YT9215S just return `0xffffffff` for somehow (is that MDIO `90000` issue or what).
- LED still not working

## External resources

These resources were dumped from the original firmware:

- WLAN Firmware `WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4 v1`
  - [IPQ5081](https://github.com/michioxd/upstream-wifi-fw/tree/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4)
  - [QCN6122  (usable on QCN6102)](https://github.com/michioxd/upstream-wifi-fw/tree/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4/qcn6122)
- WLAN Calibration data:
  - [WF-810DF 2.4GHz](https://github.com/michioxd/upstream-wifi-fw/blob/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4/cal-ahb-c000000.wifi.bin)
  - [WF-810DF 5GHz](https://github.com/michioxd/upstream-wifi-fw/blob/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4/qcn6122/cal-ahb-b00a040.wifi.bin)
- WLAN Board data:
  - [WF-810DF 2.4GHz](https://github.com/michioxd/upstream-wifi-fw/blob/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4/board-2.bin)
  - [WF-810DF 5GHz](https://github.com/michioxd/upstream-wifi-fw/blob/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4/qcn6122/board-2.bin)

## Download

Check out the latest version in the [Releases](https://github.com/michioxd/openwrt-wf810df/releases/latest) section.

## Installation

> [!CAUTION]
> **DISCLAIMER:**
> Please use this firmware responsibly and at your own risk.
> I take no responsibility or liability for any damage, data loss, device malfunction, or other issues that may occur as a result of using it.
> This includes, but is not limited to, bricked devices, system instability, or permanent hardware/software damage.
> By using this, you acknowledge that you understand the risks involved and agree that you are solely responsible for any consequences.

### Prerequisites

- FPT AX3000CV2 running stock QSDK firmware.
- Access to the U-Boot command line (via UART).
  <details>
  <summary>Check out my setup</summary>
  ESP32 used as a UART bridge

  ![img](https://github.com/user-attachments/assets/3e4140af-9982-4330-b0f6-5b8d51853c84)
  </details>
- A computer with TFTP software (Tftpd or similar), and you must know your PC/Gateway IP address.

### 0. Back up the original firmware (dump the entire NAND)

> [!IMPORTANT]
> **Save your stock firmware!**
>
> Do not skip this step.  
> You may think it is unnecessary since you do not plan to return to the “dark past”, but I assure you that the stock firmware is still useful for restoring missing settings, drivers, and for further research.

1. Power on the device and repeatedly press the **Escape (ESC)** key to enter the U-Boot command-line interface.
2. Set the environment variables so U-Boot can connect to the local network and locate your TFTP server:
   ```sh
   # Adjust these values to match your setup
   setenv ipaddr 192.168.2.240
   setenv gatewayip 192.168.2.1
   setenv serverip 192.168.2.104
   # Save to NAND for later use or debugging
   saveenv
   ```
3. For safety, the dump will be split into 4 parts. Each part is 64 MB, since the NAND size is 256 MB.
   ```sh
   mw.b 0x44000000 0xff 0x4000000
   nand read 0x44000000 0x0 0x4000000
   tftpput 0x44000000 0x4000000 part1.bin

   mw.b 0x44000000 0xff 0x4000000
   nand read 0x44000000 0x4000000 0x4000000
   tftpput 0x44000000 0x4000000 part2.bin

   mw.b 0x44000000 0xff 0x4000000
   nand read 0x44000000 0x8000000 0x4000000
   tftpput 0x44000000 0x4000000 part3.bin

   mw.b 0x44000000 0xff 0x4000000
   nand read 0x44000000 0xC000000 0x4000000
   tftpput 0x44000000 0x4000000 part4.bin
   ```
4. If you wanna restore the original firmware:
   ```sh
   tftpboot 0x44000000 part1.bin
   nand erase 0x0 0x4000000
   nand write 0x44000000 0x0 0x4000000

   tftpboot 0x44000000 part2.bin
   nand erase 0x4000000 0x4000000
   nand write 0x44000000 0x4000000 0x4000000

   tftpboot 0x44000000 part3.bin
   nand erase 0x8000000 0x4000000
   nand write 0x44000000 0x8000000 0x4000000

   tftpboot 0x44000000 part4.bin
   nand erase 0xC000000 0x4000000
   nand write 0x44000000 0xC000000 0x4000000
   ```

## License

OpenWrt License: [GPL-2.0](https://openwrt.org/license)

This repository:  GPL-2.0
