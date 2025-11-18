# 🔒 Hệ Thống Chống Trộm Báo Động Thông Minh

Hệ thống an ninh DIY sử dụng **ESP32-C3** để phát hiện mở cửa, báo chuông, gửi email cảnh báo và quản lý lịch sử qua giao diện web.

## ✨ Tính Năng

- 🚪 **Phát hiện mở cửa** - Cảm biến từ (magnetic reed switch) kích hoạt tức thời
- 🔔 **Báo động đa cấp** - Còi buzzer + âm thanh JQ6500 + email Gmail
- ⏰ **Lên lịch tự động** - Tự động bật/tắt cảnh báo theo khung giờ
- 📊 **Lịch sử chi tiết** - Ghi nhận tất cả sự kiện mở/đóng cửa
- 🌐 **Giao diện web** - Điều khiển qua trình duyệt (WebSocket real-time)
- 📱 **WiFi thông minh** - Chế độ AP tự động khi WiFi thất bại

---

## 🔧 Phần Cứng

| Linh Kiện | Chức Năng | Pin |
|-----------|----------|-----|
| **ESP32-C3** | Bộ vi điều khiển | - |
| **Cảm biến từ** | Phát hiện mở/đóng cửa | GPIO 2 |
| **Nút nhấn** | Điều khiển / Reset WiFi | GPIO 9 |
| **Còi Buzzer** | Báo chuông | GPIO 8 |
| **JQ6500** | Module âm thanh MP3 | UART (GPIO 20/21) |
| **Loa 1W 8Ω** | Phát âm thanh | JQ6500 SPK+/- |

---

## 🚀 Cài Đặt Nhanh

### 1. Chuẩn Bị
- Arduino IDE với board ESP32 (Espressif Systems)
- Cài thư viện: `AsyncTCP`, `ESPAsyncWebServer`, `NTPClient`, `Arduino_JSON`, `ESP Mail Client`
- Thư viện BLK: Copy `BLKLab_Thu_Vien` vào thư mục Arduino

### 2. Nạp Code
```
Board: ESP32C3 Dev Module
Partition Scheme: Huge App (3MB No OTA/1Mb SPIFFS)
Sketch → Upload
```

### 3. Cấu Hình WiFi
- **Lần đầu**: Nhấn giữ nút → phát WiFi `ESP32 BLK`
- Kết nối → truy cập `http://192.168.4.1`
- Nhập SSID + Password WiFi nhà → restart

### 4. Cấu Hình Email
- Truy cập: `http://192.168.1.36` → Tab **Email**
- Nhập Gmail sender (phải dùng **App Password** 16 ký tự, không phải mật khẩu thường)
- Nhập email nhận cảnh báo

---

## 📱 Giao Diện Web

Truy cập: `http://192.168.1.36` (hoặc IP DHCP tự động)

### Tab 1: Cấu Hình
- Bật/Tắt chế độ cảnh báo
- Thiết lập khung giờ báo động
- Chọn âm thanh & điều chỉnh âm lượng

### Tab 2: Lịch Sử
- Xem sự kiện mở/đóng cửa
- Xóa lịch sử

### Tab 3: Email
- Cấu hình tài khoản Gmail
- Thiết lập nội dung cảnh báo

---

## 🎮 Nút Nhấn

| Hành Động | Chức Năng |
|-----------|----------|
| **Nhấn nhanh** | Bật/Tắt chế độ cảnh báo |
| **Nhấn giữ 3s** | Reset WiFi → Phát AP `ESP32 BLK` |

---

## 📊 Quy Trình Hoạt Động

```
Khởi động
  ↓
Kết nối WiFi
  ↓
Khởi động Web Server + WebSocket
  ↓
━━━━━━━━━ Vòng lặp chính ━━━━━━━━━
  ↓
Xử lý nút nhấn → Kiểm tra lịch → Đọc sensor cửa
  ↓
┌──────────────────┬──────────────────┐
│  Cửa mở (LOW)    │  Cửa đóng (HIGH) │
└──────────────────┴──────────────────┘
  ↓                  ↓
Chế độ BẬT?        Tắt báo động
  │                  │
  ├─ Phát còi       Dừng âm thanh
  ├─ Phát âm thanh
  ├─ Gửi email
  └─ Ghi lịch sử
```

---

## ⚙️ Cấu Hình Email (Chi Tiết)

### Lấy App Password Gmail
1. Vào https://myaccount.google.com/
2. **Security** → Bật **2-Step Verification**
3. Vào https://myaccount.google.com/apppasswords
4. Chọn **Mail** → **Windows PC**
5. Sao chép 16 ký tự → nhập vào giao diện web

**⚠️ Lưu ý:** Dùng **App Password**, không phải mật khẩu Gmail thường!

---

## 🐛 Troubleshooting

### ❌ Không kết nối WiFi
- Kiểm tra SSID/Password có đúng không
- Reset WiFi: Nhấn giữ nút → cài đặt lại

### ❌ Sensor phát hiện ngược
- Cảm biến từ: HIGH = đóng, LOW = mở
- Nếu ngược, đảo dây cảm biến hoặc sửa logic code

### ❌ Email không gửi
- Kiểm tra internet kết nối
- Phải dùng **App Password** (16 ký tự), không phải mật khẩu thường
- Kiểm tra port 465 (SMTP)

### ❌ Không vào được giao diện web
- Ping `192.168.1.36` để kiểm tra IP
- Thử refresh cứng: `Ctrl+F5`
- Xem Serial Monitor xem có lỗi gì

---

## 📁 Cấu Trúc File

```
├── BLKLab_PRJ03_DIY_He_Thong_Canh_Bao_Chong_Trom.ino  (Chương trình chính)
├── index_html.h                (Giao diện web)
├── JQ6500.h / JQ6500.cpp      (Module âm thanh)
├── mybutton.h                  (Nút nhấn)
├── data_config.h               (Cấu hình WiFi AP)
└── README.md                   (Tài liệu này)
```

---

## 🔐 Bảo Mật

⚠️ **Chú ý:**
- Mật khẩu lưu plaintext trong Preferences (bộ nhớ ESP32)
- Tạo **Gmail riêng** cho hệ thống
- **WiFi phải mạnh** (12+ ký tự)
- Đặt thiết bị ở nơi an toàn
- **Không chia sẻ** thông tin cấu hình

---

## 📝 Ghi Chú

- **Giờ hệ thống:** GMT+7 (NTP từ pool.ntp.org)
- **Lưu trữ:** Preferences (bộ nhớ ESP32)
- **Lịch sử tối đa:** 100 bản ghi
- **WebSocket:** Real-time sync giữa device và web

---

## 🎓 Tài Liệu

- [ESP32-C3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- [AsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [Gmail SMTP Settings](https://support.google.com/mail/answer/7126229)

---

**BLK Lab | 2025**
