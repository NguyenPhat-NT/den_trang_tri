# Kiểm thử và nghiệm thu Đèn Deco 1.3.0

## Đã thực hiện trên phần mềm

Các kiểm tra nền dưới đây đã chạy thành công cho mã nguồn 1.1.0; phần 1.2.0/1.2.1 giữ kết quả lịch sử riêng. Thông tin build và kết quả chạy mới cho 1.3.0 ghi trong `BUILD_INFO.md` và phần bổ sung bên dưới; không coi kết quả cũ là kết quả bản mới. Người dùng đã thử 1.2.0 trên đèn thật, có phản hồi chuyển màu/Lấp lánh. **SNTP, lịch tuần và tự động mặt trời của bản 1.3.0 cần thử trên bo thật**; host/web mock không xác nhận Internet SNTP, đồng hồ ESP hoặc ramp thực tế.

- Sketch chẩn đoán LED_Check cũng đã build thành công cho cùng ESP32-C3/core.
- Build thực tế ESP32-C3: PlatformIO espressif32 6.12.0, Arduino ESP32 2.0.17, NeoPixel 1.12.5, ArduinoJson 7.4.2; USB CDC bật và phân vùng OTA 4 MB.
- Build thêm bằng Arduino CLI với đúng ESP32 3.3.1, NeoPixel 1.15.5 và ArduinoJson 7.4.3 người dùng đang có; SUCCESS. File OTA 1.1.0 dùng bộ phiên bản này.
- Kiểm tra Core.h bằng g++ với AddressSanitizer/UndefinedBehaviorSanitizer: bật/tắt giữ cấu hình, nhớ mức ngủ riêng, lệnh sai phạm vi, timer qua tràn millis, hủy timer/no-op, chạm/giữ/thả, nhiều nút và chạm đang giữ lúc boot.
- Kiểm tra renderer: đường sáng tăng đơn điệu, đầu ra không vượt cap, tắt về RGB 0, giữ màu ngủ ở mức thấp và ghép LED cùng chiều cao. Hủy timer giữa giảm sáng khôi phục mềm từ mức đang hiển thị.
- Kiểm tra Storage.h với ArduinoJson thật và Preferences mô phỏng: không ghi trước 10 giây, tính lại từ thay đổi cuối, khôi phục bản mới nhất, retry sau lỗi ghi, ghi dở một slot vẫn đọc slot hợp lệ, quay lại bản đã lưu bỏ ghi thừa, schema mới không bị ghi đè và CRC mạng.
- Kiểm tra web bằng Playwright/Chromium với API mô phỏng ở 390×844 và 1280×900: bật/tắt, sáng, màu, favorite, timer, giữ thông tin WiFi đang nhập khi polling, tên đèn, mất kết nối/khôi phục không phát lại lệnh, request OTA có token/kích thước và không có lỗi JavaScript. Không tràn ngang màn hình điện thoại.
- Kiểm tra file ứng dụng tạo ra có magic ảnh, chip ESP32-C3, dấu ứng dụng và nằm trong giới hạn phân vùng OTA; file đã có SHA-256 trong gói.

Các kiểm tra trên không thay cho flash/nạp USB, truyền OTA vào ESP, đo phản hồi mạng hoặc đo điện của thiết bị thật. Web mock kiểm tra giao diện và request, không mô phỏng stack WiFi, flash hoặc điện.

## Kiểm tra bổ sung bản 1.1.0

