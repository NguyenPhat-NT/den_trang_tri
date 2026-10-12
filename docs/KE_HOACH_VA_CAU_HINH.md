# Cấu hình và kế hoạch Đèn Deco 1.2.1

## Các quyết định đã chốt trong chat

Phạm vi: firmware, web trên ESP và hướng dẫn đấu nối/nạp. Người dùng tự xử lý cơ khí/vỏ. ESP32-C3 SuperMini, bốn TTP223, 20 WS2812B và nguồn 5 V–2 A. Tên Đèn Deco; WiFi nhà + AP dự phòng. Máy nạp Windows, Arduino IDE. Bổ sung OTA qua web theo yêu cầu mới.

Bản 1.2.1 xử lý phản hồi từ lần thử 1.2.0 trên đèn thật: bước chuyển màu còn lớn và Lấp lánh khó thấy do mật độ LED lớn. Mục tiêu khung hình khoảng 60/s với `FRAME_MS=16` thay cho 33 ms; bảy hiệu ứng chuyển màu mã 2/4/8/9/10/11/13 dùng làm tròn phân bố phần lẻ theo vị trí cố định giữa mười tầng LED, ngưỡng làm tròn đối xứng giữa hai nhánh. Dải màu Cầu vồng theo thứ tự LED giữ nguyên. Mỗi lựa chọn làm tròn chỉ khác một mã đầu ra 8 bit, không dùng dithering theo thời gian/PWM phần mềm; Ngủ và Một màu vẫn làm tròn tới mã gần nhất. Pha chạy liên tục khi đổi tốc độ và qua chu kỳ thời gian. Chu kỳ nhanh nhất Chuyển màu chậm/Cầu vồng tăng từ 1/5 giây lên 8/12 giây. Kết quả cảm nhận qua vỏ cần người dùng thử lại.

Lấp lánh ở 1.2.1 dùng màu RGB đã chọn cho cụm thưa khoảng 1–3 tầng chiều cao, xuất hiện/tắt dần trên nền gần đen; hệ số nền khoảng 0,5% thay cho 20% ở 1.2.0. Độ sáng/trần vẫn áp dụng cho nền và cụm. Không đổi 14 ID chế độ, quy tắc tint, mục yêu thích, schema ánh sáng 2 hay chính sách WiFi/AP. OTA 1.2.0 → 1.2.1 giữ cấu hình đã lưu.

Bản 1.2.0 giữ sáu chế độ cũ ở mã 0–5 và thêm tám chế độ theo đúng thứ tự: 6 Thở nhẹ, 7 Lấp lánh, 8 Cực quang, 9 Biển xanh, 10 Rừng xanh, 11 Hoàng hôn, 12 Sao băng, 13 Sóng màu. Nút M đi qua cả 14 chế độ rồi trở về Ngủ. Cả tám chế độ mới chỉnh được tốc độ; RGB giữ hiệu ứng ở Thở nhẹ, Lấp lánh và Sao băng. Một màu giữ hành vi RGB cũ; các chế độ còn lại chuyển sang Một màu khi chọn RGB. Web có mục Hướng dẫn sử dụng hiển thị trong trang điều khiển.

Nâng cấp OTA 1.1.0 → 1.2.0 giữ cấu hình đèn, các mục yêu thích cũ, WiFi và tên. Dữ liệu ánh sáng schema 2 đọc được schema 1/2, giữ nguyên mã chế độ cũ; firmware cũ không được ghi đè schema mới. Nếu cần quay về 1.1.0 sau khi lưu schema 2, dùng USB vì phiên bản cũ có thể không đọc được dữ liệu ánh sáng mới. WiFi/AP, thao tác cảm ứng và các thông số nguồn không đổi ở 1.2.0.

