/*
  Banlinhkien.com 
  PRJ03: DIY Hệ Thống Chống Trộm Báo Động
  Tính năng:
    - Phát hiện mở cửa tức thời: Sử dụng cảm biến từ (cảm biến mở cửa) để phát hiện khi cửa bị mở trái phép.
    - Cảnh báo âm thanh khẩn cấp: Khi phát hiện có xâm nhập trái phép, hệ thống kích hoạt còi báo động để phát ra âm thanh cảnh báo lớn.
    - Phát âm thanh giọng nói: Tích hợp module JQ6500 giúp phát ra các đoạn âm thanh cảnh báo thực tế.
    - Cập nhật lịch sử: Hiển thị lịch sử đóng mở cửa trên giao điện web, dễ dang quản lý và theo dõi.
    - Email thông báo: Khi có có xâm nhập trái phép hệ thống gửi tin nhắn cảnh báo về email người dùng.
  Đấu nối:
  --------------------------------
  ESP32-C3        |        Cảm biến mở cửa
  GND             |           Pin 1  
  2               |           Pin 2 
  ---------------------------------
  ESP32-C3        |            Nút nhấn SWITCH (nhấn nhanh: chuyển chế độ. Nhấn giữ: xóa cài đặt wifi, esp sẽ phát ra wifi "ESP32 BLK". bạn kết nối vào wifi, truy cập 192.168.4.1 để cài lại wifi)
  GND             |            Chân 1
  D9              |            Chân 2
  ---------------------------------
  ESP32-C3        |           CÒI
  3.3V            |           Chân +
  8               |           Chân -
  --------------------------------
  JQ6500          |           Loa 1W8R
  SPK+            |           SPK+
  SPK-            |           SPK-
  --------------------------------
  ESP32-C3        |         JQ6500
  20              |           RX
  21              |           TX
  VCC             |           5V
  GND             |           GND
  --------------------------------
  Lưu ý khi nạp code:
   + Chọn Board: ESP32C3 Dev Module
   + Chọn Port : Port tương ứng
   + USB CDC On BOOT : Enable
   + Patition Scheme: Huge App (3MB No OTA/ 1Mb SPIFFS)
  Chọn đường dẫn thư viện:
    Vào File -> Preferences -> Sketchbook location -> Các bạn trỏ đến thư mục chứa thư mục BLKLab_Thu_Vien trong tài liệu
    Chú ý đường dẫn không được có dấu tiếng việt, không có kí đặc biệt.
*/
#include <WiFiClient.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <EEPROM.h>
#include <Arduino_JSON.h>
#include <WiFi.h>
#include <time.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Preferences.h>
#include <ESP_Mail_Client.h>
#include <WiFiClientSecure.h>

#include "index_html.h"
#include "mybutton.h"
#include "JQ6500.h"
#include "data_config.h"

Preferences preferences;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // GMT+7
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

#define WIFI_OK 1
#define WIFI_DIS 0
int wifi_status = WIFI_DIS;
// IPAddress local_IP(192, 168, 1, 36);   // Tắt IP tĩnh, dùng DHCP
// IPAddress gateway(192, 168, 1, 1);
// IPAddress subnet(255, 255, 255, 0);
// IPAddress primaryDNS(8, 8, 8, 8);
// IPAddress secondaryDNS(8, 8, 4, 4);

#define BUZZER_PIN 8
#define BUTTON_SET_PIN 9
#define BUTTON_SET_ID 1
#define SENSOR_PIN 2

Button buttonSET;
JQ6500 jq6500;

struct AlertTimer {
    int startHour;
    int startMinute;
    int endHour;
    int endMinute;
};

struct DeviceConfig {
    bool alertMode;
    AlertTimer timers[10];
    int timerCount;
    int volume;
    String alertSound;
    String normalSound;
    bool buzzerEnabled;
    bool manualOverride;
} deviceConfig;

struct EmailConfig {
    String senderEmail;
    String senderPassword;
    String recipientEmail;
    String alertContent;
} emailConfig;

struct HistoryEntry {
    String time;   // HH:MM
    String date;   // DD-MM-YYYY
    String status;
    bool alertMode;
};

const int MAX_HISTORY = 100;
HistoryEntry history[MAX_HISTORY];
int historyCount = 0;
int openOnCount = 0;
int openOffCount = 0;
String lastDate = "";

