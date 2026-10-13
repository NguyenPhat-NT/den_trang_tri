# Hướng dẫn Đèn Deco 1.3.1

Ngày bàn giao: 12/10/2026. Dành cho Windows và Arduino IDE.

## 1. Cấu hình

- ESP32-C3 SuperMini, dự kiến flash 4 MB.
- 20 LED WS2812B RGB: hai đoạn 10 LED, nối dữ liệu thành một chuỗi.
- Bốn TTP223, nguồn 3,3 V, chế độ momentary/direct active high.
- Nguồn USB 5 V–2 A. Vỏ và bố trí cơ khí do bạn thực hiện.
- Web tiếng Việt nằm trong firmware. Quét/chọn WiFi nhà 2,4 GHz, AP dự phòng không mật khẩu và địa chỉ mặc định `http://den-deco.local` chỉnh được.
- Hẹn giờ, ba mục yêu thích, lưu trạng thái và cập nhật OTA qua trình duyệt.
- 14 chế độ: sáu chế độ cũ cùng tám hiệu ứng mới; mục **Hướng dẫn sử dụng** nằm ngay trên web.
- Tám dòng lịch tuần bật/tắt, giờ Internet Việt Nam UTC+7, bình minh tăng sáng và bật đèn lúc mặt trời lặn; mặc định các tự động mới tắt.

## 2. Đấu nối

Ngắt nguồn trước khi đấu hoặc sửa dây. Đối chiếu ký hiệu trên bo SuperMini thực tế.

| Chức năng | Chân ESP32-C3 | Đấu tới |
| --- | --- | --- |
| Nguồn cảm ứng | 3V3 | VCC của cả 4 TTP223 |
| Mass chung | GND | GND của TTP223 và cả hai đoạn LED |
| Cảm ứng Nguồn | GPIO 0 | OUT nút Nguồn |
| Cảm ứng Giảm | GPIO 1 | OUT nút − |
| Cảm ứng Tăng | GPIO 3 | OUT nút + |
| Cảm ứng Chế độ | GPIO 4 | OUT nút M |
| Dữ liệu LED | GPIO 5 | DIN LED 0 |
| Nguồn LED | 5V | +5V của cả hai đoạn LED |

Cấp nguồn hằng ngày: nguồn USB 5 V–2 A → cổng USB-C của bo. Chân 5V cấp LED theo bản thử trong tài liệu gốc. Không cấp LED từ 3V3. Bo SuperMini chưa có thông số dòng cho đường USB-C → 5V: cần đo tải và nhiệt ở trần ban đầu trước khi dùng lâu dài. Trần phần mềm không phải bộ giới hạn dòng đo được.

Chiều dữ liệu:

```text
GPIO 5 → DIN LED 0 → ... → LED 9
                            │ DOUT tại đỉnh
                            ▼
                         DIN LED 10 → ... → LED 19

Đoạn A: LED 0 ở đáy, LED 9 ở đỉnh.
Đoạn B: LED 10 ở đỉnh, LED 19 ở đáy.
Cặp cùng độ cao: (0,19), (1,18), ... , (9,10).
DOUT LED 19 để trống.
```

Cả hai đoạn đều cần +5V/GND; nối dữ liệu không cấp nguồn cho đoạn tiếp theo. Bản thử dùng dữ liệu 3,3 V trực tiếp, chưa thêm tụ, điện trở hoặc IC đổi mức theo tài liệu. Kiểm chứng trên dây LED thật; nếu có nháy sai màu hoặc reset, kiểm tra nguồn và tín hiệu trước khi chỉnh phần mềm.

TTP223 phải cho OUT cao khi chạm và thấp khi thả. Nếu module đang toggle/latch, đổi pad theo chính module đó. Bốn đầu vào trong firmware dùng `INPUT_PULLDOWN`: chân chưa đấu cảm ứng được kéo về mức thấp để không tự phát lệnh. Sau khi đấu TTP223 vẫn cần kiểm tra OUT và thử chạm/giữ qua vỏ thực tế; kéo xuống không thay cho hiệu chỉnh độ nhạy cảm ứng. Tránh dùng GPIO 2/8/9 cho các nút vì liên quan khởi động; giữ 18/19 cho USB. GPIO 4/5 ở đây dùng GPIO, không đồng thời dùng JTAG ngoài.

## 3. Nạp lần đầu bằng Arduino IDE trên Windows

1. Giải nén cả gói; giữ nguyên toàn bộ thư mục `DenDeco`, gồm `DenDeco.ino` và các header `Config.h`, `Core.h`, `NetworkPolicy.h`, `Storage.h`, `WebUI.h`, `Schedule.h`, `Solar.h`, `Automation.h`, `ScheduleStorage.h`.
2. Trong Boards Manager, giữ/cài **esp32 by Espressif Systems 3.3.1** như bộ IDE bạn đang dùng. Nếu IDE chưa có nguồn package, thêm URL sau vào Preferences → Additional Boards Manager URLs:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

3. Trong Library Manager, giữ/cài **Adafruit NeoPixel 1.15.5** và **ArduinoJson 7.4.3**. WiFi, WebServer, Preferences, ESPmDNS và Update đi kèm core ESP32.
4. Mở `DenDeco/DenDeco.ino`. Chọn board **ESP32C3 Dev Module**.
5. Đặt thông số Tools:

| Mục | Giá trị |
| --- | --- |
| USB CDC On Boot | Enabled |
| CPU Frequency | 160 MHz |
| Flash Size | 4 MB, xác nhận phù hợp bo thực tế |
| Flash Mode | DIO |
| Flash Frequency | 80 MHz |
| Partition Scheme | Default 4MB with spiffs, có hai vùng OTA; không chọn No OTA hoặc Huge APP |
| Upload Speed | 115200 để nạp ban đầu ổn định |
| Erase All Flash Before Sketch Upload | Disabled để giữ cấu hình NVS |
| Port | COM của ESP32-C3 |

6. Trước khi nối PC: tắt đèn và chờ ít nhất 10 giây để lưu. Tháo nguồn USB hằng ngày, rồi nối PC bằng cáp USB-C có dữ liệu. Khi nạp lần đầu hoặc nghi đèn sẽ bật tải cao, tháo dây +5V LED khi không cấp nguồn và chỉ nạp bo; sau đó tháo PC, nối lại LED và dùng nguồn 5 V–2 A. Không nối đồng thời nguồn ngoài với VBUS của PC khi chưa xác minh chống cấp ngược.
7. Bấm Verify rồi Upload. Nếu không nhận COM, giữ BOOT, bấm/thả RESET rồi thả BOOT; chọn lại COM và nạp. Các bo có thể ghi nút là B/BOOT và R/RESET.
8. Sau nạp, bấm RESET nếu cần. Mở Serial Monitor **115200 baud**. Lần đầu chưa có bản lưu, đèn ở trạng thái tắt.

