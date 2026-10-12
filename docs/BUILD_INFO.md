# Build Đèn Deco 1.3.0 — 12/10/2026

## File OTA trong gói

- Arduino CLI 1.3.1; ESP32C3 Dev Module, USB CDC bật, flash 4 MB, phân vùng Default có OTA. Board option `FlashMode=qio`; header ứng dụng dùng DIO theo recipe của core.
- Arduino ESP32 **3.3.1**, Adafruit NeoPixel **1.15.5**, ArduinoJson **7.4.3**, đúng bộ phiên bản người dùng đang có.
- Build cuối **SUCCESS**: sketch **1.226.900 / 1.310.720 byte (93%)**, RAM tĩnh **40.816 / 327.680 byte (12%)**.
- File `firmware/DenDeco-1.3.0-ota.bin`: **1.227.040 byte**, còn **83.680 byte** trong một phân vùng ứng dụng 0x140000.
- SHA-256: `1344dff881a640ee9f38c53b052f5f1cf53685fe558a22757dc707575bc4fd68`.

File OTA chứa ứng dụng, không chứa bootloader/bảng phân vùng. Header đã kiểm tra magic ảnh ESP, chip ESP32-C3 và app descriptor; esptool 4.9.0 xác nhận checksum và image hash hợp lệ. Toàn bộ mười tệp `.ino`/`.h` trong thư mục DenDeco khớp snapshot dùng build. Nạp đầu bằng USB theo hướng dẫn; file này dùng cập nhật qua web sau đó.

Phân vùng default.csv: NVS 0x9000, OTA data 0xe000, app0 0x10000 và app1 0x150000; mỗi app **0x140000 (1.310.720 byte)**. OTA không đổi bảng phân vùng.

## Profile PlatformIO kiểm tra riêng

PlatformIO Core 6.1.18, espressif32 6.12.0, board esp32-c3-devkitm-1, Arduino ESP32 **2.0.17**, NeoPixel **1.12.5**, ArduinoJson **7.4.2**. Build cuối **SUCCESS**: flash code/data **972.516 / 1.310.720 byte**, RAM tĩnh **43.476 / 327.680 byte**. File OTA bàn giao là build **3.3.1** ở trên.

`LED_Check` giữ nguyên nguồn và kết quả build từ bản trước; không biên dịch lại hoặc thử bo trong lần bổ sung lịch này.

## Kiểm tra phần mềm bản 1.3.0

- **Core — PASS**: C++11, cảnh báo nghiêm ngặt, ASan/UBSan; đủ 14 chế độ, tint/favorite, trần, timer, cảm ứng, reset WiFi và cải tiến chuyển màu/Lấp lánh đã có.
- **Lịch tuần — PASS**: tám mục, UTC+7/ngày trong tuần/qua nửa đêm, phạm vi ngày, favorite, phút đồng bộ đầu, sửa lịch, không chạy bù, chống lặp khi giờ đi lùi và gom sự kiện ON/OFF.
- **Mặt trời — PASS**: tham chiếu độc lập PyEphem/XEphem, TP.HCM và các vị trí khác, biên đường đổi ngày, năm nhuận và epoch 64 bit, vùng không mọc/lặn, offset vượt nửa đêm dùng ngày gốc, cache và chống lặp. Với TP.HCM ngày 12/10/2026, mốc làm tròn **05:42 / 17:37 UTC+7**; mô hình tham chiếu dùng chân trời −0,833° tại mực nước biển.
- **Tự động/renderer — PASS**: ưu tiên OFF > ON tuần cuối > mặt trời, dusk > dawn; preset trực tiếp, mục trống/hỏng không đổi sang cấu hình khác; ramp từ đen tới đích, đối xứng/trần, tràn millis, dừng bằng tay, tắt đầu ramp không tăng sáng thoáng qua; phát hiện clock step so với millis. Nội suy RAM, không ghi flash từng bước.
- **Lưu trữ — PASS**: baseline mặc định chỉ ghi một lần khi lưu bảo trì; lỗi mở/ghi/readback không được báo thành công. Schema ánh sáng 2, migration và lưu mạng tiếp tục đạt. Kế hoạch namespace `deco-plan` dùng hai slot/CRC/schema 1, kiểm tra JSON, lỗi ghi dở/readback, phục hồi, thế hệ tràn và bảo vệ dữ liệu tương lai.
- **Web Playwright/Chromium — PASS**: tám mục giờ/ngày/favorite, tọa độ E6/TP.HCM, preset ấm và target favorite, giữ bản nháp qua polling/lỗi/phản hồi muộn, xác nhận bỏ nháp, trạng thái giờ/mất WiFi, không phát lại POST khi nối lại. Các kiểm tra 14 mode, hướng dẫn, ánh sáng, WiFi/mDNS/clipboard, timer và OTA vẫn đạt; không lỗi JavaScript hoặc tràn ngang ở 390×844 và 1280×900.
- **Rà soát tích hợp — hoàn tất**: callback SNTP chỉ báo về loop, không chờ lấy giờ; phút đầu/edit/clock step/OTA resume không chạy bù; xác minh baseline trước khi bắt đầu Bình minh, một lựa chọn tự động được áp dụng sau khi gom tất cả sự kiện.

Host và web mock kiểm tra thuật toán/giao diện, không xác nhận SNTP, truyền OTA, thời gian giữ khi mất WiFi hoặc ánh sáng thực trên ESP. Bảng **A01–A12** trong `KIEM_THU.md` dành cho lần thử trên bo/router/vỏ thật. Chưa nạp thiết bị từ môi trường cloud.