bool alarmActive = false;
unsigned long lastAudioPlayTime = 0;
const unsigned long audioPlayInterval = 5000;
bool doorOpen = false;
bool lastDoorState = false;
bool emailSent = false;

SMTPSession smtp;
void smtpCallback(SMTP_Status status);

void setup() {
    Serial.begin(115200);
    Serial.println("🔄 Khởi động hệ thống....");
    EEPROM.begin(512);
    readEEPROM();
    loadConfig();
    loadEmailConfig();
    loadHistory();
    loadOpenCounts();

    jq6500.begin();
    Serial.println("🔄 JQ6500 UART Initialized");
    delay(1000);

    // if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    //     Serial.println("Không thể cấu hình địa chỉ IP tĩnh!");
    // }

    pinMode(BUTTON_SET_PIN, INPUT_PULLUP);
    button_init(&buttonSET, BUTTON_SET_PIN, BUTTON_ACTIVE_LOW, BUTTON_SET_ID);
    button_pressshort_set_callback((void *)button_press_short_callback);
    button_presslong_set_callback((void *)button_press_long_callback);

    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);
    pinMode(SENSOR_PIN, INPUT);

    connectToWiFi();
    serverBegin(wifi_status);
    // testSMTPConnection();  // Tắt tạm để tránh crash
    jq6500.playFileByIndex(7);
    jq6500.play();
    // sendEmailHello();  // Tắt tạm để tránh crash
    beepBuzzer(500, 3);

    timeClient.update();
    lastDate = getCurrentDate();
}

void loop() {
    handle_button(&buttonSET);
    checkAlertTimers();
    openDoor();
    ws.cleanupClients();
}

//---------------------- Hàm kiểm tra bật tắt cảnh báo theo thời gian------------------
void checkAlertTimers() {
    if (deviceConfig.timerCount <= 0) return;
    if (deviceConfig.manualOverride) return;

    timeClient.update();
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    int currentTime = currentHour * 60 + currentMinute;

    bool shouldBeAlert = false;
    for (int i = 0; i < deviceConfig.timerCount; i++) {
        int startTime = deviceConfig.timers[i].startHour * 60 + deviceConfig.timers[i].startMinute;
        int endTime = deviceConfig.timers[i].endHour * 60 + deviceConfig.timers[i].endMinute;
        if (startTime <= endTime) {
            if (currentTime >= startTime && currentTime <= endTime) {
                shouldBeAlert = true;
                break;
            }
        } else {
            if (currentTime >= startTime || currentTime <= endTime) {
                shouldBeAlert = true;
                break;
            }
        }
    }
    if (shouldBeAlert && !deviceConfig.alertMode) {
        deviceConfig.alertMode = true;
        Serial.println("Tự động bật chế độ cảnh báo theo khung giờ");
        saveConfig(); // Lưu vào Preferences
        ws.textAll(createConfigJSON());
    } else if (!shouldBeAlert && deviceConfig.alertMode) {
        deviceConfig.alertMode = false;
        Serial.println("Tự động tắt chế độ cảnh báo theo khung giờ");
        saveConfig(); // Lưu vào Preferences
        ws.textAll(createConfigJSON());
    }
}

#define DEBOUNCE_TIME 100
unsigned long lastDebounceTime = 0;
bool lastStableDoorState = LOW;

