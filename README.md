# Đèn Deco — firmware 1.1.0

Đèn trang trí dùng ESP32-C3 SuperMini, 20 WS2812B và bốn TTP223. Điều khiển bằng cảm ứng hoặc web tiếng Việt ngay trên ESP; cập nhật firmware OTA qua web.

## Tải và nạp bằng Arduino IDE trên Windows

**[Tải gói Đèn Deco 1.1.0 (.zip)](https://github.com/NguyenPhat-NT/den_trang_tri/raw/refs/heads/main/downloads/DenDeco-1.1.0.zip)** · **[Tải hướng dẫn Word](https://github.com/NguyenPhat-NT/den_trang_tri/raw/refs/heads/main/HUONG_DAN.docx)**

1. Tải ZIP, giải nén cả gói.
2. Đọc `HUONG_DAN.docx` để xem bảng đấu nối và cài đặt IDE.
3. Mở `DenDeco/DenDeco.ino`, giữ các header trong cùng thư mục.
4. Giữ/cài **esp32 by Espressif Systems 3.3.1**, **Adafruit NeoPixel 1.15.5**, **ArduinoJson 7.4.3**. Bộ phiên bản này đã biên dịch thành công; profile PlatformIO 2.0.17 cũng được kiểm tra riêng.
5. Chọn **ESP32C3 Dev Module**, USB CDC On Boot **Enabled**, flash **4 MB** nếu đúng bo thực tế và Partition Scheme **Default 4MB with spiffs** có OTA. Nạp lần đầu qua USB.
6. Nối AP **không có mật khẩu** `DenDeco-XXXXXX`, mở `http://192.168.4.1`, rồi quét/chọn WiFi nhà **2,4 GHz** và nhập mật khẩu của mạng nhà.
7. Khi đèn nhận IP từ router, AP tự tắt ngay. Chuyển điện thoại/PC sang WiFi nhà và mở **http://den-deco.local**. Địa chỉ này có thể đổi trong Cài đặt & kết nối; nếu thiết bị không hỗ trợ mDNS, dùng IP trong Serial Monitor 115200 baud hoặc danh sách DHCP của router.

Toàn bộ web nằm trong firmware; không cần nạp filesystem hoặc cài ứng dụng điện thoại. Muốn lấy toàn repo, dùng **Code → Download ZIP** trên GitHub.

## Tính năng

- Bốn cảm ứng Nguồn / − / + / M, phân biệt chạm và giữ.
- Sáu chế độ ánh sáng, chuyển sáng mềm và thang cảm nhận gamma 2,2.
- Chế độ ngủ nhớ riêng mức sáng đã chỉnh, mặc định ban đầu 5%.
- Ba mục yêu thích; hẹn giờ 1–180 phút và giảm sáng trong năm phút cuối.
- Lưu trạng thái sau 10 giây không thay đổi, kiểm tra CRC và hai bản NVS.
- Quét WiFi 2,4 GHz không chờ trong vòng lặp, chọn mạng và xem trạng thái kết nối trên web.
- Địa chỉ mặc định `http://den-deco.local`, chỉnh được tên truy cập; khi phát hiện trùng tên, đèn thêm mã bo vào địa chỉ thực tế. Nhiều đèn nên đặt tên khác nhau và dùng địa chỉ hiển thị trên web.
- AP dự phòng không mật khẩu chỉ mở khi chưa cấu hình, kết nối thất bại/mất mạng 30 giây hoặc reset WiFi. Nhận IP WiFi nhà sẽ tắt AP ngay.
- Giữ đúng hai nút + và − đồng thời 5 giây để xóa SSID/mật khẩu WiFi nhà; giữ cấu hình ánh sáng, tên đèn và tên truy cập.
- Đồng bộ trạng thái các trình duyệt, không phát lại lệnh cũ khi kết nối lại.
- OTA bằng file ứng dụng ESP32-C3; giữ cấu hình NVS sau cập nhật.

## Các tệp trong dự án

| Đường dẫn | Nội dung |
| --- | --- |
| `DenDeco/` | Sketch chính và cấu hình tập trung |
| `LED_Check/LED_Check.ino` | Kiểm tra màu, LED 0/9/10/19, cặp cùng chiều cao và OUT cảm ứng ở mức thấp |
| `HUONG_DAN.docx` | Đấu nối, nạp lần đầu, web, WiFi, OTA và nghiệm thu |
| `docs/` | Hướng dẫn Markdown, cấu hình/kế hoạch, kiểm thử và thông tin build |
| `firmware/DenDeco-1.1.0-ota.bin` | File ứng dụng cho OTA sau lần nạp USB đầu; xem thông tin build trong `docs/BUILD_INFO.md` |
| `firmware/SHA256SUMS.txt` | SHA-256 của file ứng dụng |
| `tests/` | Kiểm tra logic, lưu trữ và web |
| `platformio.ini` | Cấu hình build đã kiểm chứng; không bắt buộc cho Arduino IDE |
| `downloads/DenDeco-1.1.0.zip` | Gói bàn giao đủ mã nguồn, hướng dẫn Word và file OTA |

## Cấu hình và kiểm chứng

Nguồn tham chiếu **5 V–2 A**; 20 LED chia hai đoạn 10. Nguồn/−/+/M dùng GPIO **0/1/3/4**, dữ liệu LED GPIO **5**. TTP223 cấp **3,3 V**, direct/momentary active high. Bốn đầu vào dùng `INPUT_PULLDOWN` để không nhận mức cao do chân bỏ trống khi chưa đấu cảm ứng. Vỏ do người lắp tự thiết kế.

Thông tin biên dịch và kiểm tra phần mềm theo từng phiên bản nằm trong `docs/BUILD_INFO.md` và `docs/KIEM_THU.md`. **Chưa nạp, đo nguồn/nhiệt, thử WiFi/OTA hoặc thử cảm ứng qua vỏ trên bo thật.** Trần đầu ra ban đầu 30% không xác nhận khả năng chịu dòng của đường USB-C → chân 5V trên SuperMini; làm các bước thử trong hướng dẫn trước khi dùng lâu dài.

OTA chỉ cập nhật ứng dụng; đổi bảng phân vùng hoặc phục hồi firmware lỗi cần USB. File OTA không phải bootloader hoặc file flash gộp. Không có SSID/mật khẩu WiFi nhà trong mã nguồn: cấu hình tại thiết bị.
