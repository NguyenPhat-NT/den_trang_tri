# Build Đèn Deco 1.3.1 — 13/10/2026

## File OTA trong gói

- Arduino CLI 1.3.1; ESP32C3 Dev Module, USB CDC bật, flash 4 MB, phân vùng Default có OTA. Board option `FlashMode=qio`; header ứng dụng dùng DIO theo recipe của core.
- Arduino ESP32 **3.3.1**, Adafruit NeoPixel **1.15.5**, ArduinoJson **7.4.3**, đúng bộ phiên bản người dùng đang có.
- Build cuối **SUCCESS**, mã thoát 0: sketch **1.236.940 / 1.310.720 byte (94%)**, RAM tĩnh **40.928 / 327.680 byte (12%)**.
- File `firmware/DenDeco-1.3.1-ota.bin`: **1.237.088 byte**, còn **73.632 byte** trong một phân vùng ứng dụng 0x140000.
- SHA-256: `710429d46f946f78ee8fb6094cbbe710cd969de583bdf2e2b211d51564940fdd`.

File OTA chứa ứng dụng, không chứa bootloader/bảng phân vùng. Cả mười tệp `.ino`/`.h` khớp chính xác snapshot dùng build. esptool 5.1.0 xác nhận ảnh ESP32-C3, magic E9, chip ID 5, flash DIO/4 MB/80 MHz, checksum và validation hash hợp lệ. Nạp đầu hoặc phục hồi qua USB theo hướng dẫn; cập nhật qua web dùng file ứng dụng trên.

Phân vùng default.csv: NVS 0x9000, OTA data 0xe000, app0 0x10000 và app1 0x150000; mỗi app **0x140000 (1.310.720 byte)**. OTA không đổi bảng phân vùng.

## Profile PlatformIO kiểm tra riêng

PlatformIO Core 6.1.18, espressif32 6.12.0, board esp32-c3-devkitm-1, Arduino ESP32 **2.0.17**, NeoPixel **1.12.5**, ArduinoJson **7.4.2**. Build cuối **SUCCESS**: flash code/data **982.512 / 1.310.720 byte**, RAM tĩnh **43.588 / 327.680 byte**. File OTA bàn giao là build **3.3.1** ở trên.

`LED_Check` giữ nguyên nguồn và kết quả build từ bản trước; không biên dịch lại hoặc thử bo trong lần sửa WiFi/chẩn đoán này.

## Kiểm tra phần mềm bản 1.3.1

- **Mạng — PASS**: C++11, cảnh báo nghiêm ngặt, ASan/UBSan. Cổng hủy/bắt đầu có giới hạn 50/150 ms và 1,5 giây; driver idle không phát sự kiện vẫn bắt đầu được, không phát begin lặp; hủy/rearm, counter/millis tràn, cache connected cũ, liên kết/IP/giao diện thực, AP 30 giây và thử lại 60 giây.
- **Lưu ánh sáng/mạng — PASS**: ASan/UBSan với ArduinoJson 7.4.3; baseline lần đầu, lưu không đổi, lỗi mở/ghi/readback, schema cũ/mới, favorite, CRC, ghi dở và migration WiFi/hostname giữ dữ liệu.
- **Lưu kế hoạch — PASS**: ASan/UBSan; JSON tuần/mặt trời chặt chẽ, hai slot/CRC, lưu không đổi không ghi thêm, lỗi ghi/readback không tăng bộ đếm thành công, phục hồi, namespace độc lập, thế hệ tràn và bảo vệ dữ liệu tương lai.
- **Web Playwright/Chromium — PASS**: chẩn đoán WiFi sau mất/phục hồi, thời gian lưu/heap/stack, whitelist sao chép không có token/mật khẩu, fallback sao chép trên HTTP, chọn Ctrl+C, snapshot khi offline, tương thích thiếu trường cũ và khóa khi OTA. Toàn bộ kiểm tra giao diện lịch tuần/mặt trời, bản nháp/phản hồi muộn/no replay, 14 mode/RGB/help, ánh sáng/favorite/timer/OTA/WiFi/hostname/layout tiếp tục đạt; không lỗi JavaScript.
- **Rà soát tích hợp — hoàn tất**: xác nhận online bằng giao diện/liên kết/IP thay cache; hủy driver đang chờ trước thử lại; giữ thứ tự xác minh lưu ánh sáng rồi kế hoạch và áp dụng. Chẩn đoán chỉ RAM, không tăng ghi NVS. Stack high-water mark dùng byte thấp nhất còn trống trong vòng đời task, không nhân 4. Cờ Written chỉ ghi nhận lần ghi đã đọc kiểm tra thành công; stage complete chỉ cho biết hàm phản hồi phía ESP đã trả về.

Logic ánh sáng, cảm ứng, đồng bộ giờ, thuật toán tuần/mặt trời và ưu tiên tự động không đổi; kết quả kiểm tra thuật toán riêng của 1.3.0 được giữ trong `KIEM_THU.md`, không ghi lại thành kết quả thử bo mới.

## Phạm vi cần xác nhận trên đèn

Người dùng đã gặp trên **1.3.0**: lưu mặt trời không đổi vẫn truy cập; lưu có sửa báo đã lưu rồi web/IP mất truy cập; kế hoạch còn khi khôi phục, đèn/cảm ứng vẫn hoạt động. Đã có reason WiFi 2 ở một lần lỗi trước, chưa đối chiếu được với lần lưu. Hai vấn đề trong luồng WiFi được sửa từ rà soát SDK; **chưa xác định nguyên nhân thực tế của lỗi sau lưu hoặc xác nhận 1.3.1 hết lỗi trên bo**.

Build/host/web mock không xác nhận đường WiFi/HTTP/SNTP, truyền OTA, ánh sáng hoặc nguồn thực trên ESP. Các ca **W01–W10** ưu tiên thử phục hồi/lưu lịch; **A01–A12** giữ cho lịch và tự động. Khi lỗi lặp lại, sao chép chẩn đoán ngay sau khi web khôi phục, trước khi reboot hoặc lưu lần tiếp theo. Chưa nạp thiết bị từ môi trường cloud.