void openDoor() {
    bool currentRawState = digitalRead(SENSOR_PIN);
    if (currentRawState != lastStableDoorState) {
        lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > DEBOUNCE_TIME) {
        if (currentRawState != lastDoorState) {
            lastDoorState = currentRawState;
            timeClient.update();
            String currentTime = timeClient.getFormattedTime().substring(0, 5);
            String currentDate = getCurrentDate();

            if (currentRawState == LOW && lastDate != "" && currentDate != lastDate) {
                clearHistory();
                openOnCount = 0;
                openOffCount = 0;
                saveOpenCounts();
                lastDate = currentDate;
                Serial.println("Ngày mới: " + currentDate + " - Đã xóa dữ liệu ngày cũ");
                ws.textAll(createHistoryJSON());
            }

            if (currentRawState == LOW) {
                doorOpen = true;
                Serial.println("Cửa mở");
                addHistoryEntry(currentTime, currentDate, "Mở", deviceConfig.alertMode);
                if (deviceConfig.alertMode) {
                    openOnCount++;
                    alarmActive = true;
                    beepBuzzer(100, 6);
                    playAlertSound();
                    // if (!emailSent) {
                    //     sendEmail();  // Tắt tạm để tránh crash
                    //     emailSent = true;
                    // }
                } else {
                    openOffCount++;
                    jq6500.resetMP3();
                    playNormalSound();
                }
                saveOpenCounts();
            } else {
                doorOpen = false;
                Serial.println("Cửa đóng");
                addHistoryEntry(currentTime, currentDate, "Đóng", deviceConfig.alertMode);
                if (deviceConfig.alertMode) {
                    alarmActive = false;
                }
                emailSent = false;
                jq6500.resetMP3();
            }
            lastDate = currentDate;
            ws.textAll(createHistoryJSON());
            ws.textAll(createConfigJSON());
        }
    }
    lastStableDoorState = currentRawState;
    if (deviceConfig.alertMode && alarmActive && doorOpen) {
        if (millis() - lastAudioPlayTime >= audioPlayInterval) {
            playAlertSound();
            lastAudioPlayTime = millis();
            beepBuzzer(100, 6);
        }
        if (deviceConfig.buzzerEnabled) {
            beepBuzzer(100, 10);
            jq6500.resetMP3();
        }
    }
}

String getCurrentDate() {
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime(&epochTime);
    char dateBuffer[11];
    sprintf(dateBuffer, "%02d-%02d-%04d", ptm->tm_mday, ptm->tm_mon + 1, ptm->tm_year + 1900);
    return String(dateBuffer);
}

//--------------------Hàm phát còi chíp-----------------
void beepBuzzer(int delayBeep, int timeBeep) {
    for (int i = 0; i < timeBeep; i++) {
        digitalWrite(BUZZER_PIN, LOW);
        delay(delayBeep);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(delayBeep);
    }
}

//----------------Hàm phát âm thanh tắt cảnh báo---------------
void playAlertSound() {
    if (deviceConfig.alertSound != "none") {
        jq6500.setVolume(deviceConfig.volume);
        if (deviceConfig.alertSound == "alert1") jq6500.playFileByIndex(1);
        else if (deviceConfig.alertSound == "alert2") jq6500.playFileByIndex(2);
        else if (deviceConfig.alertSound == "alert3") jq6500.playFileByIndex(3);
        jq6500.play();
        Serial.println("Phát âm thanh cảnh báo: " + deviceConfig.alertSound);
    }
}

//----------------Hàm phát âm thanh cảnh báo---------------
void playNormalSound() {
    if (deviceConfig.normalSound != "none") {
        jq6500.setVolume(deviceConfig.volume);
        if (deviceConfig.normalSound == "normal1") jq6500.playFileByIndex(4);
        else if (deviceConfig.normalSound == "normal2") jq6500.playFileByIndex(5);
        else if (deviceConfig.normalSound == "normal3") jq6500.playFileByIndex(6);
        jq6500.play();
        Serial.println("Phát âm thanh bình thường: " + deviceConfig.normalSound);
    }
}

// -----------------------Hàm thêm lịch sử---------------------
void addHistoryEntry(String time, String date, String status, bool alertMode) {
    if (historyCount >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            history[i] = history[i + 1];
        }
        history[MAX_HISTORY - 1].time = time;
        history[MAX_HISTORY - 1].date = date;
        history[MAX_HISTORY - 1].status = status;
        history[MAX_HISTORY - 1].alertMode = alertMode;
    } else {
        for (int i = historyCount; i > 0; i--) {
            history[i] = history[i - 1];
        }
        history[0].time = time;
        history[0].date = date;
        history[0].status = status;
        history[0].alertMode = alertMode;
        historyCount++;
    }
    saveHistory();
    Serial.println("Thêm lịch sử - Ngày: " + date + ", Thời gian: " + time + ", Trạng thái: " + status + ", AlertMode: " + String(alertMode));
}

