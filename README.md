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

soon...

## Installation

soon...

## License

OpenWrt License: [GPL-2.0](https://openwrt.org/license)

This repository:  GPL-2.0
