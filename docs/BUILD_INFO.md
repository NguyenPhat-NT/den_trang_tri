# Build đã kiểm chứng — 10/10/2026

- PlatformIO Core 6.1.18; platform espressif32 6.12.0.
- Target esp32-c3-devkitm-1 (ESP32-C3 4 MB), USB CDC bật.
- Framework Arduino ESP32 2.0.17 (package 3.20017.0).
- Adafruit NeoPixel 1.12.5; ArduinoJson 7.4.2.
- Phân vùng default.csv: NVS 0x9000 / OTA data 0xe000 / app0 0x10000 và app1 0x150000, mỗi app 0x140000 (1.310.720 byte).
- Sketch LED_Check cũng biên dịch thành công: flash 248.858 byte, RAM tĩnh 13.964 byte.
- Build cuối: SUCCESS, RAM tĩnh 42.380/327.680 byte; flash code/data 866.338/1.310.720 byte.
- File ứng dụng OTA thực tế: 900.688 byte. SHA-256 ở firmware/SHA256SUMS.txt.
- Image header của file ứng dụng đúng ESP32-C3 và app descriptor. Header bootloader/wrong-chip/merged-style không qua bộ lọc OTA.

File nhị phân là ứng dụng, không chứa bootloader hoặc bảng phân vùng. Nạp đầu bằng Arduino IDE qua USB theo hướng dẫn. Profile build dùng để xác minh biên dịch cho C3; flash 4 MB và chân của SuperMini vẫn cần đối chiếu bo của bạn. Chưa nạp thiết bị hoặc OTA phần cứng từ môi trường cloud.