// ------------------------ Hàm lưu lại lịch sử-----------------
void saveHistory() {
    preferences.begin("history", false);
    preferences.putInt("historyCount", historyCount);
    for (int i = 0; i < historyCount; i++) {
        String keyTime = "history" + String(i) + "_time";
        String keyDate = "history" + String(i) + "_date";
        String keyStatus = "history" + String(i) + "_status";
        String keyAlertMode = "history" + String(i) + "_alertMode";
        preferences.putString(keyTime.c_str(), history[i].time);
        preferences.putString(keyDate.c_str(), history[i].date);
        preferences.putString(keyStatus.c_str(), history[i].status);
        preferences.putBool(keyAlertMode.c_str(), history[i].alertMode);
    }
    preferences.end();
    Serial.println("Đã lưu lịch sử vào Preferences");
}

//-----------------------------Hàm tải lại lịch sử khi khởi động-----------------
void loadHistory() {
    preferences.begin("history", true);
    historyCount = preferences.getInt("historyCount", 0);
    for (int i = 0; i < historyCount; i++) {
        String keyTime = "history" + String(i) + "_time";
        String keyDate = "history" + String(i) + "_date";
        String keyStatus = "history" + String(i) + "_status";
        String keyAlertMode = "history" + String(i) + "_alertMode";
        history[i].time = preferences.getString(keyTime.c_str(), "");
        history[i].date = preferences.getString(keyDate.c_str(), "");
        history[i].status = preferences.getString(keyStatus.c_str(), "");
        history[i].alertMode = preferences.getBool(keyAlertMode.c_str(), false);
    }
    preferences.end();
    Serial.println("Đã nạp lịch sử từ Preferences");
}

void saveOpenCounts() {
    preferences.begin("openCounts", false);
    preferences.putInt("openOnCount", openOnCount);
    preferences.putInt("openOffCount", openOffCount);
    preferences.putString("lastDate", lastDate);
    preferences.end();
    Serial.println("Đã lưu số lần mở - OpenOn: " + String(openOnCount) + ", OpenOff: " + String(openOffCount) + ", LastDate: " + lastDate);
}

void loadOpenCounts() {
    preferences.begin("openCounts", true);
    openOnCount = preferences.getInt("openOnCount", 0);
    openOffCount = preferences.getInt("openOffCount", 0);
    lastDate = preferences.getString("lastDate", "");
    preferences.end();
    Serial.println("Đã nạp số lần mở - OpenOn: " + String(openOnCount) + ", OpenOff: " + String(openOffCount) + ", LastDate: " + lastDate);
}
// ------------------Hàm xóa lịch sử--------------------
void clearHistory() {
    preferences.begin("history", false);
    preferences.clear();
    preferences.putInt("historyCount", 0);
    preferences.end();
    historyCount = 0;
    Serial.println("Đã xóa toàn bộ lịch sử trong Preferences");
    ws.textAll(createHistoryJSON()); // Gửi lịch sử rỗng sau khi xóa
}

//--------------------Hàm kiểm tra kết nối Email-----------------
void testSMTPConnection() {
    WiFiClientSecure client;
    client.setInsecure();
    Serial.print("Đang kết nối tới SMTP Server...");
    if (client.connect("smtp.gmail.com", 465)) {
        Serial.println(" Kết nối thành công!");
        beepBuzzer(100, 4);
    } else {
        Serial.println(" Không kết nối được!");
        beepBuzzer(100, 5);
    }
}

