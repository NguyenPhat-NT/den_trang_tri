# Đèn Deco — firmware 1.3.1

Đèn trang trí dùng ESP32-C3 SuperMini, 20 WS2812B và bốn TTP223. Điều khiển bằng cảm ứng hoặc web tiếng Việt ngay trên ESP; cập nhật firmware OTA qua web.

## Tải và nạp bằng Arduino IDE trên Windows

**[Tải gói Đèn Deco 1.3.1 (.zip)](https://github.com/NguyenPhat-NT/den_trang_tri/raw/refs/heads/main/downloads/DenDeco-1.3.1.zip)** · **[Tải hướng dẫn Word](https://github.com/NguyenPhat-NT/den_trang_tri/raw/refs/heads/main/HUONG_DAN.docx)**

1. Tải ZIP, giải nén cả gói.
2. Đọc `HUONG_DAN.docx` để xem bảng đấu nối và cài đặt IDE.
3. Mở `DenDeco/DenDeco.ino`, giữ các header trong cùng thư mục.
4. Giữ/cài **esp32 by Espressif Systems 3.3.1**, **Adafruit NeoPixel 1.15.5**, **ArduinoJson 7.4.3**. Đây là bộ phiên bản người dùng đang có; thông tin build của bản mới ghi trong `docs/BUILD_INFO.md`.
5. Chọn **ESP32C3 Dev Module**, USB CDC On Boot **Enabled**, flash **4 MB** nếu đúng bo thực tế và Partition Scheme **Default 4MB with spiffs** có OTA. Nạp lần đầu qua USB.
6. Nối AP **không có mật khẩu** `DenDeco-XXXXXX`, mở `http://192.168.4.1`, rồi quét/chọn WiFi nhà **2,4 GHz** và nhập mật khẩu của mạng nhà.
7. Khi đèn nhận IP từ router, AP tự tắt ngay. Chuyển điện thoại/PC sang WiFi nhà và mở **http://den-deco.local**. Địa chỉ này có thể đổi trong Cài đặt & kết nối; nếu thiết bị không hỗ trợ mDNS, dùng IP trong Serial Monitor 115200 baud hoặc danh sách DHCP của router.

Toàn bộ web nằm trong firmware; không cần nạp filesystem hoặc cài ứng dụng điện thoại. Muốn lấy toàn repo, dùng **Code → Download ZIP** trên GitHub.

**Nếu cả `.local` và IP đều không mở được:** sau khi đèn khởi động, thả hết nút rồi giữ đúng **+ và − 5 giây** để xóa WiFi và mở AP `DenDeco-XXXXXX`. Nếu vẫn không thấy AP, nạp **1.3.1 qua USB** bằng Arduino IDE, giữ **Erase All Flash Before Sketch Upload = Disabled** và đúng bảng phân vùng ở trên để giữ NVS. Tháo nguồn 5 V hằng ngày trước khi nối PC; dùng một nguồn USB khi nạp/xem Serial. Hướng dẫn đầy đủ ở mục 11 của `docs/HUONG_DAN.md`.

## Tính năng

- Bản 1.3.1 sửa luồng phục hồi WiFi khi trạng thái cache cũ hoặc lần kết nối driver còn chờ. Chỉ nhận WiFi nhà online khi driver còn liên kết với router và có IP; hủy lần kết nối đang chạy trước khi thử lại, bổ sung chẩn đoán web/Serial về lần lưu lịch, RAM và lần ngắt mạng cuối. Có nút **Sao chép chẩn đoán** trong cài đặt mạng để lấy dữ liệu sau khi web khôi phục. Riêng lỗi `AUTH_EXPIRE` chưa đủ giải thích việc không thấy AP trên bo đã báo lỗi.
- **Tối đa tám dòng lịch tuần bật/tắt**, chọn thứ và giờ Việt Nam UTC+7; mỗi lịch bật dùng một trong ba mục yêu thích đang được lưu trên đèn.
- Đồng bộ giờ Internet bằng SNTP bất đồng bộ qua WiFi nhà. Sau khi đã đồng bộ, mất WiFi vẫn tiếp tục giữ giờ trong phiên chạy; sau mất nguồn cần đồng bộ lại. Không chạy bù sự kiện bị lỡ.
- **Bình minh**: tùy chọn tăng sáng từ 0 khi đang tắt lên ánh sáng ấm 35% hoặc mục yêu thích, bắt đầu tại giờ mặt trời mọc; mặc định tăng trong 30 phút, chỉnh được 1–120 phút. **Hoàng hôn tự động**: tùy chọn bật ánh sáng ấm 35% hoặc mục yêu thích tại giờ mặt trời lặn.
- Tọa độ mặc định TP.HCM **10.776, 106.7**, chỉnh được; bình minh/hoàng hôn chọn thứ và độ lệch ±120 phút riêng, mặc định đều tắt. Giờ mặt trời tính trên ESP theo thiên văn, không cần API thời tiết.
- Điều khiển ánh sáng bằng tay dừng tăng sáng bình minh; các lịch tương lai vẫn hoạt động. Lịch tắt ưu tiên khi trùng phút với lịch bật hoặc mặt trời.
- Giữ cải tiến chuyển màu từ 1.2.1: khung hình mục tiêu khoảng 60/s, phân bố bước làm tròn màu theo vị trí cố định cho các hiệu ứng chuyển màu, giữ pha khi chỉnh tốc độ. Chuyển màu chậm/Cầu vồng có chu kỳ nhanh nhất 8/12 giây.
- Lấp lánh dùng cụm màu RGB đã chọn ở khoảng 1–3 tầng LED, xuất hiện/tắt dần trên nền gần đen để tạo tương phản rõ hơn trên bộ đèn có mật độ LED lớn. Độ mượt và độ tương phản cần thử lại trên bộ đèn thực tế.
- Bốn cảm ứng Nguồn / − / + / M, phân biệt chạm và giữ.
- **14 chế độ ánh sáng**: giữ sáu chế độ cũ, thêm Thở nhẹ, Lấp lánh, Cực quang, Biển xanh, Rừng xanh, Hoàng hôn, Sao băng và Sóng màu. Tất cả tám hiệu ứng mới chỉnh được tốc độ.
- Chọn RGB giữ hiệu ứng khi ở Thở nhẹ, Lấp lánh hoặc Sao băng; các chế độ dùng bảng màu riêng sẽ chuyển sang Một màu khi chọn RGB.
- Mục **Hướng dẫn sử dụng** ngay trên web giải thích thao tác cảm ứng, chế độ, màu, tốc độ, lưu và cập nhật.
- Chuyển sáng mềm và thang cảm nhận gamma 2,2; mã sáu chế độ cũ được giữ để nâng cấp vẫn đọc đúng các mục yêu thích.
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
| `firmware/DenDeco-1.3.1-ota.bin` | File ứng dụng cho OTA sau lần nạp USB đầu; xem thông tin build trong `docs/BUILD_INFO.md` |
| `firmware/SHA256SUMS.txt` | SHA-256 của file ứng dụng |
| `tests/` | Kiểm tra logic, lưu trữ và web |
| `platformio.ini` | Cấu hình build đã kiểm chứng; không bắt buộc cho Arduino IDE |
| `downloads/DenDeco-1.3.1.zip` | Gói bàn giao đủ mã nguồn, hướng dẫn Word và file OTA |

## Cấu hình và kiểm chứng

Nguồn tham chiếu **5 V–2 A**; 20 LED chia hai đoạn 10. Nguồn/−/+/M dùng GPIO **0/1/3/4**, dữ liệu LED GPIO **5**. TTP223 cấp **3,3 V**, direct/momentary active high. Bốn đầu vào dùng `INPUT_PULLDOWN` để không nhận mức cao do chân bỏ trống khi chưa đấu cảm ứng. Vỏ do người lắp tự thiết kế.

Thông tin biên dịch và kiểm tra phần mềm theo từng phiên bản nằm trong `docs/BUILD_INFO.md` và `docs/KIEM_THU.md`. Người dùng đã thử 1.3.0: lưu lại lịch mặt trời không đổi vẫn truy cập được; thay đổi lịch rồi lưu nhận thông báo đã lưu, sau đó web/IP mất truy cập. Kế hoạch còn giữ khi web khôi phục; LED/cảm ứng vẫn hoạt động. Đã gặp reason WiFi 2, nhưng chưa xác định nguyên nhân của lần mất truy cập sau lưu; kết quả reset +/− chưa xác nhận. **Độ ổn định sau cập nhật 1.3.1 cần thử trên bộ đèn đó**; các ca giờ/lịch/mặt trời và đo nguồn/nhiệt chưa có kết quả riêng để đánh dấu Đạt. Trần đầu ra ban đầu 30% không xác nhận khả năng chịu dòng của đường USB-C → chân 5V trên SuperMini; giữ trần và đo tải trước khi tăng.

Nâng cấp từ 1.3.0 lên 1.3.1 giữ WiFi, tên, cấu hình đèn, ba mục yêu thích và kế hoạch/tọa độ; dùng OTA khi web còn truy cập được hoặc USB khi cần phục hồi. Cả 14 mã chế độ, quy tắc RGB và schema ánh sáng 2 không đổi. Lịch/tọa độ lưu riêng trong NVS; các tự động mới mặc định tắt khi chưa có kế hoạch. Nâng cấp từ 1.2.x/1.1.0 vẫn đọc dữ liệu cũ; nếu cần quay lại 1.1.0 sau khi đã lưu schema 2, dùng USB vì firmware cũ có thể không đọc được dữ liệu đó và sẽ không ghi đè.

OTA chỉ cập nhật ứng dụng; đổi bảng phân vùng hoặc phục hồi firmware lỗi cần USB. File OTA không phải bootloader hoặc file flash gộp. Không có SSID/mật khẩu WiFi nhà trong mã nguồn: cấu hình tại thiết bị.