Bộ core/thư viện người dùng đang có là ESP32 3.3.1, NeoPixel 1.15.5 và ArduinoJson 7.4.3; giữ bộ này để biên dịch. Thông tin build và file OTA 1.3.1 ghi trong `BUILD_INFO.md`. Profile PlatformIO 2.0.17 là phương án build bổ sung; không cần đổi bộ Arduino IDE hiện tại về phiên bản đó. Người dùng đã nạp 1.3.0 và báo lỗi WiFi/web; cần xác nhận phục hồi 1.3.1 trên bo thực tế. Nếu bo có dung lượng flash khác 4 MB, chỉnh cấu hình và biên dịch lại cho bo thực tế.

## 4. Kết nối web lần đầu

Khi chưa có WiFi đã lưu, ESP mở AP tên `DenDeco-XXXXXX`, **không có mật khẩu**. `XXXXXX` là mã riêng của từng bo. Nối trực tiếp AP rồi mở `http://192.168.4.1`; không cần lấy mật khẩu AP từ Serial Monitor.

1. Dùng điện thoại hoặc PC nối AP của đèn; chọn giữ kết nối nếu hệ điều hành báo không có Internet.
2. Mở **http://192.168.4.1**; dùng HTTP, không dùng HTTPS.
3. Mở **Cài đặt & kết nối → Quét WiFi xung quanh**. Đợi kết quả rồi chọn mạng để điền tên WiFi. ESP32-C3 chỉ quét và kết nối **2,4 GHz**; với mạng ẩn, nhập đúng SSID thủ công. Trang báo đang quét, danh sách tìm thấy hoặc lỗi để thử lại. Quét chạy bất đồng bộ; hiệu ứng và cảm ứng tiếp tục hoạt động, nhưng thời gian phản hồi qua mạng cần kiểm chứng trên bo thật.
4. Nhập mật khẩu WiFi nhà rồi bấm **Lưu & kết nối**. Để trống mật khẩu với cùng SSID sẽ giữ mật khẩu đang lưu; chỉ chọn **Mạng không có mật khẩu** nếu mạng nhà là mạng mở. Mật khẩu đã lưu không được gửi lại cho trình duyệt.
5. Theo dõi trạng thái kết nối trên trang. **Ngay khi đèn kết nối WiFi nhà và nhận IP từ DHCP, AP tự tắt**, nên trang đang mở qua AP sẽ mất kết nối. Chuyển điện thoại/PC về WiFi nhà và mở **http://den-deco.local**, hoặc địa chỉ đã chỉnh trước đó.
6. Nếu kết nối thất bại sau **30 giây**, AP dự phòng mở để sửa mạng/mật khẩu. Nếu router mất kết nối, đèn thử nối lại và mở AP sau 30 giây chưa khôi phục; khi liên kết với router và nhận IP WiFi nhà trở lại, AP tự tắt ngay. Khi lỗi kéo dài, chu kỳ thử lại khoảng **60 giây**; nếu bước hủy kết nối driver bị kẹt, firmware có thể mở AP sớm hơn để giữ đường cấu hình. Điều khiển cảm ứng vẫn dùng được trong lúc đèn thử kết nối.

AP chỉ mở khi chưa cấu hình WiFi nhà, khi không kết nối được trong 30 giây hoặc khi bạn xóa/reset WiFi để cấu hình lại. AP không bật thường trực khi WiFi nhà hoạt động. Có thể xóa mạng từ web bằng **Xóa WiFi đã lưu**, hoặc giữ đúng **+ và − đồng thời 5 giây** như mục 5.

**Tên đèn** là nhãn hiển thị. **Tên truy cập trong WiFi nhà** quyết định địa chỉ `.local`: mặc định `den-deco` cho `http://den-deco.local`. Có thể đổi thành `den-phong-ngu`, `den-ban`… bằng nút **Lưu tên & địa chỉ**. Dùng 1–48 ký tự chữ thường a–z, số hoặc dấu gạch ngang; không bắt đầu/kết thúc bằng gạch ngang, không có khoảng trắng và không nhập `.local`. Sau khi đổi, dùng địa chỉ hiển thị trong **Địa chỉ trong WiFi nhà**. AP vẫn dùng mã bo `DenDeco-XXXXXX`.

Với nhiều đèn trên cùng mạng, đặt **tên truy cập khác nhau** cho từng đèn. Khi mDNS phát hiện trùng tên, firmware tự thêm mã bo sáu ký tự viết thường vào địa chỉ đang dùng, ví dụ `http://den-deco-a1b2c3.local`; nếu vẫn trùng, thêm tiếp `-2`, `-3`… Đèn giữ nguyên tên truy cập bạn đã lưu và chỉ thay tên thực tế của phiên kết nối, nên địa chỉ sau khi kết nối lại có thể thay đổi. Dùng địa chỉ trong **Địa chỉ trong WiFi nhà** hoặc Serial Monitor, không suy ra địa chỉ từ ô tên đã lưu. Cơ chế trùng tên còn cần thử với nhiều đèn thật; địa chỉ IP là cách truy cập dự phòng.

Địa chỉ `.local` dùng mDNS và chỉ có hiệu lực trong mạng nội bộ có hỗ trợ; điện thoại/PC phải ở cùng mạng với đèn. Mạng khách có cách ly thiết bị, router chặn multicast hoặc máy chưa hỗ trợ mDNS có thể không mở được địa chỉ này. Khi đó xem IP trong Serial Monitor 115200 baud hoặc danh sách DHCP của router rồi mở `http://<IP-của-đèn>`. IP phụ thuộc router; có thể đặt DHCP reservation theo MAC nếu muốn giữ IP ổn định.

IP còn nằm trong danh sách DHCP của router chỉ cho biết router đã cấp địa chỉ trước đó, không chứng minh đèn đang online. Nếu cả `.local` và IP đều không mở, dùng quy trình phục hồi ở mục 11; chỉ lỗi tên `.local` còn IP mở được mới giúp tách riêng vấn đề phân giải tên.

## 5. Sử dụng bốn nút

| Nút | Chạm ngắn, nhận khi thả | Giữ |
| --- | --- | --- |
| Nguồn | Bật/tắt | 2 giây: bật chế độ ngủ |
| − | Giảm 5 điểm sáng | Sau 0,6 giây, giảm 1 điểm mỗi 50 ms |
| + | Tăng 5 điểm sáng | Sau 0,6 giây, tăng 1 điểm mỗi 50 ms |
| M | Sang chế độ kế tiếp | 2 giây: hẹn 30 phút hoặc hủy hẹn giờ đang chạy |