//------------------- Hàm gửi tin nhắn Email---------------------
void sendEmail() {
    if (WiFi.status() == WL_CONNECTED) {
        MailClient.networkReconnect(true);
        smtp.debug(1);
        smtp.callback(smtpCallback);

        Session_Config config;
        config.server.host_name = "smtp.gmail.com";
        config.server.port = 465;
        config.login.email = emailConfig.senderEmail;
        config.login.password = emailConfig.senderPassword;
        config.login.user_domain = "";

        config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
        config.time.gmt_offset = 7;
        config.time.day_light_offset = 0;

        SMTP_Message message;
        message.sender.name = F("Hệ Thống Chống Trộm ESP32 Maker BLK");
        message.sender.email = emailConfig.senderEmail;
        message.subject = F("⚠️ Cảnh Báo");
        message.addRecipient(F("BLK"), emailConfig.recipientEmail);

        String textMsg = emailConfig.alertContent;
        message.text.content = textMsg.c_str();
        message.text.charSet = "us-ascii";
        message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

        message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
        message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

        if (!smtp.connect(&config)) {
            ESP_MAIL_PRINTF("Lỗi kết nối, Mã trạng thái: %d, Mã lỗi: %d, Lý do: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
            return;
        }

        if (!MailClient.sendMail(&smtp, &message))
            ESP_MAIL_PRINTF("Mã trạng thái: %d, Mã lỗi: %d, Lý do: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    }
}

//------------------------Hàm gửi tin nhắn Email khi khởi động-----------------------
void sendEmailHello() {
    if (WiFi.status() == WL_CONNECTED) {
        MailClient.networkReconnect(true);
        smtp.debug(1);
        smtp.callback(smtpCallback);

        Session_Config config;
        config.server.host_name = "smtp.gmail.com";
        config.server.port = 465;
        config.login.email = emailConfig.senderEmail;
        config.login.password = emailConfig.senderPassword;
        config.login.user_domain = "";

        config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
        config.time.gmt_offset = 7;
        config.time.day_light_offset = 0;

        SMTP_Message message;
        message.sender.name = F("Hệ Thống Chống Trộm ESP32 Maker BLK");
        message.sender.email = emailConfig.senderEmail;
        message.subject = F("🔒 Xin Chào 🔒");
        message.addRecipient(F("BLK"), emailConfig.recipientEmail);

        String textMsg = "Xin chào, bạn đã khởi động hệ thống chống trộm thông minh";
        message.text.content = textMsg.c_str();
        message.text.charSet = "us-ascii";
        message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

        if (!smtp.connect(&config)) {
            ESP_MAIL_PRINTF("Lỗi kết nối, Mã trạng thái: %d, Mã lỗi: %d, Lý do: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
            return;
        }

        if (!MailClient.sendMail(&smtp, &message))
            ESP_MAIL_PRINTF("Mã trạng thái: %d, Mã lỗi: %d, Lý do: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    }
}

void smtpCallback(SMTP_Status status) {
    Serial.println(status.info());
    if (status.success()) {
        Serial.println("----------------");
        ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
        ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
        Serial.println("----------------\n");

        for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
            SMTP_Result result = smtp.sendingResult.getItem(i);
            ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
            ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
            ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
            ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
            ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
        }
        Serial.println("----------------\n");
        smtp.sendingResult.clear();
    }
}

//------------------------Các hàm lưu và tải các cài đặt được thiết lập------------------
void saveConfig() {
    preferences.begin("deviceConfig", false);
    preferences.putBool("alertMode", deviceConfig.alertMode);
    preferences.putInt("timerCount", deviceConfig.timerCount);
    for (int i = 0; i < deviceConfig.timerCount; i++) {
        String keyStartHour = "timer" + String(i) + "_startHour";
        String keyStartMinute = "timer" + String(i) + "_startMinute";
        String keyEndHour = "timer" + String(i) + "_endHour";
        String keyEndMinute = "timer" + String(i) + "_endMinute";
        preferences.putInt(keyStartHour.c_str(), deviceConfig.timers[i].startHour);
        preferences.putInt(keyStartMinute.c_str(), deviceConfig.timers[i].startMinute);
        preferences.putInt(keyEndHour.c_str(), deviceConfig.timers[i].endHour);
        preferences.putInt(keyEndMinute.c_str(), deviceConfig.timers[i].endMinute);
    }
    preferences.putInt("volume", deviceConfig.volume);
    preferences.putString("alertSound", deviceConfig.alertSound);
    preferences.putString("normalSound", deviceConfig.normalSound);
    preferences.putBool("buzzerEnabled", deviceConfig.buzzerEnabled);
    preferences.putBool("manualOverride", deviceConfig.manualOverride);
    preferences.end();
    Serial.println("Đã lưu cấu hình vào Preferences");
}

void loadConfig() {
    preferences.begin("deviceConfig", true);
    deviceConfig.alertMode = preferences.getBool("alertMode", false);
    deviceConfig.timerCount = preferences.getInt("timerCount", 0);
    for (int i = 0; i < deviceConfig.timerCount; i++) {
        String keyStartHour = "timer" + String(i) + "_startHour";
        String keyStartMinute = "timer" + String(i) + "_startMinute";
        String keyEndHour = "timer" + String(i) + "_endHour";
        String keyEndMinute = "timer" + String(i) + "_endMinute";
        deviceConfig.timers[i].startHour = preferences.getInt(keyStartHour.c_str(), 0);
        deviceConfig.timers[i].startMinute = preferences.getInt(keyStartMinute.c_str(), 0);
        deviceConfig.timers[i].endHour = preferences.getInt(keyEndHour.c_str(), 0);
        deviceConfig.timers[i].endMinute = preferences.getInt(keyEndMinute.c_str(), 0);
    }
    deviceConfig.volume = preferences.getInt("volume", 50);
    deviceConfig.alertSound = preferences.getString("alertSound", "none");
    deviceConfig.normalSound = preferences.getString("normalSound", "none");
    deviceConfig.buzzerEnabled = preferences.getBool("buzzerEnabled", false);
    deviceConfig.manualOverride = preferences.getBool("manualOverride", false);
    preferences.end();
    Serial.println("Đã nạp cấu hình từ Preferences");
}

void loadEmailConfig() {
    preferences.begin("emailConfig", true);
    emailConfig.senderEmail = preferences.getString("senderEmail", "");
    emailConfig.senderPassword = preferences.getString("senderPassword", "");
    emailConfig.recipientEmail = preferences.getString("recipientEmail", "");
    emailConfig.alertContent = preferences.getString("alertContent", "Cảnh báo: Cửa đã được mở!");
    preferences.end();
    Serial.println("Đã nạp cấu hình email từ Preferences");
}

//-------------------- Hàm kiểm tra kết nối Wifi---------------------------
void connectToWiFi() {
    if (Essid.length() > 1) {
        Serial.println(Essid);
        Serial.println(Epass);
        WiFi.begin(Essid.c_str(), Epass.c_str());
        WiFi.mode(WIFI_STA);
        int countConnect = 0;
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
            Serial.print(".");
            if (countConnect++ == 10) {
                Serial.println("\nKết nối WiFi thất bại");
                beepBuzzer(100, 5);
                wifi_status = WIFI_DIS;
                break;
            }
        }
        if (WiFi.status() == WL_CONNECTED) {
            beepBuzzer(100, 3);
            Serial.println("\nĐã kết nối WiFi");
            Serial.println("IP: " + WiFi.localIP().toString());
            timeClient.begin();
            wifi_status = WIFI_OK;
        }
    }
}

// -------------------Khởi động server---------------------------
void serverBegin(int wifi_status) {
    server.end();
    delay(100);
    if (wifi_status == WIFI_OK) {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", index_html_stamode);
        });

        ws.onEvent(onWsEvent);
        server.addHandler(&ws);
        server.begin();
        Serial.println("Server STA mode với WebSocket đã khởi động");
    } else if (wifi_status == WIFI_DIS) {
        Serial.println("WIFI_DIS_APMODE");
        WiFi.softAP(ssidAP, passwordAP);
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", index_html_apmode);
        });
        server.on("/data_before", HTTP_GET, [](AsyncWebServerRequest *request) {
            String json = getJsonData();
            request->send(200, "application/json", json);
            Serial.println("Gửi dữ liệu WiFi AP: " + json);
        });
        server.on("/post_data", HTTP_POST, [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", "SUCCESS");
            ESP.restart();
        }, NULL, getDataFromClient);
        server.begin();
        Serial.println("Server AP mode đã khởi động");
    }
}