- PASS kiểm tra lõi: giữ đúng +/− 5 giây, chỉ phát reset một lần, không phát thêm chạm +/− khi thả; thả sớm/chạm nút thứ ba hủy; chờ thả hết sau boot/reset; thời gian giữ qua tràn millis.
- PASS kiểm tra chính sách mạng: chờ IP, thời hạn 30 giây, AP dự phòng, chu kỳ thử lại, hoãn thử lại khi đang quét, khôi phục và tràn millis; kiểm tra định dạng hostname.
- PASS kiểm tra lưu/migration mạng bằng Preferences mô phỏng: schema 1 giữ tên/SSID/mật khẩu, schema 2 lưu hostname và bỏ mật khẩu AP; CRC, mạng mở, từ chối hostname sai và bảo vệ schema tương lai.
- PASS web mock: quét/đọc kết quả bất đồng bộ, chọn SSID/bảo mật, lỗi/mất kết nối, giữ ô đang nhập khi polling và khi phản hồi lưu đến muộn, lưu hostname và sao chép trên HTTP, retry cùng SSID, không phát lại lệnh.
- Firmware thực: quét 2,4 GHz, lấy IP DHCP, AP tắt ngay khi STA nhận IP, mở AP sau 30 giây thất bại/mất mạng rồi tắt khi khôi phục. Những mục này cần ESP và router thật; mock không xác nhận được.
- mDNS trong LAN và nhiều đèn cùng mạng: địa chỉ mặc định, tên tự đặt, truy cập IP dự phòng và xử lý trùng tên theo địa chỉ thực tế firmware trả về. Tên trùng phải thêm mã bo sáu ký tự viết thường, rồi `-2`, `-3`… nếu còn trùng, đồng thời giữ nguyên tên người dùng đã lưu.

## Kiểm tra bổ sung bản 1.2.0

Các kiểm tra phần mềm dưới đây đã chạy thành công cho bản mới; thử nghiệm trên bo ở bảng E01–E08 vẫn cần người dùng thực hiện.

- PASS lõi/render (ASan/UBSan): đủ 14 mã đúng thứ tự và mục yêu thích, tint giữ chế độ/hủy timer đúng, mọi mode có trần/tắt/fade/timer; tám hiệu ứng mới khác nhau, dùng tốc độ, có độ liên tục tại 33 ms và uptime dài, đối xứng hai nhánh LED.
- PASS lưu/migration ánh sáng: đọc schema 1 và 2, ghi schema 2, giữ toàn bộ cấu hình/mục yêu thích ở sáu mã cũ; lưu/khôi phục mode và mục yêu thích mới; chọn slot qua tràn số thế hệ; ghi dở schema mới vẫn đọc slot cũ; bảo vệ schema tương lai. Lưu mạng không đổi và các kiểm tra mạng cũng PASS.
- PASS Playwright/Chromium với API mô phỏng: đủ 14 ID/tên/mô tả/tốc độ; đổi RGB giữ ba hiệu ứng tint, palette riêng chuyển Một màu; tương tác mode/màu khi phản hồi chậm; mục hướng dẫn dùng được qua bàn phím và offline, mô tả đúng bật/tắt đã lưu. Các kiểm tra cũ cho WiFi, draft, timer và OTA cũng PASS.
- SUCCESS build ESP32-C3 với core 3.3.1, NeoPixel 1.15.5 và ArduinoJson 7.4.3; profile PlatformIO 2.0.17 cũng SUCCESS. Ảnh ứng dụng, giới hạn phân vùng và SHA-256 ghi trong `BUILD_INFO.md`.

## Chạy thử bổ sung bản 1.2.0 trên đèn

Người dùng đã thử bản 1.2.0 và phản hồi về chuyển màu/Lấp lánh; chưa có kết quả riêng từng ca E01–E08 để ghi Đạt.

