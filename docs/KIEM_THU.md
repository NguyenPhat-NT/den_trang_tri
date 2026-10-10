# Kiểm thử và nghiệm thu

## Đã thực hiện trên phần mềm

- Sketch chẩn đoán LED_Check cũng đã build thành công cho cùng ESP32-C3/core.
- Build thực tế ESP32-C3: PlatformIO espressif32 6.12.0, Arduino ESP32 2.0.17, NeoPixel 1.12.5, ArduinoJson 7.4.2; USB CDC bật và phân vùng OTA 4 MB.
- Kiểm tra Core.h bằng g++ với AddressSanitizer/UndefinedBehaviorSanitizer: bật/tắt giữ cấu hình, nhớ mức ngủ riêng, lệnh sai phạm vi, timer qua tràn millis, hủy timer/no-op, chạm/giữ/thả, nhiều nút và chạm đang giữ lúc boot.
- Kiểm tra renderer: đường sáng tăng đơn điệu, đầu ra không vượt cap, tắt về RGB 0, giữ màu ngủ ở mức thấp và ghép LED cùng chiều cao. Hủy timer giữa giảm sáng khôi phục mềm từ mức đang hiển thị.
- Kiểm tra Storage.h với ArduinoJson thật và Preferences mô phỏng: không ghi trước 10 giây, tính lại từ thay đổi cuối, khôi phục bản mới nhất, retry sau lỗi ghi, ghi dở một slot vẫn đọc slot hợp lệ, quay lại bản đã lưu bỏ ghi thừa, schema mới không bị ghi đè và CRC mạng.
- Kiểm tra web bằng Playwright/Chromium với API mô phỏng ở 390×844 và 1280×900: bật/tắt, sáng, màu, favorite, timer, giữ thông tin WiFi đang nhập khi polling, tên đèn, mất kết nối/khôi phục không phát lại lệnh, request OTA có token/kích thước và không có lỗi JavaScript. Không tràn ngang màn hình điện thoại.
- Kiểm tra file ứng dụng tạo ra có magic ảnh, chip ESP32-C3, dấu ứng dụng và nằm trong giới hạn phân vùng OTA; file đã có SHA-256 trong gói.

Các kiểm tra trên không thay cho flash/nạp USB, truyền OTA vào ESP, đo phản hồi mạng hoặc đo điện của thiết bị thật. Web mock kiểm tra giao diện và request, không mô phỏng stack WiFi, flash hoặc điện.

## Bảng chạy thử trên thiết bị

Ghi Ngày / Kết quả / Ghi chú cho từng dòng. Mục tiêu về thời gian cần đo trên mạng thực tế.

| Mã | Kiểm tra | Cần đạt | Kết quả thực tế |
| --- | --- | --- | --- |
| T01 | Nạp LED_Check để sáng riêng LED 0/9/10/19 | Đúng đáy/đỉnh theo mapping | Chưa thử |
| T02 | Dâng lên | Hai đoạn cùng đi từ đáy lên | Chưa thử |
| T03 | Mỗi nút 20 chạm qua vỏ | Một chạm/một lệnh, không chạm giả nút cạnh | Chưa thử |
| T04 | Giữ Nguồn/M rồi thả | Một lệnh giữ, không thêm chạm ngắn | Chưa thử |
| T05 | Giữ +/− tới hai biên | Dừng 1/100 và dừng khi thả | Chưa thử |
| T06 | Khi tắt chạm +/−/M | Không tự bật; Nguồn bật cấu hình cũ | Chưa thử |
| T07 | Hai trình duyệt và cảm ứng | Cùng trạng thái; mục tiêu ≤500 ms ở LAN ổn định | Chưa thử |
| T08 | Chỉnh giây 0 và giây 6 | Chỉ lưu khoảng giây 16 | Chưa thử |
| T09 | Rút nguồn trước/sau mốc lưu | Về bản cũ/bản mới tương ứng | Chưa thử |
| T10 | Chạy hiệu ứng không thao tác | Bộ đếm writes không tăng theo frame | Chưa thử |
| T11 | Hẹn/hủy/hết giờ | Giảm sáng và tắt; timer không khôi phục qua reset | Chưa thử |
| T12 | Tắt router/nhập sai WiFi | AP mở sau 30 giây, đèn vẫn điều khiển cảm ứng được | Chưa thử |
| T13 | Tải trắng cap 30% và WiFi 30 phút | Ghi dòng/áp/nhiệt; không reset hoặc sai màu | Chưa thử |
| T14 | Tắt, đợi lưu, nạp USB giữ NVS | Sau boot vẫn tắt, cảm ứng bật được | Chưa thử |
| T15 | Quét sáng 1–100 | Không giảm hoặc vượt cap; cho phép mức trùng | Chưa thử |
| T16 | Ngủ, giữ sáng và fade trong phòng tối | Ghi màu/bước nhảy/nhấp nháy để hiệu chỉnh | Chưa thử |
| O01 | OTA đúng file qua WiFi nhà | Thành công, reboot, đúng phiên bản, giữ cấu hình/WiFi | Chưa thử |
| O02 | OTA qua AP | Thành công như WiFi nhà | Chưa thử |
| O03 | File sai chip/bootloader/merged/quá lớn | Từ chối; firmware hiện tại còn hoạt động | Chưa thử |
| O04 | Ngắt upload giữa chừng | Ứng dụng đang chạy được giữ, điều khiển hoạt động lại | Chưa thử |
| O05 | Chỉnh chưa đủ 10 giây rồi OTA | Lưu trước cập nhật, boot phục hồi cấu hình đó | Chưa thử |
| O06 | Nạp USB phục hồi | Khôi phục được khi firmware mới có lỗi chạy | Chưa thử |

## Cách chạy lại kiểm tra phần mềm

Các lệnh dưới đây dành cho Linux/môi trường phát triển, không bắt buộc để sử dụng Arduino IDE trên Windows.

```sh
pio run -e esp32c3
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined tests/core_test.cpp -o core-test
./core-test
c++ -std=c++11 -Wall -Wextra -fsanitize=address,undefined -Itests/stubs -I.pio/libdeps/esp32c3/ArduinoJson/src tests/storage_test.cpp -o storage-test
./storage-test
node tests/web_test.cjs
```

Kiểm tra web cần package Playwright và Chromium. Đặt biến `DECO_BROWSER` bằng đường dẫn executable nếu Chromium không ở `/usr/bin/chromium`. Ảnh chụp web từ mock chỉ minh họa UI, không là bằng chứng thiết bị đã chạy.