//--------------- Xử lý sự kiện WebSocket----------------------
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.println("WebSocket client connected");
        String configData = createConfigJSON();
        String historyData = createHistoryJSON();
        String emailData = createEmailConfigJSON();
        client->text("{\"type\":\"config\",\"data\":" + configData + "}");
        client->text("{\"type\":\"history\",\"data\":" + historyData + "}");
        client->text("{\"type\":\"email\",\"data\":" + emailData + "}");
        Serial.println("Sent initial data to WebSocket client");
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_DATA) {
        String msg = "";
        for (size_t i = 0; i < len; i++) {
            msg += (char)data[i];
        }
        Serial.println("Received from WebSocket: " + msg);
        handleWebSocketMessage(msg);
    }
}

void handleWebSocketMessage(String msg) {
    JSONVar doc = JSON.parse(msg);
    if (JSON.typeof(doc) == "undefined") {
        Serial.println("⚠️ Lỗi phân tích JSON!");
        return;
    }

    if (doc.hasOwnProperty("type")) {
        String type = doc["type"];
        JSONVar data = doc["data"];

        if (type == "config") {
            deviceConfig.alertMode = (bool)data["alertMode"];
            deviceConfig.timerCount = (int)data["timerCount"];
            deviceConfig.volume = (int)data["volume"];
            deviceConfig.alertSound = String(data["alertSound"]);
            deviceConfig.normalSound = String(data["normalSound"]);
            deviceConfig.manualOverride = true;

            JSONVar timers = data["timers"];
            if (JSON.typeof(timers) == "array") {
                for (int i = 0; i < deviceConfig.timerCount; i++) {
                    deviceConfig.timers[i].startHour = (int)timers[i]["startHour"];
                    deviceConfig.timers[i].startMinute = (int)timers[i]["startMinute"];
                    deviceConfig.timers[i].endHour = (int)timers[i]["endHour"];
                    deviceConfig.timers[i].endMinute = (int)timers[i]["endMinute"];
                }
            }
            saveConfig(); // Lưu cấu hình vào Preferences ngay khi nhận
            printConfig();
            ws.textAll("{\"type\":\"config\",\"data\":" + createConfigJSON() + "}");
        } else if (type == "email") {
            emailConfig.senderEmail = String(data["senderEmail"]);
            emailConfig.senderPassword = String(data["senderPassword"]);
            emailConfig.recipientEmail = String(data["recipientEmail"]);
            emailConfig.alertContent = String(data["alertContent"]);
            saveEmailConfig(); // Lưu cấu hình email vào Preferences ngay khi nhận
            ws.textAll("{\"type\":\"email\",\"data\":" + createEmailConfigJSON() + "}");
        } else if (type == "clear_history") {
            clearHistory();
            openOnCount = 0;
            openOffCount = 0;
            saveOpenCounts();
            ws.textAll("{\"type\":\"history\",\"data\":" + createHistoryJSON() + "}");
        }
    }
}

