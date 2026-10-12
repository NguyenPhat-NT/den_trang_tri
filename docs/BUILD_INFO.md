# Build Đèn Deco 1.2.0 — 12/10/2026

## File OTA trong gói

- Arduino CLI 1.3.1; target ESP32C3 Dev Module, flash 4 MB và USB CDC bật. Board option `FlashMode=qio`; header ứng dụng dùng DIO theo recipe của core.
- Arduino ESP32 3.3.1, Adafruit NeoPixel 1.15.5, ArduinoJson 7.4.3, giữ bộ phiên bản người dùng đang có.
- Phân vùng default.csv: NVS 0x9000, OTA data 0xe000, app0 0x10000 và app1 0x150000; mỗi app 0x140000 (1.310.720 byte).
- Build cuối: SUCCESS; sketch 1.147.218/1.310.720 byte (87%), RAM tĩnh 39.840/327.680 byte (12%). File ảnh ứng dụng 1.147.360 byte; SHA-256 đã đối chiếu và ghi ở `firmware/SHA256SUMS.txt`.

File `firmware/DenDeco-1.2.0-ota.bin` là ứng dụng, không chứa bootloader hoặc bảng phân vùng. Header được kiểm tra có magic ảnh ESP, chip ESP32-C3 và app descriptor; ảnh nằm trong phân vùng OTA. esptool kiểm tra checksum và image hash hợp lệ. Nạp đầu qua USB theo hướng dẫn; file này dùng cho cập nhật từ web sau đó.

## Profile PlatformIO được kiểm tra riêng

`platformio.ini` giữ profile ESP32-C3 đã có: PlatformIO Core 6.1.18, espressif32 6.12.0, board esp32-c3-devkitm-1, Arduino ESP32 2.0.17, NeoPixel 1.12.5 và ArduinoJson 7.4.2. Build cuối SUCCESS: flash code/data 900.018/1.310.720 byte, RAM tĩnh 42.516/327.680 byte. Đây là profile kiểm tra thêm; file OTA 1.2.0 được bàn giao là build 3.3.1 ở trên.

## Kiểm tra phần mềm

- Core: C++11, cảnh báo nghiêm ngặt, AddressSanitizer/UndefinedBehaviorSanitizer — PASS. Kiểm tra đủ 14 mode, tint, yêu thích, trần, tắt/fade/timer, đối xứng, tốc độ, độ liên tục và uptime dài. Chính sách WiFi giữ nguyên từ 1.1.0, không sửa ở bản này.
- Storage với ArduinoJson thật, Preferences mô phỏng — PASS. Đọc dữ liệu ánh sáng schema 1, giữ cả sáu ID cũ và toàn bộ cấu hình; ghi schema 2/lưu mode mới, đọc hai schema cùng slot qua tràn số thế hệ, fallback bản cũ sau ghi dở và bảo vệ schema tương lai. Bộ kiểm tra migration mạng cũng chạy thành công.
- Playwright/Chromium — PASS. Đủ 14 ID/tên/mô tả và quy tắc tốc độ/màu, đổi màu không mất ba hiệu ứng tint, lệnh mode/màu nối tiếp khi phản hồi chậm, hướng dẫn dùng bàn phím/đọc khi offline, khôi phục đúng bật/tắt đã lưu. Kiểm tra cũ cho timer/yêu thích/WiFi/ô nhập/OTA cũng đạt; OTA bị từ chối không để lại lựa chọn mode chưa thực hiện.
- Màn hình 390×844 và 1280×900: không tràn ngang, không có lỗi JavaScript.

`LED_Check` giữ nguyên nguồn từ bản 1.0.0; đã build với profile trên ở bản trước (flash 248.858 byte, RAM tĩnh 13.964 byte).

Người dùng đã lắp đèn và báo bản trước chạy mượt. Bản 1.2.0 mới cần thử trên bo/vỏ thật: hiệu ứng, tốc độ/tint, mục hướng dẫn và nâng cấp OTA giữ cấu hình. Chưa nạp thiết bị từ môi trường cloud.
