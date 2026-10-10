# Hướng dẫn Đèn Deco 1.0.0

Ngày bàn giao: 10/10/2026. Dành cho Windows và Arduino IDE.

## 1. Cấu hình

- ESP32-C3 SuperMini, dự kiến flash 4 MB.
- 20 LED WS2812B RGB: hai đoạn 10 LED, nối dữ liệu thành một chuỗi.
- Bốn TTP223, nguồn 3,3 V, chế độ momentary/direct active high.
- Nguồn USB 5 V–2 A. Vỏ và bố trí cơ khí do bạn thực hiện.
- Web tiếng Việt nằm trong firmware. WiFi nhà 2,4 GHz và AP dự phòng.
- Hẹn giờ, ba mục yêu thích, lưu trạng thái và cập nhật OTA qua trình duyệt.

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

TTP223 phải cho OUT cao khi chạm và thấp khi thả. Nếu module đang toggle/latch, đổi pad theo chính module đó. Tránh dùng GPIO 2/8/9 cho các nút vì liên quan khởi động; giữ 18/19 cho USB. GPIO 4/5 ở đây dùng GPIO, không đồng thời dùng JTAG ngoài.

## 3. Nạp lần đầu bằng Arduino IDE trên Windows

1. Giải nén cả gói; giữ `DenDeco.ino`, `Config.h`, `Core.h`, `Storage.h`, `WebUI.h` trong cùng thư mục `DenDeco`.
2. Trong Boards Manager, cài **esp32 by Espressif Systems**, chọn phiên bản **2.0.17**. Nếu IDE chưa có nguồn package, thêm URL sau vào Preferences → Additional Boards Manager URLs:

```text
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

3. Trong Library Manager, cài **Adafruit NeoPixel 1.12.5** và **ArduinoJson 7.4.2**. WiFi, WebServer, Preferences, ESPmDNS và Update đi kèm core ESP32.
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

Các kiểm tra biên dịch trong gói dùng PlatformIO và cùng phiên bản core/thư viện trên. Chưa kiểm chứng trực tiếp thao tác Upload trong Arduino IDE trên bo của bạn. Nếu bo có dung lượng flash khác 4 MB, không dùng sẵn file OTA 4 MB; chỉnh cấu hình và biên dịch lại cho bo thực tế.

## 4. Kết nối web lần đầu

Khi chưa có WiFi, ESP mở AP tên `DenDeco-XXXXXX`. Mật khẩu AP ngẫu nhiên riêng, 12 ký tự, được lưu trên đèn và in trong Serial Monitor khi AP mở:

```text
AP: DenDeco-XXXXXX
AP password: <mật khẩu riêng được đèn tạo>
Open http://192.168.4.1
```

Ghi lại mật khẩu để dùng khi AP dự phòng mở. Firmware không dùng mật khẩu chung cho mọi đèn. Nếu cửa sổ Serial mở quá muộn, mở sẵn rồi bấm RESET để xem lại dòng AP.

1. Dùng điện thoại hoặc PC nối AP của đèn; chọn giữ kết nối nếu hệ điều hành báo không có Internet.
2. Mở **http://192.168.4.1**; dùng HTTP, không dùng HTTPS.
3. Mở Cài đặt & kết nối, nhập WiFi nhà 2,4 GHz và mật khẩu rồi bấm Lưu & kết nối. Mật khẩu đã lưu không được gửi lại cho trình duyệt.
4. Sau khoảng 1–2 giây, đèn thử kết nối. Nếu vẫn ở AP, bạn có thể xem IP WiFi trong trang. Trên WiFi nhà, mở IP đó hoặc `http://den-deco-xxxxxx.local` nếu Windows/điện thoại hỗ trợ mDNS.
5. Khi không kết nối được sau 30 giây, AP dự phòng mở. Cảm ứng và hiệu ứng vẫn hoạt động trong quá trình thử kết nối.

AP đã mở sẽ được giữ đến lần khởi động lại để tránh ngắt phiên cấu hình. Khi khởi động lại và WiFi nhà kết nối bình thường trong 30 giây, AP không mở. Có thể xem IP trong Serial Monitor hoặc danh sách DHCP của router. IP phụ thuộc router; đặt DHCP reservation theo MAC nếu muốn ổn định.

Đổi tên đèn chỉ đổi nhãn hiển thị; AP và hostname dùng mã thiết bị để nhiều bộ Đèn Deco không trùng tên mạng. Để trống mật khẩu khi nhập cùng SSID sẽ giữ mật khẩu đang lưu; chọn Mạng không có mật khẩu khi dùng mạng mở.

## 5. Sử dụng bốn nút

| Nút | Chạm ngắn, nhận khi thả | Giữ |
| --- | --- | --- |
| Nguồn | Bật/tắt | 2 giây: bật chế độ ngủ |
| − | Giảm 5 điểm sáng | Sau 0,6 giây, giảm 1 điểm mỗi 50 ms |
| + | Tăng 5 điểm sáng | Sau 0,6 giây, tăng 1 điểm mỗi 50 ms |
| M | Sang chế độ kế tiếp | 2 giây: hẹn 30 phút hoặc hủy hẹn giờ đang chạy |