void printConfig() {
    Serial.println("\n===== CẤU HÌNH NHẬN ĐƯỢC =====");
    Serial.print("🔔 Chế độ cảnh báo: "); Serial.println(deviceConfig.alertMode ? "Bật" : "Tắt");
    Serial.print("📊 Số lượng hẹn giờ: "); Serial.println(deviceConfig.timerCount);
    for (int i = 0; i < deviceConfig.timerCount; i++) {
        Serial.printf("🕒 Timer %d: %02d:%02d → %02d:%02d\n", i + 1,
                      deviceConfig.timers[i].startHour, deviceConfig.timers[i].startMinute,
                      deviceConfig.timers[i].endHour, deviceConfig.timers[i].endMinute);
    }
    Serial.print("🔊 Âm lượng: "); Serial.println(deviceConfig.volume);
    Serial.print("🎵 Âm báo cảnh báo: "); Serial.println(deviceConfig.alertSound);
    Serial.print("🎶 Âm báo thường: "); Serial.println(deviceConfig.normalSound);
    Serial.print("🔧 Chế độ thủ công: "); Serial.println(deviceConfig.manualOverride ? "Bật" : "Tắt");
    Serial.println("================================\n");
}

String createConfigJSON() {
    JSONVar jsonData;
    jsonData["alertMode"] = deviceConfig.alertMode;
    JSONVar timers;
    for (int i = 0; i < deviceConfig.timerCount; i++) {
        JSONVar timer;
        timer["startHour"] = deviceConfig.timers[i].startHour;
        timer["startMinute"] = deviceConfig.timers[i].startMinute;
        timer["endHour"] = deviceConfig.timers[i].endHour;
        timer["endMinute"] = deviceConfig.timers[i].endMinute;
        timers[i] = timer;
    }
    jsonData["timers"] = timers;
    jsonData["timerCount"] = deviceConfig.timerCount;
    jsonData["volume"] = deviceConfig.volume;
    jsonData["alertSound"] = deviceConfig.alertSound;
    jsonData["normalSound"] = deviceConfig.normalSound;
    jsonData["manualOverride"] = deviceConfig.manualOverride;
    String jsonString = JSON.stringify(jsonData);
    return jsonString;
}