Giữ rồi thả không phát thêm chạm ngắn. **Ngoại lệ cho hai nút + và −:** giữ đúng hai nút này đồng thời liên tục **5 giây** để reset WiFi nhà. Thao tác chỉ xóa SSID/mật khẩu WiFi nhà và mở AP không mật khẩu; giữ trạng thái ánh sáng, mục yêu thích, tên đèn và tên truy cập. Thả trước 5 giây hoặc chạm thêm Nguồn/M sẽ hủy thao tác reset. Sau reset cần thả hết các nút mới nhận thao tác tiếp theo.

Các tổ hợp nhiều nút khác sẽ hủy phần thao tác tiếp theo và chờ thả hết; lệnh giữ đã thực hiện trước đó không thể thu hồi. Sau cấp nguồn, firmware bỏ qua cảm ứng ít nhất 1 giây và chờ tất cả nút được thả. Vì vậy không dùng việc giữ +/− sẵn lúc cấp nguồn để reset mạng; đợi hết giai đoạn khởi động, thả hết rồi giữ lại.

Khi tắt, các thao tác riêng +/−/M được bỏ qua; tổ hợp +/− giữ 5 giây vẫn reset được WiFi. Giữ Nguồn vẫn bật chế độ ngủ. Nút tăng/giảm dừng ở 1 và 100, không tự tắt.

Thứ tự M: Ngủ → Một màu → Chuyển màu chậm → Ánh nến → Cầu vồng chậm → Ánh sáng dâng lên → Thở nhẹ → Lấp lánh → Cực quang → Biển xanh → Rừng xanh → Hoàng hôn → Sao băng → Sóng màu → Ngủ. Chạm M đi qua cả 14 chế độ; web cho chọn trực tiếp.

Sáu mã chế độ cũ 0–5 được giữ nguyên; tám chế độ mới nối tiếp ở 6–13 để nâng cấp không đổi ý nghĩa cấu hình/mục yêu thích đã lưu.

Màu ngủ là RGB (255,96,24). Lần đầu mức ngủ 5%; khi chỉnh sáng trong chế độ ngủ, đèn nhớ riêng mức này. Lần vào chế độ ngủ tiếp theo dùng mức đã nhớ. Sang chế độ khác giữ mức sáng hiện tại; không có bản nhớ riêng cho từng chế độ.

## 6. Web, độ sáng và hẹn giờ

Web và cảm ứng dùng cùng trạng thái. Chỉnh ở một nơi sẽ cập nhật những trình duyệt khác bằng polling 250 ms khi tab đang hoạt động. Hai lệnh gần nhau áp dụng theo thứ tự ESP xử lý.

Trang có mục **Hướng dẫn sử dụng** ngay trên web để xem nhanh cách chạm/giữ, chọn chế độ và màu, chỉnh tốc độ, lưu mục yêu thích, hẹn giờ, kết nối và OTA.

| Mã | Chế độ | Chỉnh tốc độ 1–100 | Khi chọn màu RGB |
| --- | --- | --- | --- |
| 0 | Ngủ | Không | Chuyển sang Một màu |
| 1 | Một màu | Không | Giữ Một màu |
| 2 | Chuyển màu chậm | Có | Chuyển sang Một màu |
| 3 | Ánh nến | Không | Chuyển sang Một màu |
| 4 | Cầu vồng chậm | Có | Chuyển sang Một màu |
| 5 | Ánh sáng dâng lên | Có | Chuyển sang Một màu |
| 6 | Thở nhẹ | Có | Đổi màu, giữ Thở nhẹ |
| 7 | Lấp lánh | Có | Đổi màu, giữ Lấp lánh |
| 8 | Cực quang | Có | Chuyển sang Một màu |
| 9 | Biển xanh | Có | Chuyển sang Một màu |
| 10 | Rừng xanh | Có | Chuyển sang Một màu |
| 11 | Hoàng hôn | Có | Chuyển sang Một màu |
| 12 | Sao băng | Có | Đổi màu, giữ Sao băng |
| 13 | Sóng màu | Có | Chuyển sang Một màu |

Thở nhẹ, Lấp lánh và Sao băng dùng màu RGB bạn chọn làm màu hiệu ứng. Cực quang, Biển xanh, Rừng xanh, Hoàng hôn và Sóng màu dùng bảng màu riêng. Cả tám hiệu ứng mới đều dùng thanh tốc độ; tăng giá trị làm hiệu ứng chạy nhanh hơn. Độ sáng chính và trần đầu ra vẫn áp dụng cho mọi chế độ.

Bản 1.2.1 giữ đủ 14 chế độ và sửa chuyển màu/Lấp lánh theo phản hồi trên đèn thật. Khung hình đặt cách nhau 16 ms, mục tiêu khoảng 60 khung hình/giây khi vòng lặp chạy bình thường. Pha hiệu ứng được giữ liên tục khi đổi tốc độ và qua các chu kỳ thời gian; thay tốc độ không đưa hiệu ứng nhảy sang một vị trí khác. Ở tốc độ 100, chu kỳ Chuyển màu chậm là 8 giây và Cầu vồng chậm là 12 giây, thay cho 1/5 giây ở bản 1.2.0.

Lấp lánh tạo cụm thưa bằng chính màu RGB đã chọn, khoảng 1–3 tầng chiều cao LED, xuất hiện và tắt dần trên nền gần đen. Hệ số nền giảm từ 20% ở bản 1.2.0 xuống khoảng 0,5%, rồi vẫn áp dụng độ sáng và trần đầu ra. Cách tán sáng của vỏ có thể làm cụm sáng hòa lại; thử màu, sáng và tốc độ trên vỏ đang dùng để đánh giá độ tương phản.

- Khi tắt, chỉnh sáng/màu/chế độ hoặc áp dụng mục yêu thích không tự bật. Nút Chế độ ngủ là hành động bật vào ngủ giống giữ Nguồn.
- Chọn RGB giữ chế độ ở Một màu, Thở nhẹ, Lấp lánh và Sao băng; ở các chế độ khác sẽ chuyển sang Một màu. Chọn lại hiệu ứng dùng bảng màu riêng để trở về hiệu ứng đó.
- Tốc độ 1–100 dùng cho Chuyển màu, Cầu vồng, Dâng lên và cả tám hiệu ứng mới; không làm đổi chế độ hoặc màu.
- Ba mục yêu thích lưu màu, chế độ, độ sáng và tốc độ; không lưu cờ bật/tắt hoặc hẹn giờ.
- Hẹn giờ 1–180 phút, chỉ khi đang bật. Năm phút cuối giảm sáng; thời lượng ngắn hơn 5 phút sẽ giảm từ đầu.
- Đổi thật độ sáng, màu, chế độ, tốc độ, trần hoặc áp dụng cấu hình khác sẽ hủy hẹn giờ. Sửa mục yêu thích, đổi tên/mạng hoặc gửi lại cùng giá trị không hủy.
- Khi mất kết nối, web khóa điều khiển. Kết nối lại chỉ đọc trạng thái; không tự gửi lại thao tác cũ.