| Mã | Kiểm tra | Cần đạt | Kết quả thực tế |
| --- | --- | --- | --- |
| E01 | Chọn tám hiệu ứng mới trên web, đi vòng bằng M | Đúng tên/mã 6–13; M đi đủ 14 chế độ rồi về Ngủ, ánh sáng hiển thị qua vỏ ổn định | Chưa ghi kết quả từng ca |
| E02 | Tốc độ 1/50/100 ở từng hiệu ứng mới | Tăng tốc độ thấy hiệu ứng chạy nhanh hơn; giữ mode, màu và độ sáng đang chọn | Chưa ghi kết quả từng ca |
| E03 | Đổi RGB ở Thở nhẹ, Lấp lánh và Sao băng | Hiệu ứng giữ nguyên, dùng màu mới; các trình duyệt đồng bộ đúng | Chưa ghi kết quả từng ca |
| E04 | Đổi RGB ở các hiệu ứng dùng bảng màu riêng | Chuyển sang Một màu; chọn lại hiệu ứng trở về bảng màu của hiệu ứng | Chưa ghi kết quả từng ca |
| E05 | Lưu/áp dụng mục yêu thích chứa hiệu ứng mới, chờ lưu rồi reboot | Khôi phục đúng mode, RGB, độ sáng và tốc độ; không đổi cờ bật/tắt ngoài quy tắc hiện có | Chưa ghi kết quả từng ca |
| E06 | OTA 1.1.0 → 1.2.0 với cấu hình và ba mục yêu thích cũ | Giữ mã chế độ cũ, cấu hình đèn, WiFi, tên và hostname sau reboot/lưu mới | Chưa ghi kết quả từng ca |
| E07 | Đọc Hướng dẫn sử dụng trên điện thoại và PC | Mở được ngay trong web, thao tác và bảng chế độ đúng; nội dung đọc được, không tràn ngang | Chưa ghi kết quả từng ca |
| E08 | Bật/tắt, chỉnh sáng và hẹn giờ khi chạy hiệu ứng mới | Trần và độ sáng áp dụng; tắt RGB 0; giảm/tắt theo timer và hủy timer đúng quy tắc | Chưa ghi kết quả từng ca |

## Kiểm tra bổ sung bản 1.2.1

Các kiểm tra phần mềm bên dưới đã đạt cho bản 1.2.1; cảm nhận trên đèn/vỏ thật vẫn cần người dùng kiểm tra ở bảng F01–F04.

- PASS Core (C++11, ASan/UBSan): khung mục tiêu 16 ms, phân bố làm tròn cố định ở mã 2/4/8/9/10/11/13; giữ đúng trần, Ngủ/Một màu đồng màu và ngưỡng làm tròn đối xứng giữa hai nhánh. Kiểm tra Chuyển màu ở mức sáng 5% có bước màu tổng hợp tối đa 0,1 mã/kênh/16 ms trên chuỗi LED mô phỏng. Mẫu làm tròn không đổi theo thời gian nên không tạo PWM phần mềm.
- PASS pha/tốc độ: đổi tốc độ giữ pha và vị trí cụm, chu kỳ/uptime dài không làm mất độ chính xác chuyển động; Chuyển màu chậm/Cầu vồng có chu kỳ nhanh nhất 8/12 giây. Các kiểm tra fade, khung đầu chuyển tiếp, bật/tắt và timer vẫn đạt; sai số làm tròn ở mức timer 50% không quá một mã.
- PASS Lấp lánh: cụm RGB thưa chỉ 1–3 tầng đang sáng, các vị trí còn lại gần đen; lên/xuống mềm, không vượt độ sáng/trần, tint giữ chế độ. Mô hình tán sáng Gaussian cho thấy tương phản tốt hơn bản cũ, nhưng không thay cho kiểm tra vỏ thật; tổng ánh sáng của chế độ này giảm vì ít LED sáng cùng lúc.
- PASS web: mô tả cụm sáng/nền tối và lời khuyên chỉnh theo phòng/vỏ đúng; kiểm tra bàn phím/layout/hướng dẫn và các điều khiển cũ vẫn đạt. Giữ 14 ID, trường cấu hình, schema 2, mục yêu thích và WiFi/AP; lưu trữ không sửa ở bản này. Build ESP32-C3 dùng core/thư viện hiện tại; thông tin và SHA ở `BUILD_INFO.md`.

## Chạy thử bổ sung bản 1.2.1 trên đèn