Giữ rồi thả không phát thêm chạm ngắn. Nhiều nút cùng lúc sẽ hủy phần thao tác tiếp theo và chờ thả hết; lệnh giữ đã thực hiện trước đó không thể thu hồi. Sau cấp nguồn, firmware bỏ qua cảm ứng ít nhất 1 giây và chờ tất cả nút được thả.

Khi tắt, +/−/M được bỏ qua. Giữ Nguồn vẫn bật chế độ ngủ. Nút tăng/giảm dừng ở 1 và 100, không tự tắt.

Thứ tự M: Ngủ → Một màu → Chuyển màu chậm → Ánh nến → Cầu vồng chậm → Ánh sáng dâng lên → Ngủ.

Màu ngủ là RGB (255,96,24). Lần đầu mức ngủ 5%; khi chỉnh sáng trong chế độ ngủ, đèn nhớ riêng mức này. Lần vào chế độ ngủ tiếp theo dùng mức đã nhớ. Sang chế độ khác giữ mức sáng hiện tại; không có bản nhớ riêng cho từng chế độ.

## 6. Web, độ sáng và hẹn giờ

Web và cảm ứng dùng cùng trạng thái. Chỉnh ở một nơi sẽ cập nhật những trình duyệt khác bằng polling 250 ms khi tab đang hoạt động. Hai lệnh gần nhau áp dụng theo thứ tự ESP xử lý.

- Khi tắt, chỉnh sáng/màu/chế độ hoặc áp dụng mục yêu thích không tự bật. Nút Chế độ ngủ là hành động bật vào ngủ giống giữ Nguồn.
- Chọn màu chuyển sang Một màu. Tốc độ 1–100 dùng cho Chuyển màu, Cầu vồng và Dâng lên.
- Ba mục yêu thích lưu màu, chế độ, độ sáng và tốc độ; không lưu cờ bật/tắt hoặc hẹn giờ.
- Hẹn giờ 1–180 phút, chỉ khi đang bật. Năm phút cuối giảm sáng; thời lượng ngắn hơn 5 phút sẽ giảm từ đầu.
- Đổi thật độ sáng, màu, chế độ, tốc độ, trần hoặc áp dụng cấu hình khác sẽ hủy hẹn giờ. Sửa mục yêu thích, đổi tên/mạng hoặc gửi lại cùng giá trị không hủy.
- Khi mất kết nối, web khóa điều khiển. Kết nối lại chỉ đọc trạng thái; không tự gửi lại thao tác cũ.

Thanh sáng 1–100 là thang cảm nhận, được ánh xạ gamma 2,2. Trần ban đầu 30% mã đầu ra; trang cài đặt cho chỉnh 3–30%. Mức thấp nhất dùng `6/255` để màu cam ngủ còn các kênh màu ở mức thấp; đây là thông số khởi đầu cần thử trong vỏ. Nhiều mức thấp có thể trùng mã do LED chỉ có 8 bit mỗi kênh. Không dùng dithering ở bản này.

Đổi thông số hiệu chỉnh trong `Config.h` nếu cần, rồi biên dịch lại. Không tăng trần kỹ thuật trước khi đo dòng, điện áp và nhiệt đường cấp nguồn. Firmware không có phép đo dòng hay nhiệt tự động.

## 7. Lưu trạng thái

Sau thay đổi thực sự, đèn áp dụng ngay rồi chờ **10 giây từ thay đổi cuối** để lưu. Web hiển thị Đang chờ lưu, Đã lưu sau khi ghi/đọc kiểm tra thành công, hoặc lỗi lưu. Chưa có bản lưu sẽ hiển thị Cấu hình mặc định.

Lưu: bật/tắt, RGB, chế độ, độ sáng, tốc độ, mức ngủ riêng, trần đầu ra và ba mục yêu thích. Tên đèn, mạng và mật khẩu AP lưu riêng khi bấm áp dụng. Không lưu pha hiệu ứng, khung hình hoặc thời gian hẹn giờ còn lại.

Rút nguồn trước 10 giây sẽ mất phần chỉnh chưa lưu. Hiệu ứng, chuyển sáng và hẹn giờ giảm sáng không ghi flash mỗi khung hình. Hết hẹn giờ tạo thay đổi tắt, rồi vẫn chờ 10 giây để lưu.

Firmware có hai bản NVS, CRC và số thế hệ; boot chọn bản hợp lệ mới nhất. Khi ghi lỗi, giữ RAM và thử lại sau 5 giây. Khi gặp schema mới hơn, không tự ghi đè bằng firmware cũ. OTA cùng schema giữ dữ liệu; thay đổi firmware tương lai cần quy tắc migration nếu đổi schema, gamma hoặc cách hiểu thông số.

## 8. Cập nhật OTA

Lần nạp đầu tiên vẫn qua USB để có firmware và bảng phân vùng OTA. Sau đó dùng web qua WiFi nhà hoặc AP:

1. Trong Arduino IDE, giữ nguyên board 4 MB và Partition Scheme có OTA; chọn Sketch → Export Compiled Binary.
2. Tìm file ứng dụng **DenDeco.ino.bin** hoặc tên tương đương do IDE xuất. Không chọn `.bootloader.bin`, `.partitions.bin`, `.merged.bin` hoặc file dùng cho ESP32/S2/S3 khác. Gói bàn giao cũng có `firmware/DenDeco-1.0.0-ota.bin`.
3. Mở Cập nhật firmware qua WiFi trên web, chọn file, bấm Cập nhật firmware và xác nhận.
4. Firmware lưu ngay các thay đổi đang chờ trước khi bắt đầu OTA, thay vì chờ 10 giây trong phiên bảo trì này. Khi lưu không thành công hoặc schema không hỗ trợ, OTA chưa bắt đầu.
5. Giữ nguồn và kết nối ổn định. Ánh sáng giữ khung hình hiện tại; cảm ứng, hiệu ứng và xử lý hẹn giờ tạm dừng trong lúc upload; bộ đếm thời gian chạy vẫn trôi. Không dùng điều khiển bình thường cho tới khi hoàn tất.
6. File được ghi vào phân vùng ứng dụng còn lại, kiểm tra kích thước, header/chip ESP32-C3, dấu ứng dụng và kiểm tra ảnh của thư viện Update. Khi hợp lệ, đèn trả thành công rồi khởi động lại sau khoảng 1,8 giây. Trang thử đồng bộ lại sau khoảng 7 giây.
7. Đèn khôi phục trạng thái đã lưu và WiFi. Hẹn giờ không tiếp tục sau khởi động, giống quy tắc mất điện.

File của bản này nhỏ hơn giới hạn 1.310.720 byte mỗi phân vùng. Nếu dùng sơ đồ phân vùng khác, giới hạn trên trang lấy từ phân vùng thực tế. Đổi bảng phân vùng phải nạp USB; OTA chỉ thay ứng dụng.

Nếu upload bị ngắt hoặc file bị từ chối trước khi hoàn tất, ứng dụng đang chạy vẫn được giữ. Nếu mất điện sau khi đã ghi firmware hợp lệ và chọn boot, lần bật sau có thể chạy firmware mới. Nếu firmware mới hợp lệ về định dạng nhưng có lỗi chạy, bản này chưa có rollback tự động sau boot; khôi phục bằng USB. Phiên OTA kiểm tra token và cùng origin; dùng trong mạng nội bộ, không mở cổng điều khiển ra Internet. Bản này chưa xác minh chữ ký số firmware.

## 9. Thử lần đầu và chẩn đoán

1. Nạp với nguồn LED tháo ra khi cần tránh tải trên USB PC.
2. Có thể nạp `LED_Check/LED_Check.ino` với cùng cấu hình IDE để thử dây ở mức thấp: toàn bộ LED lần lượt đỏ/xanh lá/xanh dương, sau đó LED 0/9/10/19 sáng riêng, rồi từng cặp đi lên. Serial Monitor báo bước thử và OUT từng cảm ứng. Sketch này không sửa NVS; sau thử nạp lại `DenDeco/DenDeco.ino`. Hoặc sau đấu đúng và dùng nguồn hằng ngày, bật Một màu ở mức thấp để kiểm tra các kênh. Nếu xanh/đỏ đổi chỗ, sửa `NEO_GRB` ở sketch theo dây thực tế rồi nạp lại.
3. Thử Dâng lên: hai bên phải cùng đi từ đáy lên. Nếu ngược một bên, kiểm tra chiều nối/chỉ số.
4. Thử từng nút, chạm 20 lần, giữ và chạm đồng thời qua vỏ.
5. Kiểm tra chỉnh → chờ lưu → reset, và chỉnh → rút nguồn sớm → bật lại.
6. Nhập sai WiFi để kiểm tra AP dự phòng; dùng AP cập nhật lại thông tin.
7. Chạy trắng ở trần 30% và tải WiFi trong 30 phút; ghi điện áp, dòng và nhiệt. Dừng thử nếu có reset, sai màu hoặc nhiệt bất thường để xử lý đường nguồn trước khi dùng hàng ngày.
8. Thử OTA với file gói bàn giao, rồi kiểm tra lại phiên bản và cấu hình.

Không thấy AP: đợi 30 giây nếu đã lưu mạng; nếu WiFi nhà đang kết nối, xem IP router/Serial. Không hiện mật khẩu AP: mở Serial Monitor trước rồi bấm RESET. Đèn chạm tự bật: kiểm tra module direct active high, dây và vùng cảm ứng/vỏ. OTA báo file sai: chọn đúng ứng dụng `.ino.bin`, không dùng bootloader/merged.

Bộ bàn giao đã kiểm tra biên dịch, logic và giao diện bằng mô phỏng. Các kiểm tra điện, độ nhạy, nhiệt, truyền dữ liệu và OTA thật cần thực hiện trên thiết bị của bạn; xem `KIEM_THU.md` để ghi kết quả.