Thanh sáng 1–100 là thang cảm nhận, được ánh xạ gamma 2,2. Trần ban đầu 30% mã đầu ra; trang cài đặt cho chỉnh 3–30%. Mức thấp nhất dùng `6/255` để màu cam ngủ còn các kênh màu ở mức thấp; đây là thông số khởi đầu cần thử trong vỏ. Nhiều mức thấp có thể trùng mã do LED chỉ có 8 bit mỗi kênh.

Để tạo màu tổng hợp gần các mức trung gian hơn, bản 1.2.1 dùng **làm tròn phân bố theo vị trí cố định** cho Chuyển màu chậm, Cầu vồng chậm, Cực quang, Biển xanh, Rừng xanh, Hoàng hôn và Sóng màu (mã 2/4/8/9/10/11/13). Với phần lẻ của mỗi kênh màu, mỗi tầng trong mười tầng chiều cao chọn một trong hai mã đầu ra 8 bit liền kề, chênh một mã. Hai LED cùng độ cao dùng cùng ngưỡng làm tròn; nếu màu nguồn của hiệu ứng giống nhau thì đầu ra cũng giống nhau. Cầu vồng vẫn giữ dải màu theo thứ tự chuỗi LED như bản trước. Đây là dithering không gian với mẫu cố định, **không dùng dithering theo thời gian hoặc PWM phần mềm**. Ngủ và Một màu giữ cách làm tròn tới mã gần nhất. Hiệu quả nhìn qua vỏ cần thử lại; việc tăng số khung hình và phân bố màu không bảo đảm mọi mức sáng đều hết bước màu.

Đổi thông số hiệu chỉnh trong `Config.h` nếu cần, rồi biên dịch lại. Không tăng trần kỹ thuật trước khi đo dòng, điện áp và nhiệt đường cấp nguồn. Firmware không có phép đo dòng hay nhiệt tự động.

## 7. Giờ Internet, lịch tuần và mặt trời

### Giờ Việt Nam và kết nối

Các lịch dùng giờ Việt Nam **UTC+7**, không đổi múi giờ theo tọa độ hoặc giờ của điện thoại. ESP đồng bộ giờ thực qua **SNTP** bất đồng bộ khi WiFi nhà có Internet; trang hiển thị giờ và trạng thái đồng bộ. Đèn chỉ đang phát AP lần đầu thì chưa có Internet để lấy giờ, nên các lịch chờ đến khi đồng bộ thành công. Cảm ứng, web và hiệu ứng vẫn dùng được trong lúc chờ.

Sau khi đã đồng bộ trong phiên chạy, mất WiFi/Internet vẫn tiếp tục giữ giờ bằng đồng hồ của ESP và các lịch vẫn hoạt động; độ chính xác có thể trôi nếu mất mạng lâu. Khi router có Internet trở lại, ESP tiếp tục đồng bộ. Sau mất nguồn/reboot, cần lấy giờ Internet lại trước khi lịch chạy; thiết bị không có RTC có pin để giữ giờ khi mất điện.

**Không chạy bù sự kiện bị lỡ.** Khi mới khởi động/đồng bộ lần đầu, lưu sửa kế hoạch, giờ bị nhảy hoặc kết thúc OTA, firmware lấy phút hiện tại làm mốc và chỉ chờ các phút kế tiếp. Vì vậy, lịch trùng phút đang cấu hình sẽ không bật đèn ngay; đặt giờ thử ít nhất một phút phía trước và lưu trước khi đến phút đó. Các lần đồng bộ định kỳ bình thường không chạy lại sự kiện đã thực hiện.

### Tám dòng lịch tuần

1. Trước khi tạo lịch bật, chuẩn bị ánh sáng rồi bấm **Lưu hiện tại** ở một trong ba mục yêu thích.
2. Trong mục lịch trên web, bật sử dụng lịch tuần. Có tối đa **tám dòng**; mỗi dòng chọn bật/tắt, giờ `HH:MM`, một hoặc nhiều thứ từ Thứ Hai đến Chủ nhật và bật sử dụng dòng đó.
3. Với dòng **bật**, chọn **Mục 1/2/3**; với dòng tắt không cần mục yêu thích. Bấm lưu kế hoạch và kiểm tra trạng thái lưu thành công.

Khi đến giờ bật, đèn đọc **nội dung hiện đang lưu** trong mục yêu thích đã chọn, áp dụng mode/màu/độ sáng/tốc độ và bật đèn. Sửa mục yêu thích trước lần lịch tiếp theo sẽ làm lần bật đó dùng cấu hình mới; lịch chỉ giữ số mục, không chép cố định màu vào dòng lịch. Nếu mục đã chọn trống hoặc dữ liệu ánh sáng không hợp lệ, sự kiện bật đó được bỏ qua an toàn. Muốn hủy một dòng, tắt sử dụng dòng rồi lưu; tắt lịch tuần không xóa các dòng đã nhập.

Ô đang chỉnh là bản nháp cho đến khi lưu thành công. Trang giữ các ô đang sửa khi cập nhật trạng thái định kỳ; giờ đèn vẫn chạy theo kế hoạch đã lưu. Các lịch/tọa độ được lưu riêng trong NVS, giữ qua reboot và OTA. Không phải chờ mốc lưu ánh sáng 10 giây để áp dụng kế hoạch đã lưu.

### Bình minh và bật lúc mặt trời lặn

Tọa độ đã chốt là **TP.HCM: vĩ độ 10.776, kinh độ 106.7**. Có thể nhập vĩ độ **−89 đến 89**, kinh độ **−180 đến 180** nếu đổi nơi dùng đèn. Các giờ hiển thị vẫn là UTC+7. Bình minh và mặt trời lặn là **hai lựa chọn độc lập**, mặc định đều tắt; mỗi lựa chọn có các thứ hoạt động và độ lệch **−120 đến +120 phút** riêng. Độ lệch âm chạy sớm, dương chạy muộn so với giờ mặt trời.

| Tự động | Thời điểm bắt đầu | Ánh sáng |
| --- | --- | --- |
| Bình minh | Đúng giờ mặt trời mọc, cộng độ lệch đã chọn | Khi đang tắt, tăng từ 0 lên mục yêu thích hoặc màu ấm có sẵn trong **30 phút**, chỉnh được **1–120 phút** |
| Mặt trời lặn | Đúng giờ mặt trời lặn, cộng độ lệch đã chọn | Bật mục yêu thích hoặc màu ấm có sẵn |

