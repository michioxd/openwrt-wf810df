![OpenWrt logo](include/logo.png)

Unofficial [**OpenWrt**](https://openwrt.org/) for **FPT AX3000CV2 (also known as Actiontec WF-810DF)**.  
See the official OpenWrt README for general build instructions and project information: [click here](https://github.com/openwrt/openwrt/blob/master/README.md)

This build mainly targets the upcoming [**OpenWrt 25.12 mainstream release**](https://github.com/openwrt/openwrt/tree/openwrt-25.12). We keep all changes as close to upstream OpenWrt as possible.

Additional work may include device bring-up, hardware validation, driver fixes, calibration data integration, LED and GPIO mapping, and performance tuning specific to the AX3000CV2 platform.

## Specifications

- **SoC:** Qualcomm IPQ5018
- **RAM:** 512 MB NYANA `NT5CC256M16ER-EK`
- **NAND:** 256 MB GigaDevice `GD5F2GM7REYIG`
- **Wi-Fi:**
  - Internal (2.4GHz)
  - Qualcomm QCN6102 (5GHz AX)
- Ethernet:
  - 3x ports 1GbE Switch Motorcomm YT9215S
  - 1x port 1GbE WAN Internal

## Hardware working status

- [x] Internal IPQ5018 WAN
- [ ] 3x 1GbE Switch Motorcomm YT9215S
- [x] 2.4GHz WiFi (Internal IPQ5018)
- [x] 5GHz WiFi (QCA6102)
- [x] NAND Flash
- [ ] LEDs (Red/White) *(unknown state)*
- [x] Buttons (Reset/WPS (aka Mesh button))

## Software working status

soon...

## Download

soon

## License

OpenWrt License: [GPL-2.0](https://openwrt.org/license)

This repository:  GPL-2.0