Bản 1.1.0 bổ sung quét/chọn WiFi 2,4 GHz bất đồng bộ và trạng thái kết nối trên web. Địa chỉ mặc định `http://den-deco.local`; tên truy cập chỉnh được riêng với nhãn tên đèn. Với nhiều đèn, đặt tên truy cập khác nhau và dùng địa chỉ thực tế hiển thị trên trang. Khi phát hiện trùng tên mDNS, tên thực tế được thêm mã bo sáu ký tự viết thường rồi `-2`, `-3`… nếu còn trùng; tên người dùng đã lưu không bị đổi. Kiểm tra định kỳ theo trạng thái mDNS SDK, nhưng thử trùng tên trên phần cứng còn chờ. Địa chỉ `.local` phụ thuộc mDNS trong mạng nội bộ; IP DHCP là cách truy cập dự phòng.

AP `DenDeco-XXXXXX` không có mật khẩu, chỉ mở khi chưa lưu WiFi, chưa kết nối được trong 30 giây hoặc xóa/reset WiFi để cấu hình lại. WiFi nhà kết nối thành công và nhận IP từ DHCP sẽ tắt AP ngay, kể cả phiên cấu hình đang dùng AP. Khi router mất kết nối, đèn thử nối lại, mở AP sau 30 giây chưa khôi phục và tắt AP khi nhận IP trở lại.

Giữ đúng hai nút + và − đồng thời liên tục 5 giây chỉ xóa SSID/mật khẩu WiFi nhà; giữ cấu hình ánh sáng, ba mục yêu thích, tên đèn và tên truy cập. Thả sớm hoặc thêm nút khác hủy reset. Bốn đầu vào cảm ứng dùng `INPUT_PULLDOWN` để chân chưa đấu không tự sinh lệnh; chạm/giữ và độ nhạy qua vỏ vẫn cần thử trên phần cứng.

Đặc tả Word là nguồn yêu cầu; thông tin được chốt cho phiên bản này là nguồn 5 V–2 A và 20 LED. Cấu hình bản cũ không thay thế các quyết định đã xác nhận cho bản này.

Chế độ ngủ nhớ riêng mức đã chỉnh, ban đầu 5%. Các mặc định giữ/nút, sáu chế độ gốc, timer và gamma lấy từ đề xuất trong tài liệu đã được người dùng đồng ý; tám hiệu ứng bổ sung thuộc bản 1.2.0. Mức tối thiểu hiệu chỉnh khởi đầu 6/255 để cam ngủ không chỉ còn kênh đỏ; hiệu chỉnh theo vỏ/đèn thật.

## Kế hoạch và trạng thái

| Giai đoạn | Kết quả | Trạng thái |
| --- | --- | --- |
| Chốt phạm vi, cấu hình và phụ thuộc | Arduino core/thư viện có phiên bản rõ ràng | Hoàn tất |
| Viết lõi điều khiển, cảm ứng, renderer và timer | Một nguồn trạng thái, xử lý theo millis không chờ WiFi | Hoàn tất và kiểm tra logic |
| NVS | Hai slot, CRC, kiểm tra ghi và retry; ánh sáng schema 2 đọc schema 1/2 | Bản 1.2.0 cập nhật; kết quả kiểm tra ghi ở KIEM_THU.md |
| Tám hiệu ứng | 14 mã chế độ, tốc độ/tint giữ nguyên; 1.2.1 sửa chuyển màu và Lấp lánh | Người dùng đã thử 1.2.0; cần thử lại 1.2.1 qua vỏ |
| Chuyển màu/Lấp lánh 1.2.1 | Khung 16 ms, làm tròn theo vị trí cố định, giữ pha và cụm thưa trên nền tối | Kết quả kiểm tra phần mềm ghi ở KIEM_THU.md; cảm nhận trên bo chờ thử lại |
| Hướng dẫn trong web | Thao tác cảm ứng, chế độ/màu/tốc độ, lưu, mạng và OTA | Bản 1.2.0 cập nhật; kiểm tra trên điện thoại sau nâng cấp |
| Web/WiFi | Web nhúng, polling 250 ms; quét 2,4 GHz, trạng thái mạng, hostname và AP dự phòng | Bản 1.1.0 cập nhật; kết quả kiểm tra ghi ở KIEM_THU.md |
| Reset WiFi từ cảm ứng | Giữ đúng +/− 5 giây, giữ cấu hình đèn và tên | Có từ bản 1.1.0; chưa có kết quả riêng từng ca qua vỏ |
| OTA | File ứng dụng C3, phân vùng còn lại, giữ NVS, reboot sau thành công | Kiểm tra nâng cấp 1.2.0 → 1.2.1 giữ cấu hình |
| Bàn giao Windows | Sketch, .bin OTA 1.2.1, hướng dẫn và checklist | Thông tin gói/build ghi ở BUILD_INFO.md |
| Nạp và đo trên thiết bị | LED, TTP223 qua vỏ, độ sáng, nguồn, nhiệt, quét/WiFi/mDNS, AP và OTA | Người dùng đã thử 1.2.0; bản 1.2.1 cần thử lại trên bo/vỏ đó |