Màu ấm có sẵn là **RGB (255,160,72), độ sáng 35%** trên thang sáng chính, vẫn chịu trần đầu ra. Lựa chọn này dùng được dù chưa lưu mục yêu thích. Nếu chọn Mục 1/2/3, tự động đọc cấu hình mục đó khi bắt đầu sự kiện; mục trống/hỏng thì bỏ qua, không tự đổi sang màu ấm.

Bình minh **bắt đầu tăng tại giờ mặt trời mọc**, không tăng trước để đạt sáng tối đa đúng lúc mọc. Độ lệch cho phép dời thời điểm bắt đầu nếu muốn; kết thúc tăng sau thời lượng đã chọn. Các thao tác điều khiển ánh sáng bằng cảm ứng/web dừng lần tăng đang chạy và áp dụng lệnh tay. Việc này không tắt các lịch: sự kiện tương lai vẫn tiếp tục chạy.

Nếu đèn đã bật, Bình minh chuyển từ mức đang hiển thị tới mức đích. Các bước tăng sáng chạy trong RAM; cấu hình ánh sáng được lưu khi hoàn tất hoặc khi bạn dừng bằng tay. Lưu kế hoạch hoặc bắt đầu OTA cũng dừng lần tăng hiện tại ở mức gần nhất có thể lưu. Nếu mất nguồn trong lúc tăng, đèn khôi phục cấu hình trước khi bắt đầu, rồi chờ đồng bộ giờ và sự kiện tương lai; không tăng tiếp giữa chừng. Nếu chưa lưu được cấu hình nền, sự kiện Bình minh được bỏ qua và web báo lỗi.

Giờ mọc/lặn được tính **trên ESP** theo công thức thiên văn kiểu NOAA và tọa độ/ngày, với giả định đường chân trời chuẩn; giờ được làm tròn đến phút. Đây là ước tính thiên văn, **không phải dự báo thời tiết**; không cần tài khoản, API key hay dịch vụ thời tiết. Mây, địa hình, nhà cửa và cách nhìn mặt trời thực tế có thể làm thời điểm quan sát khác với giờ tính. Nếu vị trí/ngày không có sự kiện mọc hoặc lặn hợp lệ, tự động tương ứng không chạy ngày đó; lịch tuần theo giờ vẫn dùng được.

**Hoàng hôn** trong danh sách 14 hiệu ứng (mã 11) vẫn là một hiệu ứng màu. Tự động lúc mặt trời lặn là lịch bật theo giờ thiên văn; chỉ chọn mục yêu thích chứa hiệu ứng Hoàng hôn khi muốn lịch bật vào hiệu ứng này.

### Trùng lịch và hẹn giờ đếm ngược

Mỗi phút sự kiện chỉ được quyết định một lần theo thứ tự ưu tiên:

1. Có lịch tuần **tắt**: tắt thắng tất cả lịch bật và sự kiện mặt trời trùng phút.
2. Nếu không có lịch tắt, lịch tuần **bật** thắng sự kiện mặt trời; nhiều dòng bật cùng phút thì dòng bật sau cùng trong danh sách thắng.
3. Nếu chỉ có hai sự kiện mặt trời trùng phút, **mặt trời lặn** thắng bình minh.

Đèn áp dụng kết quả được chọn một lần, tránh bật rồi tắt do nhiều sự kiện cùng phút. Sự kiện lịch được áp dụng sẽ hủy hẹn giờ đếm ngược đang chạy. Hẹn giờ đếm ngược 1–180 phút vẫn là chức năng riêng; các lần lịch tương lai không bị hủy khi thao tác tay hoặc đặt/hủy đếm ngược.

### Thử nhanh trên đèn

- **Lịch tuần một phút:** đợi trang có giờ đồng bộ, lưu Mục 1, đặt một dòng bật cho hôm nay ở phút kế tiếp và một dòng tắt một phút sau đó. Lưu trước giờ bật, giữ trang mở để đối chiếu giờ và trạng thái. Nếu đã sang phút bật lúc lưu, đặt lại sang phút sau; không sửa đồng hồ để ép chạy.
- **Giữ giờ khi mất WiFi:** sau khi đã đồng bộ, đặt lịch tương lai rồi tắt router; kiểm tra giờ tiếp tục chạy và sự kiện đến đúng phút. Bật router lại, kiểm tra đồng bộ/khôi phục. Reboot khi không có Internet phải chờ giờ mới, không chạy bù lịch cũ.
- **Bình minh thử ngắn:** bật bình minh cho hôm nay, chọn màu ấm có sẵn và thời lượng 1 phút. Dùng độ lệch ±120 phút để dời giờ mặt trời mọc sang một phút tương lai nếu thời điểm hiện tại nằm trong phạm vi đó; nếu không, thử gần giờ mọc ngày tiếp theo. Giữ đồng hồ thực; không giả giờ và không dùng chế độ mô phỏng để xác nhận phần cứng. Sau thử trả thời lượng/độ lệch về giá trị dùng hằng ngày.
- **OTA giữ kế hoạch:** lưu các dòng/tọa độ, thực hiện OTA 1.3.1 rồi kiểm tra lại. Đợi SNTP đồng bộ sau reboot và đặt một sự kiện tương lai mới; sự kiện bị lỡ trong OTA không được chạy bù.

## 8. Lưu trạng thái

Sau thay đổi thực sự, đèn áp dụng ngay rồi chờ **10 giây từ thay đổi cuối** để lưu. Web hiển thị Đang chờ lưu, Đã lưu sau khi ghi/đọc kiểm tra thành công, hoặc lỗi lưu. Chưa có bản lưu sẽ hiển thị Cấu hình mặc định.

Lưu: bật/tắt, RGB, chế độ, độ sáng, tốc độ, mức ngủ riêng, trần đầu ra và ba mục yêu thích. Tên đèn, tên truy cập, SSID và mật khẩu WiFi nhà lưu riêng khi bấm áp dụng. AP không có mật khẩu. Reset WiFi chỉ xóa SSID/mật khẩu mạng nhà; giữ cấu hình ánh sáng và tên. Không lưu pha hiệu ứng, khung hình hoặc thời gian hẹn giờ còn lại.

Rút nguồn trước 10 giây sẽ mất phần chỉnh chưa lưu. Hiệu ứng, chuyển sáng và hẹn giờ giảm sáng không ghi flash mỗi khung hình. Hết hẹn giờ tạo thay đổi tắt, rồi vẫn chờ 10 giây để lưu.