| Mã | Kiểm tra | Cần đạt / cần ghi nhận | Kết quả thực tế |
| --- | --- | --- | --- |
| F01 | Các hiệu ứng tự chuyển màu mã 2/4/8/9/10/11/13; sáng 5/50/100 và tốc độ 1/50/100 | So sánh bước màu qua đúng vỏ với 1.2.0, ghi mức còn thấy bước và hai nhánh cùng độ cao; không suy ra mượt từ FPS | Chưa thử 1.2.1 |
| F02 | Đổi tốc độ khi hiệu ứng đang chạy, theo dõi qua vòng chu kỳ | Pha không nhảy lúc đổi tốc độ; thay đổi tốc độ thấy được; ghi nếu còn giật qua chu kỳ | Chưa thử 1.2.1 |
| F03 | Lấp lánh qua vỏ dày với vài màu RGB, sáng/tốc độ khác nhau | Cụm xuất hiện/tắt dần thấy được trên nền gần đen, giữ màu đã chọn; ghi độ tương phản so với 1.2.0 | Chưa thử 1.2.1 |
| F04 | OTA 1.2.0 → 1.2.1, reboot và áp dụng mục yêu thích | Giữ trạng thái bật/tắt, 14 mã chế độ, RGB, sáng/tốc độ, mục yêu thích, WiFi/tên/hostname; thử bật/tắt bình thường | Chưa thử 1.2.1 |

## Kiểm tra bổ sung bản 1.3.0

Các bộ kiểm tra C++11 với ASan/UBSan, cảnh báo nghiêm ngặt và web Playwright/Chromium đã chạy **PASS** cho bản 1.3.0. Arduino 3.3.1 và PlatformIO/core 2.0.17 đều build **SUCCESS**; chi tiết trong `BUILD_INFO.md`. Nội dung đã kiểm tra trên host:

- PASS Lịch tuần: tám dòng, ngày/giờ UTC+7, tham chiếu mục yêu thích lúc chạy; chọn ON/OFF trùng phút đúng ưu tiên và chỉ áp dụng một kết quả. OFF tuần thắng, ON tuần cuối thắng mặt trời, dusk thắng dawn khi chỉ có hai mặt trời. Mục trống/hỏng bỏ qua an toàn.
- PASS Mô hình đồng hồ: trạng thái chưa đồng bộ, phút đầu sau sync không chạy, holdover sau mất WiFi, tràn thời gian; không chạy bù/replay sau boot/sửa kế hoạch/nhảy giờ/OTA. Callback SNTP định kỳ bình thường không phát lại sự kiện cũ.
- PASS Mặt trời: TP.HCM 10.776/106.7, giới hạn tọa độ/offset/ngày, sự kiện mọc/lặn có hoặc không hợp lệ; dawn bắt đầu tại giờ mọc cộng offset, ramp 0→preset/mục yêu thích theo 1–120 phút; dusk độc lập. Preset ấm RGB (255,160,72), 35% và trần đầu ra áp dụng.
- PASS Điều khiển/renderer: thao tác ánh sáng tay dừng ramp, lịch tương lai giữ; sự kiện tự động áp dụng hủy countdown; không đổi 14 mode/tint/hiệu ứng Hoàng hôn mã 11.
- PASS Lưu kế hoạch NVS riêng: CRC/validation theo bản triển khai, giữ qua reset/OTA, không đổi schema ánh sáng 2 hoặc cấu hình mạng. Web giữ draft khi polling/saving, hiển thị giờ/lịch/mặt trời; kiểm tra bàn phím và layout điện thoại.

PASS bổ sung baseline lưu bảo trì khi lần đầu/mặc định và lỗi mở/ghi/readback; tắt trong đoạn đầu Bình minh không tăng sáng thoáng qua. Rà soát luồng OTA thất bại lấy lại mốc phút tại điểm tiếp tục vòng lặp.

Các thử host chỉ xác nhận thuật toán/mô hình và tương tác web. Cần chạy riêng trên ESP/router để xác nhận đường SNTP thực, thời gian tiếp tục khi mất WiFi và sự kiện/ramp phát ánh sáng.

