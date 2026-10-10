# Cấu hình và kế hoạch Đèn Deco

## Các quyết định đã chốt trong chat

Phạm vi: firmware, web trên ESP và hướng dẫn đấu nối/nạp. Người dùng tự xử lý cơ khí/vỏ. ESP32-C3 SuperMini, bốn TTP223, 20 WS2812B và nguồn 5 V–2 A. Tên Đèn Deco; WiFi nhà + AP dự phòng. Máy nạp Windows, Arduino IDE. Bổ sung OTA qua web theo yêu cầu mới.

Đặc tả Word là nguồn yêu cầu; thông tin được chốt cho phiên bản này là nguồn 5 V–2 A và 20 LED. Cấu hình bản cũ không thay thế các quyết định đã xác nhận cho bản này.

Chế độ ngủ nhớ riêng mức đã chỉnh, ban đầu 5%. Các mặc định giữ/nút, 6 chế độ, timer và gamma lấy từ đề xuất trong tài liệu đã được người dùng đồng ý. Mức tối thiểu hiệu chỉnh khởi đầu 6/255 để cam ngủ không chỉ còn kênh đỏ; phải thử trên đèn thật.

## Kế hoạch và trạng thái

| Giai đoạn | Kết quả | Trạng thái |
| --- | --- | --- |
| Chốt phạm vi, cấu hình và phụ thuộc | Arduino core/thư viện có phiên bản rõ ràng | Hoàn tất |
| Viết lõi điều khiển, cảm ứng, renderer và timer | Một nguồn trạng thái, xử lý theo millis không chờ WiFi | Hoàn tất và kiểm tra logic |
| NVS | Hai slot, CRC, kiểm tra ghi và retry; không lưu khung hình | Hoàn tất và kiểm tra mô phỏng lỗi |
| Web/WiFi | Web nhúng, polling 250 ms, AP dự phòng và cài đặt mạng | Hoàn tất; giao diện đã kiểm tra với API mô phỏng |
| OTA | File ứng dụng C3, phân vùng còn lại, giữ NVS, reboot sau thành công | Đã viết/biên dịch; cần thử truyền và reboot thật |
| Bàn giao Windows | Sketch, .bin OTA, hướng dẫn và checklist | Hoàn tất |
| Nạp và đo trên thiết bị | LED, TTP223, độ sáng, nguồn, nhiệt, AP và OTA | Người dùng thực hiện trên bộ lắp thật |

## Điểm cần hiệu chỉnh sau nạp

- Xác nhận flash 4 MB và pinout bo, thứ tự màu GRB của dây LED.
- Độ nhạy TTP223 qua vỏ; thời gian giữ và bước sáng nếu muốn đổi.
- Mức sáng ngủ mặc định, MIN_OUTPUT và gamma trong phòng tối/vỏ thật.
- Dòng/điện áp/nhiệt khi dùng đường USB-C → 5V, trước khi tăng trần.
- Hiệu ứng dâng lên cần hai đoạn đối xứng; sửa mapping nếu bố trí LED khác.

Cấu hình SSID/mật khẩu trực tiếp tại thiết bị. Toàn bộ dự án và gói tải được đưa lên repo GitHub NguyenPhat-NT/den_trang_tri theo yêu cầu của chủ dự án. Web điều khiển nằm trên ESP; không xuất bản website điều khiển ra ngoài. Chưa nạp thiết bị từ cloud.

## Kiến trúc

Core.h chứa điều khiển, nhận cảm ứng, timer và renderer; không phụ thuộc Arduino. Storage.h encode dữ liệu rõ bằng JSON và envelope nhị phân; Preferences chỉ là lớp I/O. DenDeco.ino dùng WebServer tích hợp, WiFi và Update, mọi lệnh vào cùng Controller. WebUI.h chứa HTML/CSS/JS đầy đủ, không dùng CDN.

Không có tác vụ chờ kết nối WiFi trong vòng lặp. WebServer xử lý request tuần tự; HTTP chậm hoặc upload OTA có thể tạm chiếm vòng lặp. Mục tiêu phản hồi và đồng bộ trong tài liệu cần đo trên mạng/bo thật; kiểm tra trình duyệt mô phỏng không chứng minh các mốc 100/500 ms của thiết bị.