Firmware có hai bản NVS, CRC và số thế hệ; boot chọn bản hợp lệ mới nhất. Khi ghi lỗi, giữ RAM và thử lại sau 5 giây. Khi gặp schema mới hơn, không tự ghi đè bằng firmware cũ.

Bản 1.3.1 tiếp tục đọc dữ liệu ánh sáng schema 1 và 2, ghi schema 2 như bản 1.2.x/1.3.0. Nâng cấp từ 1.3.0 giữ cấu hình đèn, cả 14 mã chế độ, ba mục yêu thích, WiFi, tên và tên truy cập. Kế hoạch tuần/bình minh/mặt trời lặn và tọa độ lưu riêng trong namespace NVS `deco-plan`; không đổi schema ánh sáng. Nạp USB với Erase All Flash Disabled và giữ bảng phân vùng để giữ NVS. Nâng cấp từ 1.2.x/1.1.0 giữ dữ liệu cũ; nếu cần quay lại 1.1.0 sau khi dữ liệu đã lưu schema 2, dùng USB: firmware cũ có thể không đọc được schema mới và sẽ không ghi đè dữ liệu đó.

## 9. Cập nhật OTA

Lần nạp đầu tiên vẫn qua USB để có firmware và bảng phân vùng OTA. Sau đó dùng web qua WiFi nhà hoặc AP:

1. Trong Arduino IDE, giữ nguyên board 4 MB và Partition Scheme có OTA; chọn Sketch → Export Compiled Binary.
2. Tìm file ứng dụng **DenDeco.ino.bin** hoặc tên tương đương do IDE xuất. Không chọn `.bootloader.bin`, `.partitions.bin`, `.merged.bin` hoặc file dùng cho ESP32/S2/S3 khác. Gói bàn giao cũng có `firmware/DenDeco-1.3.1-ota.bin`; dùng để nâng cấp khi web vẫn truy cập được. Nếu cả IP/AP đều không mở, nạp sketch qua USB theo mục 11.
3. Mở Cập nhật firmware qua WiFi trên web, chọn file, bấm Cập nhật firmware và xác nhận.
4. Firmware lưu ngay các thay đổi đang chờ trước khi bắt đầu OTA, thay vì chờ 10 giây trong phiên bảo trì này. Khi lưu không thành công hoặc schema không hỗ trợ, OTA chưa bắt đầu.
5. Giữ nguồn và kết nối ổn định. Ánh sáng giữ khung hình hiện tại; cảm ứng, hiệu ứng và xử lý hẹn giờ tạm dừng trong lúc upload; bộ đếm thời gian chạy vẫn trôi. Không dùng điều khiển bình thường cho tới khi hoàn tất.
6. File được ghi vào phân vùng ứng dụng còn lại, kiểm tra kích thước, header/chip ESP32-C3, dấu ứng dụng và kiểm tra ảnh của thư viện Update. Khi hợp lệ, đèn trả thành công rồi khởi động lại sau khoảng 1,8 giây. Trang thử đồng bộ lại sau khoảng 7 giây.
7. Đèn khôi phục trạng thái đã lưu, tên/tên truy cập và WiFi. Hẹn giờ không tiếp tục sau khởi động, giống quy tắc mất điện. Nếu đang OTA qua AP và đã lưu WiFi nhà hợp lệ, sau reboot đèn nối mạng nhà rồi tắt AP; chuyển điện thoại/PC về mạng nhà và mở địa chỉ `.local` hoặc IP.

Giới hạn của phân vùng ứng dụng mặc định là 1.310.720 byte; kích thước file 1.3.1 được ghi trong `BUILD_INFO.md`. Nếu dùng sơ đồ phân vùng khác, giới hạn trên trang lấy từ phân vùng thực tế. Đổi bảng phân vùng phải nạp USB; OTA chỉ thay ứng dụng.

Nếu upload bị ngắt hoặc file bị từ chối trước khi hoàn tất, ứng dụng đang chạy vẫn được giữ. Nếu mất điện sau khi đã ghi firmware hợp lệ và chọn boot, lần bật sau có thể chạy firmware mới. Nếu firmware mới hợp lệ về định dạng nhưng có lỗi chạy, bản này chưa có rollback tự động sau boot; khôi phục bằng USB. Phiên OTA kiểm tra token và cùng origin; dùng trong mạng nội bộ, không mở cổng điều khiển ra Internet. Bản này chưa xác minh chữ ký số firmware.

## 10. Thử lần đầu và chẩn đoán

1. Nạp với nguồn LED tháo ra khi cần tránh tải trên USB PC.
2. Có thể nạp `LED_Check/LED_Check.ino` với cùng cấu hình IDE để thử dây ở mức thấp: toàn bộ LED lần lượt đỏ/xanh lá/xanh dương, sau đó LED 0/9/10/19 sáng riêng, rồi từng cặp đi lên. Serial Monitor báo bước thử và OUT từng cảm ứng. Sketch này không sửa NVS; sau thử nạp lại `DenDeco/DenDeco.ino`. Hoặc sau đấu đúng và dùng nguồn hằng ngày, bật Một màu ở mức thấp để kiểm tra các kênh. Nếu xanh/đỏ đổi chỗ, sửa `NEO_GRB` ở sketch theo dây thực tế rồi nạp lại.
3. Thử Dâng lên: hai bên phải cùng đi từ đáy lên. Nếu ngược một bên, kiểm tra chiều nối/chỉ số.
4. Thử từng nút, chạm 20 lần, giữ và chạm đồng thời qua vỏ. Kiểm tra thêm giữ đúng +/− 5 giây reset WiFi và giữ ngắn hơn/chạm nút thứ ba không reset. Khi chưa đấu cảm ứng, các chân bỏ trống phải không tự phát lệnh.
5. Kiểm tra chỉnh → chờ lưu → reset, và chỉnh → rút nguồn sớm → bật lại.
6. Quét/chọn mạng, thử mật khẩu sai để kiểm tra AP dự phòng; dùng AP cập nhật lại thông tin. Khi nhập đúng và nhận IP, xác nhận AP tắt ngay. Thử tắt/bật router để kiểm tra mở AP sau 30 giây và tự tắt khi mạng khôi phục. Thử địa chỉ `.local`, IP và tên riêng nếu dùng nhiều đèn.
7. Chạy trắng ở trần 30% và tải WiFi trong 30 phút; ghi điện áp, dòng và nhiệt. Dừng thử nếu có reset, sai màu hoặc nhiệt bất thường để xử lý đường nguồn trước khi dùng hàng ngày.
8. Thử OTA với file gói bàn giao, rồi kiểm tra lại phiên bản và cấu hình.
9. Chọn đủ tám hiệu ứng bổ sung trên web và đi đủ 14 chế độ bằng M. Thử tốc độ 1/50/100, đổi RGB khi đang Thở nhẹ/Lấp lánh/Sao băng, kiểm tra các hiệu ứng khác chuyển sang Một màu khi chọn RGB, rồi lưu/khôi phục mục yêu thích. Mở Hướng dẫn sử dụng trên điện thoại để đọc ngay tại trang.
10. Với bản 1.2.1, thử bảy chế độ chuyển màu ở sáng 5/50/100 và tốc độ 1/50/100 qua đúng vỏ đang dùng; đổi tốc độ trong lúc chạy để xem pha có liên tục. Thử Lấp lánh với vài màu RGB trên vỏ đang dùng, ghi độ tương phản và thời gian xuất hiện/tắt. Sau OTA 1.2.0 → 1.2.1 kiểm tra bật/tắt, mục yêu thích và WiFi giữ nguyên.