## Chạy thử bổ sung bản 1.3.0 trên đèn

| Mã | Kiểm tra | Cần đạt / cần ghi nhận | Kết quả thực tế |
| --- | --- | --- | --- |
| A01 | Nạp/boot lần đầu chỉ có AP, chưa có Internet | Web/cảm ứng hoạt động; giờ/lịch chờ đồng bộ, không tự bật theo giờ chưa xác thực | Chưa thử 1.3.0 |
| A02 | Nối WiFi nhà có Internet và chờ SNTP | Trang hiển thị ngày/giờ UTC+7 đúng; phút đồng bộ đầu không chạy sự kiện | Chưa thử 1.3.0 |
| A03 | Lịch ON hôm nay ở phút kế tiếp, OFF một phút sau | Dùng favorite đã lưu, đúng phút/đúng bật tắt; lưu trước phút chạy, không chỉnh giả giờ | Chưa thử 1.3.0 |
| A04 | Sửa nội dung favorite sau lưu dòng ON; thử favorite trống | ON dùng nội dung mới lúc chạy; mục trống/hỏng bỏ qua an toàn | Chưa thử 1.3.0 |
| A05 | Sau đồng bộ đặt lịch tương lai, tắt/bật router | Đồng hồ tiếp tục và lịch chạy khi mất WiFi; khôi phục SNTP không replay | Chưa thử 1.3.0 |
| A06 | Reboot không Internet, sửa lịch trùng phút hiện tại, lỡ giờ trong OTA | Chờ sync sau boot, không chạy bù/phát lại; thử tiếp bằng phút tương lai | Chưa thử 1.3.0 |
| A07 | TP.HCM, xem giờ mọc/lặn và chỉnh offset/ngày | Giờ tính hợp lý theo ngày, UTC+7; độc lập ngày/offset mỗi tự động, tọa độ lưu đúng | Chưa thử 1.3.0 |
| A08 | Dawn màu ấm, ramp 1 phút, offset đưa vào phút tương lai hợp lệ | Bắt đầu tại giờ mọc+offset, từ 0 lên RGB 255/160/72 ở mức 35%; ghi độ mượt thực, không giả đồng hồ | Chưa thử 1.3.0 |
| A09 | Dusk chọn màu ấm rồi favorite tại giờ lặn+offset | Bật đúng preset/mục yêu thích; không tự chọn hiệu ứng Hoàng hôn mã 11 | Chưa thử 1.3.0 |
| A10 | Thao tác tay khi dawn đang tăng; đặt countdown trước sự kiện tự động | Tay dừng ramp, lịch tương lai vẫn chạy; sự kiện tự động áp dụng hủy countdown | Chưa thử 1.3.0 |
| A11 | Cùng phút nhiều ON/OFF, tuần+mặt trời, dawn+dusk | OFF thắng; không OFF thì ON tuần cuối thắng; chỉ mặt trời thì dusk thắng; không bật/tắt trung gian | Chưa thử 1.3.0 |
| A12 | Draft kế hoạch đang sửa khi polling, lưu rồi reset/OTA | Draft không bị ghi đè; kế hoạch đã lưu/tọa độ giữ, ánh sáng/favorites/WiFi/tên cũng giữ | Chưa thử 1.3.0 |

Với A08/A09, chỉ dùng offset ±120 phút khi giờ hiện tại gần sự kiện mặt trời; nếu không, thử vào ngày/giờ thích hợp. Không giả đồng hồ để biến kiểm tra thuật toán thành kết quả phần cứng. Bảo vệ khỏi chạy bù sau clock jump được kiểm tra trên host; kết quả SNTP/ramp thật ghi riêng.

## Bảng chạy thử trên thiết bị

Ghi Ngày / Kết quả / Ghi chú cho từng dòng. Các ca nền T/N/O giữ lại để bổ sung kết quả cụ thể; trạng thái chưa ghi kết quả không phủ nhận việc người dùng đã lắp và dùng đèn. Mục tiêu về thời gian cần đo trên mạng thực tế.