## Điểm cần hiệu chỉnh sau nạp

- Xác nhận flash 4 MB và pinout bo, thứ tự màu GRB của dây LED.
- Độ nhạy TTP223 qua vỏ; thời gian giữ và bước sáng nếu muốn đổi.
- Mức sáng ngủ mặc định, MIN_OUTPUT và gamma trong phòng tối/vỏ thật.
- Dòng/điện áp/nhiệt khi dùng đường USB-C → 5V, trước khi tăng trần.
- Hiệu ứng dâng lên cần hai đoạn đối xứng; sửa mapping nếu bố trí LED khác.
- Quét/chọn WiFi trên 2,4 GHz; AP tắt ngay khi nhận IP và mở lại sau 30 giây mất kết nối.
- `.local` trên PC/điện thoại cùng mạng; các tên riêng khi dùng nhiều đèn và phương án IP nếu mDNS bị chặn.
- Reset đúng +/− 5 giây qua vỏ, không nhận nhầm nút và giữ nguyên cấu hình/tên sau reset.
- Tốc độ, màu và độ mượt qua vỏ, ưu tiên mã 2/4/8/9/10/11/13 ở sáng 5/50/100 và tốc độ 1/50/100; giữ pha khi đổi tốc độ.
- Độ tương phản Lấp lánh trên vỏ dày với màu RGB đang chọn và nền gần đen.
- Cấu hình/mục yêu thích/bật tắt trước khi OTA 1.2.0 → 1.2.1 và sau reboot; WiFi/tên giữ nguyên.

Cấu hình SSID/mật khẩu trực tiếp tại thiết bị. Toàn bộ dự án và gói tải được đưa lên repo GitHub NguyenPhat-NT/den_trang_tri theo yêu cầu của chủ dự án. Web điều khiển nằm trên ESP; không xuất bản website điều khiển ra ngoài. Chưa nạp thiết bị từ cloud.

## Kiến trúc

Core.h chứa điều khiển, nhận cảm ứng, timer và renderer; không phụ thuộc Arduino. Storage.h encode dữ liệu rõ bằng JSON và envelope nhị phân; Preferences chỉ là lớp I/O. DenDeco.ino dùng WebServer tích hợp, WiFi và Update, mọi lệnh vào cùng Controller. WebUI.h chứa HTML/CSS/JS đầy đủ, không dùng CDN.

Không có tác vụ chờ kết nối WiFi hoặc đợi quét WiFi trong vòng lặp. Web yêu cầu quét rồi đọc kết quả/trạng thái bằng API; phần STA/AP theo dõi kết nối và IP được cấp. WebServer xử lý request tuần tự; HTTP chậm hoặc upload OTA có thể tạm chiếm vòng lặp. Mục tiêu phản hồi và đồng bộ trong tài liệu cần đo trên mạng/bo thật; kiểm tra trình duyệt mô phỏng không chứng minh các mốc 100/500 ms của thiết bị.