Không thấy AP: đợi 30 giây nếu đã lưu mạng; nếu WiFi nhà đang kết nối, AP đã tắt đúng thiết kế. Muốn cấu hình lại từ cảm ứng, giữ đúng +/− 5 giây sau khi đã khởi động và thả hết nút. AP không yêu cầu mật khẩu. Trang AP mất kết nối ngay sau Lưu & kết nối: chuyển sang WiFi nhà và mở `.local` hoặc IP. Không mở được `.local`: kiểm tra cùng mạng, tên truy cập và dùng IP từ router/Serial. Không thấy mạng khi quét: xác nhận mạng 2,4 GHz, thử lại hoặc nhập SSID thủ công. Đèn chạm tự bật: kiểm tra module direct active high, dây và vùng cảm ứng/vỏ. OTA báo file sai: chọn đúng ứng dụng `.ino.bin`, không dùng bootloader/merged.

Thông tin biên dịch và kiểm tra logic/giao diện bằng mô phỏng được ghi trong `BUILD_INFO.md` và `KIEM_THU.md`. Người dùng đã nạp 1.3.0: lưu lại lịch mặt trời không đổi vẫn dùng được; đổi lịch rồi lưu nhận thông báo đã lưu, sau đó web/IP mất truy cập. Khi truy cập khôi phục, kế hoạch vẫn còn; LED/cảm ứng vẫn chạy. Chưa xác định nguyên nhân mất truy cập sau lưu hoặc kết quả reset +/−. **Độ ổn định sau cập nhật 1.3.1 cần thử trên bộ đèn này**; các ca SNTP/lịch/mặt trời chưa có kết quả riêng. Khi web đang dùng được, có thể cập nhật OTA; nếu mất web/AP, dùng mục 11. Sau nạp, ưu tiên W01–W10, giữ A01–A12 và các ca ánh sáng/nguồn/cảm ứng để bổ sung kết quả.

## 11. Phục hồi khi web và AP không truy cập được

### Kiểm tra AP từ cảm ứng

Đợi đèn khởi động, thả hết bốn nút rồi giữ đúng **+ và − đồng thời 5 giây**. Thả cả hai sau khi giữ đủ thời gian, tìm AP **DenDeco-XXXXXX** bằng điện thoại/PC. Thao tác này chỉ xóa SSID/mật khẩu WiFi nhà; giữ cấu hình ánh sáng, mục yêu thích, kế hoạch, tên và hostname. Nếu thấy AP, nối không mật khẩu và mở **http://192.168.4.1**. Có thể cập nhật file OTA 1.3.1 khi trang hoạt động hoặc nhập lại WiFi nhà sau khi cập nhật.

Nếu không thấy AP hoặc không mở được trang AP, tiếp tục nạp USB. Việc IP còn xuất hiện trong router và LED/cảm ứng vẫn hoạt động chưa đủ để xác nhận đường HTTP/WiFi đang online. Khi cả tên `.local` và IP đều lỗi, chưa thể kết luận lỗi DNS, mật khẩu đã lưu hay firmware OTA bị hỏng.

### Nạp lại bằng Arduino IDE trên Windows, giữ NVS

1. Tải **DenDeco-1.3.1.zip**, giải nén cả gói và mở `DenDeco/DenDeco.ino` cùng toàn bộ header. File `.bin` OTA là ảnh ứng dụng cho web; lần phục hồi USB này dùng chức năng Upload sketch của Arduino IDE.
2. Giữ core **esp32 by Espressif Systems 3.3.1**, **Adafruit NeoPixel 1.15.5**, **ArduinoJson 7.4.3**. Chọn **ESP32C3 Dev Module**, **USB CDC On Boot = Enabled**, flash **4 MB** nếu đúng bo và **Partition Scheme = Default 4MB with spiffs** có OTA. Giữ bảng phân vùng đang dùng; **Erase All Flash Before Sketch Upload = Disabled** để giữ cấu hình NVS.
3. Tắt đèn bằng cảm ứng và chờ ít nhất 10 giây nếu có thay đổi ánh sáng cần lưu. Tháo nguồn 5 V hằng ngày và nguồn 5 V ngoài khác trước khi nối cáp dữ liệu USB-C từ PC. **Chỉ dùng một nguồn USB khi nạp hoặc xem Serial**, không cấp đồng thời 5 V ngoài vào bo với USB PC. Nếu cần tránh tải LED trên USB PC, tháo dây +5V LED khi đã ngắt nguồn rồi chỉ nạp bo.
4. Chọn COM, bấm **Verify** rồi **Upload**. Nếu chưa có COM, giữ BOOT, bấm/thả RESET rồi thả BOOT; chọn lại COM và nạp. Không bật Erase All Flash để xử lý lỗi kết nối.
5. Sau Upload, mở **Serial Monitor 115200 baud**, rồi bấm RESET nếu cần để xem từ dòng khởi động. Giữ cáp USB PC là nguồn duy nhất khi thu log. Firmware 1.3.1 in phiên bản/lý do reset, sự kiện STA/lần bắt đầu kết nối và bản tin mạng khoảng mỗi 10 giây: online, status, liên kết router, IP, AP, hostname, heap và uptime. Log không in mật khẩu.
6. Nếu WiFi nhà được giữ và kết nối lại, mở IP **hiện tại** từ Serial hoặc địa chỉ `.local` hiện tại. Nếu chưa kết nối được, kiểm tra AP sau 30 giây; AP có thể mở sớm hơn khi bước hủy driver bị kẹt. Sau khi xác nhận web hoạt động, tháo USB PC, nối lại nguồn/LED khi đã ngắt điện rồi dùng nguồn 5 V–2 A hằng ngày.
7. Kiểm tra lại cấu hình đèn, ba mục yêu thích, kế hoạch/tọa độ và tên. Nếu đã reset WiFi bằng +/−, nhập lại mạng nhà; giữ các cấu hình còn lại. Phục hồi thực tế được ghi ở bảng W01–W08, không suy ra thành công chỉ từ thông báo biên dịch/Upload.