| Mã | Kiểm tra | Cần đạt | Kết quả thực tế |
| --- | --- | --- | --- |
| T01 | Nạp LED_Check để sáng riêng LED 0/9/10/19 | Đúng đáy/đỉnh theo mapping | Chưa ghi kết quả từng ca |
| T02 | Dâng lên | Hai đoạn cùng đi từ đáy lên | Chưa ghi kết quả từng ca |
| T03 | Mỗi nút 20 chạm qua vỏ đã lắp | Một chạm/một lệnh, không chạm giả nút cạnh; phải thử vật lý, không thay bằng mock | Chưa ghi kết quả từng ca |
| T04 | Giữ Nguồn/M rồi thả | Một lệnh giữ, không thêm chạm ngắn | Chưa ghi kết quả từng ca |
| T05 | Giữ +/− tới hai biên | Dừng 1/100 và dừng khi thả | Chưa ghi kết quả từng ca |
| T06 | Khi tắt chạm +/−/M | Không tự bật; Nguồn bật cấu hình cũ | Chưa ghi kết quả từng ca |
| T07 | Hai trình duyệt và cảm ứng | Cùng trạng thái; mục tiêu ≤500 ms ở LAN ổn định | Chưa ghi kết quả từng ca |
| T08 | Chỉnh giây 0 và giây 6 | Chỉ lưu khoảng giây 16 | Chưa ghi kết quả từng ca |
| T09 | Rút nguồn trước/sau mốc lưu | Về bản cũ/bản mới tương ứng | Chưa ghi kết quả từng ca |
| T10 | Chạy hiệu ứng không thao tác | Bộ đếm writes không tăng theo frame | Chưa ghi kết quả từng ca |
| T11 | Hẹn/hủy/hết giờ | Giảm sáng và tắt; timer không khôi phục qua reset | Chưa ghi kết quả từng ca |
| T12 | Tắt router/nhập sai WiFi | AP không mật khẩu mở sau 30 giây, đèn vẫn điều khiển cảm ứng được | Chưa ghi kết quả từng ca |
| T13 | Tải trắng cap 30% và WiFi 30 phút | Ghi dòng/áp/nhiệt; không reset hoặc sai màu | Chưa ghi kết quả từng ca |
| T14 | Tắt, đợi lưu, nạp USB giữ NVS | Sau boot vẫn tắt, cảm ứng bật được | Chưa ghi kết quả từng ca |
| T15 | Quét sáng 1–100 | Không giảm hoặc vượt cap; cho phép mức trùng | Chưa ghi kết quả từng ca |
| T16 | Ngủ, giữ sáng và fade trong phòng tối | Ghi màu/bước nhảy/nhấp nháy để hiệu chỉnh | Chưa ghi kết quả từng ca |
| N01 | Chưa nối bốn module cảm ứng | Chân INPUT_PULLDOWN không tự sinh lệnh sau boot | Chưa ghi kết quả từng ca |
| N02 | Lần đầu hoặc xóa WiFi | AP DenDeco-XXXXXX mở không mật khẩu; web http://192.168.4.1 dùng được | Chưa ghi kết quả từng ca |
| N03 | Quét/chọn mạng 2,4 GHz | Hiển thị đang quét/kết quả/lỗi; chọn điền SSID; cảm ứng và hiệu ứng tiếp tục | Chưa ghi kết quả từng ca |
| N04 | Mạng ẩn, mạng mở, mật khẩu sai | Nhập SSID thủ công; mạng mở không mật khẩu; sai mật khẩu có trạng thái và AP dự phòng | Chưa ghi kết quả từng ca |
| N05 | Lưu WiFi đúng khi đang dùng AP | Khi nhận IP DHCP, AP tắt ngay; chuyển về WiFi nhà mở được web | Chưa ghi kết quả từng ca |
| N06 | Router tắt rồi bật lại | AP mở sau 30 giây mất mạng; tự nối lại và AP tắt khi nhận IP, không cần reboot | Chưa ghi kết quả từng ca |
| N07 | http://den-deco.local và đổi hostname | Cùng LAN mở được tên hỗ trợ mDNS; địa chỉ mới đúng; IP dự phòng truy cập được | Chưa ghi kết quả từng ca |
| N08 | Hai đèn trên cùng LAN | Đặt hostname riêng; địa chỉ thực tế mở đúng từng đèn, kiểm tra cả trường hợp trùng tên | Chưa ghi kết quả từng ca |
| N09 | Giữ đúng +/− 5 giây qua vỏ | Xóa chỉ WiFi nhà, mở AP; LED, favorites, tên đèn/hostname giữ nguyên; chỉ reset một lần | Chưa ghi kết quả từng ca |
| N10 | Giữ +/− dưới 5 giây, thêm Nguồn/M, giữ lúc boot | Không reset nhầm; thả hết rồi thao tác lại bình thường | Chưa ghi kết quả từng ca |
| N11 | Hai đèn cùng hostname; tạo thêm trùng tên với hậu tố | Tên thực tế thêm mã bo viết thường rồi số khi cần; web/Serial báo đúng; tên đã lưu giữ nguyên, mở đúng từng đèn | Chưa ghi kết quả từng ca |
| O01 | OTA đúng file qua WiFi nhà | Thành công, reboot, đúng phiên bản, giữ cấu hình/WiFi | Chưa ghi kết quả từng ca |
| O02 | OTA qua AP | Thành công; sau reboot AP/STA đúng cấu hình; nếu nhận IP WiFi nhà, AP tắt | Chưa ghi kết quả từng ca |
| O03 | File sai chip/bootloader/merged/quá lớn | Từ chối; firmware hiện tại còn hoạt động | Chưa ghi kết quả từng ca |
| O04 | Ngắt upload giữa chừng | Ứng dụng đang chạy được giữ, điều khiển hoạt động lại | Chưa ghi kết quả từng ca |
| O05 | Chỉnh chưa đủ 10 giây rồi OTA | Lưu trước cập nhật, boot phục hồi cấu hình đó | Chưa ghi kết quả từng ca |
| O06 | Nạp USB phục hồi | Khôi phục được khi firmware mới có lỗi chạy | Chưa ghi kết quả từng ca |

