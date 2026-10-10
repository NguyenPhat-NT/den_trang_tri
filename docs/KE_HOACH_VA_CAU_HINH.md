# Cấu hình và kế hoạch Đèn Deco 1.1.0

## Các quyết định đã chốt trong chat

Phạm vi: firmware, web trên ESP và hướng dẫn đấu nối/nạp. Người dùng tự xử lý cơ khí/vỏ. ESP32-C3 SuperMini, bốn TTP223, 20 WS2812B và nguồn 5 V–2 A. Tên Đèn Deco; WiFi nhà + AP dự phòng. Máy nạp Windows, Arduino IDE. Bổ sung OTA qua web theo yêu cầu mới.

Bản 1.1.0 bổ sung quét/chọn WiFi 2,4 GHz bất đồng bộ và trạng thái kết nối trên web. Địa chỉ mặc định `http://den-deco.local`; tên truy cập chỉnh được riêng với nhãn tên đèn. Với nhiều đèn, đặt tên truy cập khác nhau và dùng địa chỉ thực tế hiển thị trên trang. Khi phát hiện trùng tên mDNS, tên thực tế được thêm mã bo sáu ký tự viết thường rồi `-2`, `-3`… nếu còn trùng; tên người dùng đã lưu không bị đổi. Kiểm tra định kỳ theo trạng thái mDNS SDK, nhưng thử trùng tên trên phần cứng còn chờ. Địa chỉ `.local` phụ thuộc mDNS trong mạng nội bộ; IP DHCP là cách truy cập dự phòng.

AP `DenDeco-XXXXXX` không có mật khẩu, chỉ mở khi chưa lưu WiFi, chưa kết nối được trong 30 giây hoặc xóa/reset WiFi để cấu hình lại. WiFi nhà kết nối thành công và nhận IP từ DHCP sẽ tắt AP ngay, kể cả phiên cấu hình đang dùng AP. Khi router mất kết nối, đèn thử nối lại, mở AP sau 30 giây chưa khôi phục và tắt AP khi nhận IP trở lại.

Giữ đúng hai nút + và − đồng thời liên tục 5 giây chỉ xóa SSID/mật khẩu WiFi nhà; giữ cấu hình ánh sáng, ba mục yêu thích, tên đèn và tên truy cập. Thả sớm hoặc thêm nút khác hủy reset. Bốn đầu vào cảm ứng dùng `INPUT_PULLDOWN` để chân chưa đấu không tự sinh lệnh; chạm/giữ và độ nhạy qua vỏ vẫn cần thử trên phần cứng.

Đặc tả Word là nguồn yêu cầu; thông tin được chốt cho phiên bản này là nguồn 5 V–2 A và 20 LED. Cấu hình bản cũ không thay thế các quyết định đã xác nhận cho bản này.

Chế độ ngủ nhớ riêng mức đã chỉnh, ban đầu 5%. Các mặc định giữ/nút, 6 chế độ, timer và gamma lấy từ đề xuất trong tài liệu đã được người dùng đồng ý. Mức tối thiểu hiệu chỉnh khởi đầu 6/255 để cam ngủ không chỉ còn kênh đỏ; phải thử trên đèn thật.

## Kế hoạch và trạng thái

| Giai đoạn | Kết quả | Trạng thái |
| --- | --- | --- |
| Chốt phạm vi, cấu hình và phụ thuộc | Arduino core/thư viện có phiên bản rõ ràng | Hoàn tất |
| Viết lõi điều khiển, cảm ứng, renderer và timer | Một nguồn trạng thái, xử lý theo millis không chờ WiFi | Hoàn tất và kiểm tra logic |
| NVS | Hai slot, CRC, kiểm tra ghi và retry; không lưu khung hình | Hoàn tất và kiểm tra mô phỏng lỗi |
| Web/WiFi | Web nhúng, polling 250 ms; quét 2,4 GHz, trạng thái mạng, hostname và AP dự phòng | Bản 1.1.0 cập nhật; kết quả kiểm tra ghi ở KIEM_THU.md |
| Reset WiFi từ cảm ứng | Giữ đúng +/− 5 giây, giữ cấu hình đèn và tên | Bản 1.1.0 cập nhật; thao tác vật lý qua vỏ chưa thử |
| OTA | File ứng dụng C3, phân vùng còn lại, giữ NVS, reboot sau thành công | Đã viết/biên dịch; cần thử truyền và reboot thật |
| Bàn giao Windows | Sketch, .bin OTA 1.1.0, hướng dẫn và checklist | Thông tin gói/build ghi ở BUILD_INFO.md |
| Nạp và đo trên thiết bị | LED, TTP223 qua vỏ, độ sáng, nguồn, nhiệt, quét/WiFi/mDNS, AP và OTA | Chưa thử; người dùng thực hiện trên bộ lắp thật |

## Điểm cần hiệu chỉnh sau nạp

- Xác nhận flash 4 MB và pinout bo, thứ tự màu GRB của dây LED.
- Độ nhạy TTP223 qua vỏ; thời gian giữ và bước sáng nếu muốn đổi.
- Mức sáng ngủ mặc định, MIN_OUTPUT và gamma trong phòng tối/vỏ thật.
- Dòng/điện áp/nhiệt khi dùng đường USB-C → 5V, trước khi tăng trần.
- Hiệu ứng dâng lên cần hai đoạn đối xứng; sửa mapping nếu bố trí LED khác.
- Quét/chọn WiFi trên 2,4 GHz; AP tắt ngay khi nhận IP và mở lại sau 30 giây mất kết nối.
- `.local` trên PC/điện thoại cùng mạng; các tên riêng khi dùng nhiều đèn và phương án IP nếu mDNS bị chặn.
- Reset đúng +/− 5 giây qua vỏ, không nhận nhầm nút và giữ nguyên cấu hình/tên sau reset.

Cấu hình SSID/mật khẩu trực tiếp tại thiết bị. Toàn bộ dự án và gói tải được đưa lên repo GitHub NguyenPhat-NT/den_trang_tri theo yêu cầu của chủ dự án. Web điều khiển nằm trên ESP; không xuất bản website điều khiển ra ngoài. Chưa nạp thiết bị từ cloud.

## Kiến trúc

Core.h chứa điều khiển, nhận cảm ứng, timer và renderer; không phụ thuộc Arduino. Storage.h encode dữ liệu rõ bằng JSON và envelope nhị phân; Preferences chỉ là lớp I/O. DenDeco.ino dùng WebServer tích hợp, WiFi và Update, mọi lệnh vào cùng Controller. WebUI.h chứa HTML/CSS/JS đầy đủ, không dùng CDN.

Không có tác vụ chờ kết nối WiFi hoặc đợi quét WiFi trong vòng lặp. Web yêu cầu quét rồi đọc kết quả/trạng thái bằng API; phần STA/AP theo dõi kết nối và IP được cấp. WebServer xử lý request tuần tự; HTTP chậm hoặc upload OTA có thể tạm chiếm vòng lặp. Mục tiêu phản hồi và đồng bộ trong tài liệu cần đo trên mạng/bo thật; kiểm tra trình duyệt mô phỏng không chứng minh các mốc 100/500 ms của thiết bị.