### Đọc thông tin chẩn đoán

Lý do WiFi **2 = AUTH_EXPIRE** cho biết trạng thái xác thực đã hết hiệu lực; thông báo này **không tự chứng minh mật khẩu đã lưu sai**. Với core 3.3.1, trạng thái cache có thể còn báo `WL_CONNECTED` sau sự kiện này, trong khi driver không còn liên kết thực với router. IDF thường xóa IP khi ngắt, nên riêng reason 2/cache cũ chưa chứng minh vì sao AP không xuất hiện. Bản 1.3.1 kiểm tra cả `isConnected`, liên kết driver và IP, rồi hủy lần kết nối driver đang chờ trước khi bắt đầu lần mới; AP dự phòng được quyết định theo trạng thái thực này.

Khi web đã truy cập được, mở **Thông tin chẩn đoán kết nối** trong cài đặt mạng để đọc thông tin nâng cao. JSON ở **http://<IP-hiện-tại>/api/state** có đối tượng **wifiDiagnostics** để đối chiếu với log Serial:

| Trường | Ý nghĩa |
| --- | --- |
| `status` | Mã trạng thái cache Arduino, chỉ là thông tin quan sát |
| `interfaceReady` | Cờ `isConnected` của giao diện STA |
| `associated` | Driver hiện còn liên kết vật lý với router |
| `beginResult` | Kết quả số của lần begin cuối; −1 khi chưa gọi |
| `beginAttempts` | Số lần bắt đầu kết nối trong phiên chạy |
| `pending` | Đang chờ bước bắt đầu kết nối mới |
| `cancelTimedOut` | Bước hủy/chờ rời liên kết đã hết thời hạn |
| `disconnectError` | Lỗi từ API hủy kết nối driver; rỗng khi thành công hoặc driver đã không kết nối |
| `lastEvent` | Mã sự kiện WiFi cuối được ghi nhận |
| `lastDisconnectReason`, `lastDisconnectAtMs` | Lý do và thời điểm ms từ boot của lần ngắt được ghi gần nhất; bỏ qua ASSOC_LEAVE 8 do chủ động rời mạng |
| `disconnectCount` | Số lần ngắt được ghi trong phiên chạy, không tính lần chủ động rời nói trên |
| `powerSave` | Mã chế độ tiết kiệm WiFi đang dùng; −1 nếu không đọc được |

Mã cache, IP hoặc `beginResult` đơn lẻ không được dùng để kết luận online/thất bại. Đặc biệt, **beginResult = 4 không tự xác nhận lần kết nối mới đã thất bại** vì core có thể trả lại giá trị cache cũ. Đọc cùng `interfaceReady`, `associated`, IP hiện tại và trạng thái mạng trên trang. Ghi lại các bản tin trước/sau mất liên lạc để phân biệt lỗi driver, mạng và reset thiết bị.

Đối tượng **resources** ghi lý do reset (`resetReason`), heap đang trống (`freeHeap`), mức heap trống thấp nhất (`minFreeHeap`), khối heap trống lớn nhất (`largestFreeBlock`) và stack còn trống thấp nhất của loop (`loopStackFree`). Các giá trị bộ nhớ dùng **byte**. `loopStackFree` và `planSaveDiagnostics.stackFree` là mức thấp nhất còn trống trong vòng đời tác vụ theo ESP-IDF, không phải lượng stack trống tức thời và **không nhân thêm 4**.

Đối tượng **planSaveDiagnostics** ghi lần lưu lịch gần nhất:

| Nhóm trường | Ý nghĩa |
| --- | --- |
| `kind`, `stage`, `count` | Loại lịch, checkpoint hiện tại và số lần lưu trong phiên |
| `startedAtMs`, `totalMs` | Mốc ms từ boot và thời gian xử lý đã ghi nhận |
| `lightMs`, `planMs`, `applyMs`, `responseMs` | Thời gian lưu ánh sáng, lưu kế hoạch, áp dụng và gọi hàm gửi phản hồi trên ESP |
| `successful`, `lightWritten`, `planWritten` | Bước lưu/kiểm tra storage thành công; hai cờ Written ghi nhận lần ghi ánh sáng/kế hoạch đã đọc kiểm tra thành công |
| `heapBefore`, `heapAfter`, `stackFree` | Heap trước/sau và mức stack trống thấp nhất của loop |
| `onlineBefore/After`, `associatedBefore/After`, `interfaceBefore/After`, `ipBefore/After` | Trạng thái online, liên kết driver, giao diện STA và IP trước/sau lưu |

Phản hồi lưu có thể ghi stage **responding**; lần cập nhật trạng thái kế tiếp mới thấy **complete**. `successful=true` xác nhận bước lưu/kiểm tra storage đã thành công; hai cờ Written chỉ được bật khi lần ghi đã đọc kiểm tra thành công. Khi một lần lưu lỗi, cờ Written=false vẫn có thể đã ghi flash nhưng đọc kiểm tra thất bại. `complete`/`responseMs` chỉ ghi hàm gửi phản hồi phía ESP đã trả về, không xác nhận trình duyệt nhận được dữ liệu. Các bản ghi chẩn đoán chỉ nằm trong **RAM của phiên chạy**, mất khi reboot và không tạo thêm ghi NVS.

Để lấy bằng chứng lỗi sau **Lưu lịch mặt trời**:

1. Khi web truy cập lại, mở **Cài đặt & kết nối → Thông tin chẩn đoán kết nối → Sao chép chẩn đoán** trước khi reset nguồn. Bản sao dùng các trường được chọn sẵn, không chứa session token; tránh sao chép toàn bộ `/api/state` để chia sẻ.
2. Nếu tái thử, lưu lịch **không đổi một lần**, đợi một lần cập nhật trạng thái sau phản hồi rồi sao chép chẩn đoán.
3. Đổi **một giá trị** của lịch mặt trời, lưu **một lần** và thu bản sao/log Serial sau phản hồi hoặc ngay khi web trở lại. Không bấm lưu liên tục. Ghi thông báo, khả năng truy cập IP/AP và kế hoạch sau khi khôi phục để so sánh hai lần.

Kế hoạch còn giữ chứng minh dữ liệu đã phục hồi được; chưa đủ để quy nguyên nhân mất truy cập cho NVS, WiFi hoặc HTTP. Đối chiếu checkpoint/thời gian lưu, bộ nhớ và thời điểm/lý do ngắt mạng để tiếp tục xác định nguyên nhân.