## Cách chạy lại kiểm tra phần mềm

Các lệnh dưới đây dành cho Linux/môi trường phát triển, không bắt buộc để sử dụng Arduino IDE trên Windows.

```sh
pio run -e esp32c3
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined tests/core_test.cpp -o core-test
./core-test
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined tests/network_test.cpp -o network-test
./network-test
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -Itests/stubs -I.pio/libdeps/esp32c3/ArduinoJson/src tests/storage_test.cpp -o storage-test
./storage-test
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined tests/schedule_test.cpp -o schedule-test
./schedule-test
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined tests/solar_test.cpp -o solar-test
./solar-test
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined tests/automation_test.cpp -o automation-test
./automation-test
c++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined -Itests/stubs -I.pio/libdeps/esp32c3/ArduinoJson/src tests/schedule_storage_test.cpp -o schedule-storage-test
./schedule-storage-test
node tests/web_test.cjs
```

Kiểm tra web cần package Playwright và Chromium. Đặt biến `DECO_BROWSER` bằng đường dẫn executable nếu Chromium không ở `/usr/bin/chromium`. Ảnh chụp web từ mock chỉ minh họa UI, không là bằng chứng thiết bị đã chạy.

Người dùng đã lắp và thử 1.2.0, có phản hồi chuyển màu/Lấp lánh; bảng nền chưa có kết quả từng ca để đánh dấu Đạt. Bản 1.3.0 ưu tiên A01–A12 trên ESP/router thật, đồng thời giữ F01–F04 để ghi kết quả ánh sáng khi thử lại.
