# Build Đèn Deco 1.1.0 — 10/10/2026

## File OTA trong gói

- Arduino CLI 1.3.1; target ESP32C3 Dev Module, flash 4 MB, QIO và USB CDC bật.
- Arduino ESP32 3.3.1, Adafruit NeoPixel 1.15.5, ArduinoJson 7.4.3, giữ bộ phiên bản người dùng đang có.
- Phân vùng default.csv: NVS 0x9000, OTA data 0xe000, app0 0x10000 và app1 0x150000; mỗi app 0x140000 (1.310.720 byte).
- Build cuối: SUCCESS; sketch 1.132.584/1.310.720 byte (86%), RAM tĩnh 39.840/327.680 byte (12%). File ảnh ứng dụng 1.132.736 byte; SHA-256 đã đối chiếu và ghi ở `firmware/SHA256SUMS.txt`.

File `firmware/DenDeco-1.1.0-ota.bin` là ứng dụng, không chứa bootloader hoặc bảng phân vùng. Header được kiểm tra có magic ảnh ESP, chip ESP32-C3 và app descriptor; ảnh phải nằm trong phân vùng OTA. Nạp đầu qua USB theo hướng dẫn; file này dùng cho cập nhật từ web sau đó.

## Profile PlatformIO được kiểm tra riêng

`platformio.ini` giữ profile ESP32-C3 đã có: PlatformIO Core 6.1.18, espressif32 6.12.0, board esp32-c3-devkitm-1, Arduino ESP32 2.0.17, NeoPixel 1.12.5 và ArduinoJson 7.4.2. Build cuối SUCCESS: flash code/data 885.510/1.310.720 byte, RAM tĩnh 42.516/327.680 byte. Đây là profile kiểm tra thêm; file OTA 1.1.0 được bàn giao là build 3.3.1 ở trên.

## Kiểm tra phần mềm

- Core và chính sách WiFi: C++11, cảnh báo nghiêm ngặt, AddressSanitizer/UndefinedBehaviorSanitizer — PASS.
- Storage với ArduinoJson thật, Preferences mô phỏng, lỗi ghi/CRC và migration schema mạng 1 → 2 — PASS.
- Playwright/Chromium: điều khiển, timer, yêu thích, request OTA, quét WiFi, chọn SSID/bảo mật, hostname/sao chép HTTP, trạng thái AP, ô đang nhập và mất/kết nối lại — PASS.
- Màn hình 390×844 và 1280×900: không tràn ngang, không có lỗi JavaScript.

`LED_Check` giữ nguyên nguồn từ bản 1.0.0; đã build với profile trên ở bản trước (flash 248.858 byte, RAM tĩnh 13.964 byte).

Profile dùng để xác minh biên dịch cho C3. Flash 4 MB, chân của SuperMini, WiFi/mDNS/OTA thật và cảm ứng qua vỏ cần thử trên bo của bạn. Chưa nạp thiết bị từ môi trường cloud.