String createHistoryJSON() {
    JSONVar jsonData;
    for (int i = 0; i < historyCount; i++) {
        JSONVar event;
        event["time"] = history[i].time;
        event["date"] = history[i].date;
        event["status"] = history[i].status;
        event["alertMode"] = history[i].alertMode;
        jsonData[i] = event;
    }
    String jsonString = JSON.stringify(jsonData);
    return jsonString;
}

String createEmailConfigJSON() {
    JSONVar jsonData;
    jsonData["senderEmail"] = emailConfig.senderEmail;
    jsonData["senderPassword"] = emailConfig.senderPassword;
    jsonData["recipientEmail"] = emailConfig.recipientEmail;
    jsonData["alertContent"] = emailConfig.alertContent;
    String jsonString = JSON.stringify(jsonData);
    return jsonString;
}

//------------------ Lưu cấu hình email vào preferences------------------
void saveEmailConfig() {
    preferences.begin("emailConfig", false);
    preferences.putString("senderEmail", emailConfig.senderEmail);
    preferences.putString("senderPassword", emailConfig.senderPassword);
    preferences.putString("recipientEmail", emailConfig.recipientEmail);
    preferences.putString("alertContent", emailConfig.alertContent);
    preferences.end();
    Serial.println("Đã lưu cấu hình email vào Preferences");
}

//--------------------- Xử lý nút nhấn ngắn-----------------------
void button_press_short_callback(uint8_t button_id) {
    if (button_id == BUTTON_SET_ID) {
        deviceConfig.alertMode = !deviceConfig.alertMode;
        deviceConfig.manualOverride = true;
        Serial.println("Chế độ cảnh báo: " + String(deviceConfig.alertMode ? "Bật" : "Tắt"));
        saveConfig();
        ws.textAll("{\"type\":\"config\",\"data\":" + createConfigJSON() + "}");
    }
}

//-------------------- Xử lý nút nhấn dài---------------------
void button_press_long_callback(uint8_t button_id) {
    if (button_id == BUTTON_SET_ID) {
        wifi_status = WIFI_DIS;
        clearEeprom();
        ESP.restart();
        Serial.println("Đã reset thiết bị");
    }
}

//-------------------- Đọc EEPROM-----------------------------
void readEEPROM() {
    for (int i = 0; i < 32; ++i) Essid += char(EEPROM.read(i));
    for (int i = 32; i < 64; ++i) Epass += char(EEPROM.read(i));
    if (Essid.length() == 0) Essid = "BLK";
    Serial.println("Đọc từ EEPROM - SSID: " + Essid + ", Password: " + Epass);
}

//-------------------- Xóa EEPROM-------------------------
void clearEeprom() {
    for (int i = 0; i < 250; ++i) EEPROM.write(i, 0);
    EEPROM.commit();
    Serial.println("Đã xóa EEPROM");
}

//---------------------- Ghi EEPROM---------------------------
void writeEEPROM() {
    clearEeprom();
    for (int i = 0; i < Essid.length(); ++i) EEPROM.write(i, Essid[i]);
    for (int i = 0; i < Epass.length(); ++i) EEPROM.write(32 + i, Epass[i]);
    EEPROM.commit();
    Serial.println("Đã ghi vào EEPROM - SSID: " + Essid + ", Password: " + Epass);
    delay(500);
}

//----------------------- Dữ liệu WiFi AP mode-----------------------
String getJsonData() {
    JSONVar myObject;
    myObject["ssid"] = Essid;
    myObject["pass"] = Epass;
    String jsonString = JSON.stringify(myObject);
    Serial.println("Dữ liệu WiFi AP gửi đi: " + jsonString);
    return jsonString;
}

//------------------ Nhận dữ liệu từ client AP mode----------------------------
void getDataFromClient(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    JSONVar myObject = JSON.parse((char *)data);
    if (myObject.hasOwnProperty("ssid")) Essid = (const char*)myObject["ssid"];
    if (myObject.hasOwnProperty("pass")) Epass = (const char*)myObject["pass"];
    Serial.println("Nhận dữ liệu WiFi từ client - SSID: " + Essid + ", Password: " + Epass);
    writeEEPROM();
}