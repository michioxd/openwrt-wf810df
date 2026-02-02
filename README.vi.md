[English](README.md) | **Tiếng Việt**

![Logo OpenWrt](include/logo.png)

Bản [**OpenWrt**](https://openwrt.org/) không chính thức cho **FPT AX3000CV2 (còn được gọi là Actiontec/CIG WF-810DF)**.  
Xem hướng dẫn build chung và thông tin dự án tại README chính thức của OpenWrt: [tại đây](https://github.com/openwrt/openwrt/blob/master/README.md)

Bản build này chủ yếu nhắm đến phiên bản [**OpenWrt 25.12 mainstream release**](https://github.com/openwrt/openwrt/tree/openwrt-25.12) sắp tới. Chúng tôi cố gắng giữ mọi thay đổi sát với OpenWrt gốc nhất có thể.

Trong này có thể bao gồm những thành phần như xác thực phần cứng, sửa lỗi driver, tích hợp dữ liệu hiệu chuẩn, gán LED và GPIO, cũng như tinh chỉnh hiệu năng dành riêng cho nền tảng AX3000CV2.

> [!CAUTION]
> **CHỈ DÀNH CHO MỤC ĐÍCH GIÁO DỤC VÀ NGHIÊN CỨU**
>
> Firmware này được cung cấp **“NGUYÊN TRẠNG”** (AS IS), hoàn toàn phục vụ cho mục đích **giáo dục và nghiên cứu**.  
> **Bạn hoàn toàn chịu mọi rủi ro nếu sử dụng**

## Thông số kỹ thuật

- **SoC:** Qualcomm IPQ5018
- **RAM:** [512 MB NYANA `NT5CC256M16ER-EK`](https://www.nanya.com/en/product/4248/NT5CC256M16ER-EK)
- **NAND:** [256 MB GigaDevice `GD5F2GM7REYIG`](https://www.gigadevice.com/product/flash/spi-nand-flash/gd5f2gm7re)
- **Wi-Fi:**
  - Nội bộ (2.4GHz)
  - Qualcomm QCN6102 (5GHz AX)
- **Ethernet:**
  - [3x cổng 1GbE Switch Motorcomm YT9215S](https://en.motor-comm.com/download?kw=&category=135&wd=1&tp=1)
  - 1x cổng 1GbE WAN Nội bộ

## Trạng thái hoạt động phần cứng

- [x] WAN Nội bộ IPQ5018
- [ ] 3x cổng 1GbE Switch Motorcomm YT9215S
- [x] Wi-Fi 2.4GHz (SoC)
- [x] Wi-Fi 5GHz (QCN6102)
- [x] NAND Flash
- [ ] Đèn LED (Đỏ/Trắng) *(trạng thái không xác định)*
- [x] Các nút bấm (Reset/WPS (hay còn gọi là nút Mesh))

### Các vấn đề đã biết

- YT9215S chỉ trả về `0xffffffff` vì lý do nào đó (liệu có phải lỗi MDIO `90000` hay gì không).
- Đèn LED vẫn chưa hoạt động

## Tài nguyên bên ngoài

Các tài nguyên này được dump từ firmware gốc:

- Firmware WLAN `WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4 v1`
  - [IPQ5081](https://github.com/michioxd/upstream-wifi-fw/tree/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4)
  - [QCN6122 (dùng được cho QCN6102)](https://github.com/michioxd/upstream-wifi-fw/tree/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4/qcn6122)
- Dữ liệu hiệu chuẩn WLAN (Calibration data):
  - [WF-810DF 2.4GHz](https://github.com/michioxd/upstream-wifi-fw/blob/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4/cal-ahb-c000000.wifi.bin)
  - [WF-810DF 5GHz](https://github.com/michioxd/upstream-wifi-fw/blob/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4/qcn6122/cal-ahb-b00a040.wifi.bin)
- Dữ liệu Board WLAN:
  - [WF-810DF 2.4GHz](https://github.com/michioxd/upstream-wifi-fw/blob/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4/board-2.bin)
  - [WF-810DF 5GHz](https://github.com/michioxd/upstream-wifi-fw/blob/main/ath11k-firmware/IPQ5018_QCN6122_QCN6122/hw1.0/2.8/WLAN.HK.2.8-01357-QCAHKSWPL_SILICONZ-1.56794.4/qcn6122/board-2.bin)

## Tải xuống

Kiểm tra phiên bản mới nhất trong phần [Releases](https://github.com/michioxd/openwrt-wf810df/releases/latest).

## Cài đặt

> [!CAUTION]
> **TUYÊN BỐ MIỄN TRỪ TRÁCH NHIỆM QUAN TRỌNG – ĐỌC KỸ**
>
> Firmware này được cung cấp **NGUYÊN TRẠNG** và chỉ dành cho **người dùng nâng cao**.  
> **BẠN HOÀN TOÀN CHỊU MỌI RỦI RO KHI SỬ DỤNG NÓ.**
>
> Tôi **từ chối mọi trách nhiệm và nghĩa vụ pháp lý** đối với bất kỳ thiệt hại, mất mát dữ liệu, hỏng hóc thiết bị, hoặc các vấn đề khác có thể phát sinh từ việc sử dụng nó.
> Điều này bao gồm, nhưng **không giới hạn ở**, **thiết bị bị brick vĩnh viễn**, hệ thống không ổn định, mất chức năng, hoặc hư hỏng phần cứng/phần mềm không thể phục hồi.
>
> Bằng việc tiếp tục, bạn **xác nhận rằng bạn hiểu đầy đủ các rủi ro liên quan**, chấp nhận rằng việc flash firmware vốn dĩ nguy hiểm, và đồng ý rằng **một mình bạn hoàn toàn chịu trách nhiệm cho tất cả hậu quả**, dù mong đợi hay không mong đợi.
>
> **Nếu bạn không hiểu rõ những gì mình đang làm, DỪNG LẠI NGAY và KHÔNG TIẾP TỤC.**

<details>
<summary>Tôi chắc chắn, để tôi "vọc" thiết bị của tôi</summary>

### Yêu cầu tiên quyết

Hiện tại, bạn bắt buộc phải sử dụng U-Boot để flash firmware. Flash trực tiếp từ firmware gốc chưa được hỗ trợ (ít nhất là cho đến bây giờ).

- FPT AX3000CV2 đang chạy firmware stock QSDK.
- Quyền truy cập vào dòng lệnh U-Boot (qua UART).
  <details>
  <summary>Xem setup của tôi</summary>
  ESP32 được dùng làm cầu nối UART (UART bridge)

  ![img](https://github.com/user-attachments/assets/3e4140af-9982-4330-b0f6-5b8d51853c84)

  Code để sử dụng ESP32 làm cầu nối UART:

  ```c
  #include <HardwareSerial.h>
  
  #define RX 14
  #define TX 13
  
  HardwareSerial sr(2);
  
  void setup() {
    Serial.begin(115200);
    sr.begin(115200, SERIAL_8N1, RX, TX);
  }
  
  void loop() {
    while (Serial.available()) sr.write(Serial.read());
    while (sr.available())     Serial.write(sr.read());
  }
  ```
  
  </details>
- Một máy tính có phần mềm TFTP (Tftpd hoặc tương tự), và bạn cần biết địa chỉ IP của PC/Gateway của mình.

### 0. Sao lưu firmware gốc (dump toàn bộ NAND)

> **Hãy lưu firmware gốc của bạn!**
>
> Đừng bỏ qua bước này.  
> Bạn có thể nghĩ nó không cần thiết vì bạn không định quay lại "quá khứ đen tối", nhưng tôi đảm bảo rằng firmware stock vẫn hữu ích để khôi phục các cài đặt, driver bị thiếu và để nghiên cứu thêm.

1. Bật nguồn thiết bị và nhấn liên tục phím **Escape (ESC)** để vào giao diện dòng lệnh U-Boot.
2. Thiết lập các biến môi trường để U-Boot có thể kết nối với mạng cục bộ và tìm máy chủ TFTP của bạn:
   ```sh
   # Điều chỉnh các giá trị này phù hợp với thiết lập của bạn
   setenv ipaddr 192.168.2.240
   setenv gatewayip 192.168.2.1
   setenv serverip 192.168.2.104
   # Lưu vào NAND để sử dụng sau hoặc debug
   saveenv
   ```
3. Để an toàn, file dump sẽ được chia thành 4 phần. Mỗi phần 64 MB, vì kích thước NAND là 256 MB.
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
4. Nếu bạn muốn khôi phục firmware gốc:
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

### 1. Flash firmware

1. Xóa firmware cũ, bắt đầu từ `0xd00000` (~13 MB) với kích thước `0xf300000` (~249 MB):
   ```
   nand scrub 0xd00000 0xf300000
   ```
   Nếu được nhắc, hãy nhấn `y` rồi nhấn **Enter**.
2. Đảm bảo bạn đã tải xuống tệp có đuôi `...-qualcommax-ipq50xx-fpt_ax3000cv2-squashfs-factory.ubi` từ trang [Releases](https://github.com/michioxd/openwrt-wf810df/releases/latest). Đổi tên nó để dễ gõ hơn. Ví dụ: `update.ubi`.
4. Tải image vào bộ nhớ:
   ```
   tftpboot 0x44000000 update.ubi
   ```
5. Flash vào NAND
   ```sh
   nand write 0x44000000 0xd00000 ${filesize}
   ```
   Đợi một chút để quá trình hoàn tất.

### 2. Thiết lập môi trường khởi động

Sau khi flash image, bạn cần tạo lệnh boot tùy chỉnh cho U-Boot. Lệnh `bootipq` sử dụng các phân vùng cố định, nên nó không nhận diện được firmware của ta.

```sh
setenv bootargs "ubi.mtd=20 root=/dev/ubiblock0_1 rootfstype=squashfs rootwait"
setenv bootowrt "setenv mtdparts mtdparts=nand0:0xf300000@0xd00000(rootfs); ubi part rootfs; ubi read 0x44000000 kernel; bootm 0x44000000"
setenv bootcmd "run bootowrt"
saveenv
```

Bây giờ bạn có thể dùng `run bootowrt` để khởi động vào firmware mới, hoặc đơn giản là chạy `reset` để khởi động lại :)

</details>

## Giấy phép

Giấy phép OpenWrt: [GPL-2.0](https://openwrt.org/license)

Kho lưu trữ này: GPL-2.0

## Lời cảm ơn

- Dự án OpenWrt và tất cả những người đóng góp
- Những người đóng góp cho [kho lưu trữ này](https://github.com/michioxd/openwrt-wf810df/graphs/contributors)
