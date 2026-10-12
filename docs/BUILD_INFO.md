# Build Đèn Deco 1.2.1 — 12/10/2026

## File OTA trong gói

- Arduino CLI 1.3.1; target ESP32C3 Dev Module, flash 4 MB và USB CDC bật. Board option `FlashMode=qio`; header ứng dụng dùng DIO theo recipe của core.
- Arduino ESP32 3.3.1, Adafruit NeoPixel 1.15.5, ArduinoJson 7.4.3, giữ bộ phiên bản người dùng đang có.
- Phân vùng default.csv: NVS 0x9000, OTA data 0xe000, app0 0x10000 và app1 0x150000; mỗi app 0x140000 (1.310.720 byte).
- Build cuối: SUCCESS; sketch 1.147.970/1.310.720 byte (87%), RAM tĩnh 40.096/327.680 byte (12%). File ảnh ứng dụng 1.148.112 byte; SHA-256 đã đối chiếu và ghi ở `firmware/SHA256SUMS.txt`.

File `firmware/DenDeco-1.2.1-ota.bin` là ứng dụng, không chứa bootloader hoặc bảng phân vùng. Header được kiểm tra có magic ảnh ESP, chip ESP32-C3 và app descriptor; ảnh nằm trong phân vùng OTA. esptool kiểm tra checksum và image hash hợp lệ. Sáu tệp nguồn dùng để build khớp bản đóng gói. Nạp đầu qua USB theo hướng dẫn; file này dùng cho cập nhật từ web sau đó.

## Profile PlatformIO được kiểm tra riêng

`platformio.ini` giữ profile ESP32-C3 đã có: PlatformIO Core 6.1.18, espressif32 6.12.0, board esp32-c3-devkitm-1, Arduino ESP32 2.0.17, NeoPixel 1.12.5 và ArduinoJson 7.4.2. Build cuối SUCCESS: flash code/data 900.578/1.310.720 byte, RAM tĩnh 42.756/327.680 byte. Đây là profile kiểm tra thêm; file OTA 1.2.1 được bàn giao là build 3.3.1 ở trên.

## Kiểm tra phần mềm

- Core: C++11, cảnh báo nghiêm ngặt, AddressSanitizer/UndefinedBehaviorSanitizer — PASS. Kiểm tra đủ 14 mode/tint/yêu thích/cap/tắt/timer/fade; giữ pha khi đổi tốc độ, đầu ra chính xác ở khung đầu chuyển tiếp, mẫu làm tròn cố định không nhấp nháy theo thời gian, chu kỳ dài/tràn thời gian và cụm Lấp lánh thưa.
- Kiểm tra độc lập 11.648 trường hợp renderer — PASS; bao gồm trần 3/30%, timer 50% sai số tối đa một mã, khung đầu/partial fade, tốc độ, tắt và tràn thời gian.
- Lưu trữ/schema ánh sáng 2, trường cấu hình, 14 ID, mục yêu thích và chính sách WiFi không sửa ở bản 1.2.1. Các kiểm tra lưu/migration thành công của bản 1.2.0 ghi trong `KIEM_THU.md`.
- Playwright/Chromium — PASS. Bộ kiểm tra điều khiển, WiFi, ô nhập, màu/tốc độ/OTA giữ nguyên đạt; mô tả Lấp lánh trên nền tối đã kiểm tra. Sau khi đổi riêng lời khuyên độ sáng/tốc độ theo phòng/vỏ, kiểm tra nhắm tới hướng dẫn/bàn phím/layout cũng đạt.
- Màn hình 390×844 và 1280×900: không tràn ngang, không có lỗi JavaScript.

`LED_Check` giữ nguyên nguồn từ bản 1.0.0; đã build với profile trên ở bản trước (flash 248.858 byte, RAM tĩnh 13.964 byte).

## So sánh mô phỏng 1.2.0 → 1.2.1

Chuyển màu ở độ sáng 5%, tốc độ 50, mẫu 16 ms: số màu RGB tổng hợp 36 → 366; bước màu tổng hợp tối đa mỗi kênh 1 → 0,1 mã; thời gian giữ nguyên toàn bộ khung dài nhất 1.872 → 176 ms. Ở độ sáng 100% và tốc độ 100, bước lớn nhất từng LED 8 → 1 mã trong 16 ms; bản 1.2.0 thực dùng khung 33 ms có thể nhảy tới 16 mã. Những số này dành riêng cho ca Chuyển màu đã đo, không suy ra mọi bảng màu đều mượt hơn ở mức sáng 5%.

Với Lấp lánh RGB trắng, mô hình tán sáng Gaussian rộng 1,5 tầng LED cho thấy cụm sáng phân biệt hơn: khoảng 9,65 → 2,45 tầng gần mức sáng đỉnh. Chế độ mới ít LED sáng cùng lúc nên tổng ánh sáng thấp hơn và có khoảng tối giữa các cụm; mô hình không xác nhận hình dạng/tán sáng của vỏ thực tế.

Người dùng đã thử 1.2.0 và báo bước chuyển màu lớn, Lấp lánh khó nhận ra do mật độ LED. Bản 1.2.1 đã kiểm tra phần mềm; cảm nhận màu/cụm sáng, độ sáng phù hợp và OTA giữ cấu hình cần thử lại trên bo/vỏ thật. Chưa nạp thiết bị từ môi trường cloud.
