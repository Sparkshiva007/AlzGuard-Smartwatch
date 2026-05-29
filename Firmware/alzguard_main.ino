/*
 * alzguard_main.ino
 * ───────────────
 * Main Entry Point
 */
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

// 1. MUST INCLUDE TYPES FIRST
#include "alzguard_types.h"

// 2. Global State Instance (Do not redefine the struct here!)
DeviceState state;

// 3. Display & Graphics (Standard, no DriveBus)
#include <Arduino_GFX_Library.h>
#include <lvgl.h>

// 4. Sensors (Direct Headers)
#include "SensorQMI8658.hpp"
#include "SensorPCF85063.hpp"
#include <MAX30100_PulseOximeter.h>
#include <Adafruit_AHTX0.h>

// 5. Connectivity
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// 6. Custom Modules
#include "ui_main.h"
#include "fall_detector.h"
#include "ble_manager.h"
#include "wifi_manager.h"
#include "reminder_manager.h"
#include "battery_monitor.h"

// ─── PIN DEFINITIONS ───
#define LCD_DC      4
#define LCD_CS      5
#define LCD_CLK     6
#define LCD_DIN     7
#define LCD_RST     8
#define LCD_BL      15
#define TOUCH_INT   14
#define TOUCH_RST   13
#define I2C_SCL     10
#define I2C_SDA     11
#define BAT_ADC     1
#define SYS_OUT     40
#define SYS_EN      41
#define BUZZER_PIN  42
#define CST816T_ADDR 0x15

#define LCD_WIDTH   240
#define LCD_HEIGHT  280
#define LVGL_TICK_MS 5

// ─── DISPLAY DRIVER (Standard GFX) ───
Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_CLK, LCD_DIN, GFX_NOT_DEFINED);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 0, true, LCD_WIDTH, LCD_HEIGHT, 0, 20);

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[LCD_WIDTH * 20];
static lv_color_t buf2[LCD_WIDTH * 20];
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// ─── SENSORS ───
SensorQMI8658 qmi;
SensorPCF85063 rtc;
PulseOximeter pox;
Adafruit_AHTX0 aht;
Preferences prefs;

// ─── TIMING ───
uint32_t lastSensorReadMs  = 0;
uint32_t lastBatCheckMs    = 0;
uint32_t lastReminderChk   = 0;
uint32_t lastHeartRateMs   = 0;
const uint32_t SCREEN_TIMEOUT_MS = 15000;

// ─── LVGL CALLBACKS ───
void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)color_p, w, h);
    lv_disp_flush_ready(drv);
}

void screenWake() {
    if (state.screenOn) return;
    state.screenOn = true;
    digitalWrite(LCD_BL, HIGH); // Force backlight ON 100%
    lv_disp_trig_activity(NULL);
}

void screenSleep() {
    if (!state.screenOn) return;
    state.screenOn = false;
    digitalWrite(LCD_BL, LOW);  // Force backlight OFF
}

// ─── BARE-METAL TOUCH DRIVER (From successful isolation test!) ───
// ─── CONTINUOUS TOUCH POLLING DRIVER ───
void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    static int32_t last_x = 0;
    static int32_t last_y = 0;
    bool is_touched = false;

    // Constantly poll the I2C bus instead of waiting for the interrupt pin
    Wire.beginTransmission(CST816T_ADDR);
    Wire.write(0x02); // Point to the "Number of Fingers" register
    if (Wire.endTransmission(false) == 0) {
        Wire.requestFrom((uint16_t)CST816T_ADDR, (uint8_t)5);
        if (Wire.available() >= 5) {
            uint8_t fingers = Wire.read();
            uint8_t xh = Wire.read(); uint8_t xl = Wire.read();
            uint8_t yh = Wire.read(); uint8_t yl = Wire.read();
            
            // If the chip says a finger is currently on the glass
            if (fingers > 0) {
                is_touched = true;
                last_x = ((xh & 0x0F) << 8) | xl;
                last_y = ((yh & 0x0F) << 8) | yl;

                if (!state.screenOn) screenWake();
                state.lastTouchMs = millis();
            }
        }
    }

    // Tell LVGL exactly what the finger is doing
    data->state = is_touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = last_x;
    data->point.y = last_y;
}

static void lvgl_tick_timer_cb(TimerHandle_t timer) { lv_tick_inc(LVGL_TICK_MS); }

// Forward declarations for loop calls
void processCommand(const String &json);
void handleFallAlert();

// ─── SETUP ───
void setup() {
    Serial.begin(115200);
    Serial.println("[AlzGuard] Booting...");

    pinMode(SYS_EN,  OUTPUT); digitalWrite(SYS_EN,  HIGH);
    pinMode(SYS_OUT, OUTPUT); digitalWrite(SYS_OUT, HIGH);
    pinMode(BUZZER_PIN, OUTPUT); noTone(BUZZER_PIN);

    // I2C Init
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);

    // Touch Hardware Wakeup
    pinMode(TOUCH_RST, OUTPUT);
    pinMode(TOUCH_INT, INPUT_PULLUP);
    digitalWrite(TOUCH_RST, LOW); delay(50); digitalWrite(TOUCH_RST, HIGH); delay(50);
    Wire.beginTransmission(CST816T_ADDR); Wire.write(0xFE); Wire.write(0x00); Wire.endTransmission();

    // Display Init
    // Display Init
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH); // Turn backlight on IMMEDIATELY
    gfx->begin();
    gfx->fillScreen(BLACK);

    // LVGL Init
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, LCD_WIDTH * 20);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH; disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb; disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER; indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);

    TimerHandle_t tickTimer = xTimerCreate("lvgl_tick", pdMS_TO_TICKS(LVGL_TICK_MS), pdTRUE, NULL, lvgl_tick_timer_cb);
    xTimerStart(tickTimer, 0);

    prefs.begin("alzguard", false);

    // Sensors Init
    if (rtc.begin(Wire, PCF85063_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) { Serial.println("[RTC] OK"); }
    if (qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) {
        qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz, SensorQMI8658::LPF_MODE_0, true);
        qmi.configGyroscope(SensorQMI8658::GYR_RANGE_512DPS, SensorQMI8658::GYR_ODR_896_8Hz, SensorQMI8658::LPF_MODE_3, true);
        qmi.enableGyroscope(); qmi.enableAccelerometer();
        Serial.println("[IMU] OK");
    }
    
    // Uncomment these when you wire the external sensors!
    // if (pox.begin()) { pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA); Serial.println("[MAX30100] OK"); }
    // if (aht.begin()) { Serial.println("[AHT21B] OK"); }

    BLEManager::begin("AlzGuard");
    String ssid = prefs.getString("wifi_ssid", "");
    String pass = prefs.getString("wifi_pass", "");
    if (ssid.length() > 0) WiFiManager::connect(ssid, pass);

    ReminderManager::begin(&prefs);
    FallDetector::begin();
    BatteryMonitor::begin(BAT_ADC);
    UI::begin();


    // ─── PREMIUM BOOT CHIME ───
    // A fast, optimistic ascending "Power On" melody (C5, E5, G5, C6)
    int bootMelody[] = {523, 659, 784, 1047}; 
    int bootDurations[] = {100, 100, 100, 400}; 
    for (int i = 0; i < 4; i++) {
        tone(BUZZER_PIN, bootMelody[i], bootDurations[i]);
        delay(bootDurations[i] + 20); // 20ms gap for crispness
    }
    noTone(BUZZER_PIN);
    state.lastTouchMs = millis();
    Serial.println("[AlzGuard] Ready!");
}

// ─── MAIN LOOP ───
void loop() {
    uint32_t now = millis();
    lv_timer_handler();

    // Screen Timeout
    if (state.screenOn && (now - state.lastTouchMs > SCREEN_TIMEOUT_MS)) { screenSleep(); }

    // Raise-to-wake IMU logic
    if (!state.screenOn && (now - state.lastRaiseMs > 500)) {
        float az = abs(state.accelZ); float ax = abs(state.accelX);
        if (ax > 0.6f && az < 0.6f) { screenWake(); state.lastTouchMs = now; }
        state.lastRaiseMs = now;
    }

    // Fast Sensors (IMU for fall detection)
    if (now - lastSensorReadMs >= 50) {
        lastSensorReadMs = now;
        if (qmi.getDataReady()) {
            qmi.getAccelerometer(state.accelX, state.accelY, state.accelZ);
            qmi.getGyroscope(state.gyroX, state.gyroY, state.gyroZ);
        }
        FallDetector::update(state.accelX, state.accelY, state.accelZ, state.gyroX, state.gyroY, state.gyroZ);
        if (FallDetector::fallOccurred()) { state.fallDetected = true; state.lastFallTime = now; handleFallAlert(); }
    }

    // Heart Rate polling
    if (now - lastHeartRateMs >= 20) {
        lastHeartRateMs = now; 
        // pox.update(); 
        // state.heartRate = pox.getHeartRate(); 
        // state.spo2 = pox.getSpO2();
    }

    // Slow Sensors (Battery, Temp/Hum, RTC)
    if (now - lastBatCheckMs >= 5000) {
        lastBatCheckMs = now;
        BatteryMonitor::update();
        state.batteryPct = BatteryMonitor::percentage();
        state.batteryVolts = BatteryMonitor::voltage();

        // sensors_event_t humidity_evt, temp_evt;
        // if (aht.getEvent(&humidity_evt, &temp_evt)) { state.temperature = temp_evt.temperature; state.humidity = humidity_evt.relative_humidity; }

        // Fetch time directly from sensor methods
// Fetch time gracefully using 'auto' to bypass library struct naming issues
        auto dt = rtc.getDateTime();
        state.year   = dt.year % 100;
        state.month  = dt.month;
        state.day    = dt.day;
        state.hour   = dt.hour;
        state.minute = dt.minute;
        state.second = dt.second;

        UI::updateVitals(state);
        if (state.batteryPct < 15.0f) { UI::showBatteryWarning(state.batteryPct); }
    }

    // Reminders
    if (now - lastReminderChk >= 30000) {
        lastReminderChk = now; ReminderManager::tick(state.hour, state.minute);
    }

    BLEManager::loop(state);
    state.bleConnected = BLEManager::isConnected();
    state.wifiConnected = (WiFi.status() == WL_CONNECTED);

    if (BLEManager::hasCommand()) { processCommand(BLEManager::getCommand()); }

    delay(5);
}

void processCommand(const String &json) {
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, json) != DeserializationError::Ok) return;

    const char *cmd = doc["cmd"];
    if (!cmd) return;
    if (strcmp(cmd, "set_time") == 0) {
        rtc.setDateTime(doc["year"], doc["month"], doc["day"], doc["hour"], doc["minute"], doc["second"]);
        Serial.println("[CMD] Time set");
    }
    else if (strcmp(cmd, "add_reminder") == 0) {
        Reminder r; r.id = doc["id"] | 0; r.hour = doc["hour"] | 0; r.minute = doc["min"] | 0;
        r.type = (ReminderType)(doc["type"] | 0); r.enabled = true;
        strlcpy(r.label, doc["label"] | "Reminder", sizeof(r.label)); r.repeatDays = doc["days"] | 0x7F;
        ReminderManager::add(r); UI::refreshReminders();
    }
    else if (strcmp(cmd, "del_reminder") == 0) { ReminderManager::remove(doc["id"] | 0); UI::refreshReminders(); }
    else if (strcmp(cmd, "set_wifi") == 0) {
        String ssid = doc["ssid"] | ""; String pass = doc["pass"] | "";
        prefs.putString("wifi_ssid", ssid); prefs.putString("wifi_pass", pass);
        WiFiManager::connect(ssid, pass);
    }
    else if (strcmp(cmd, "alert_ack") == 0) { UI::dismissFallAlert(); noTone(BUZZER_PIN); }
    else if (strcmp(cmd, "buzz_test") == 0) { tone(BUZZER_PIN, 1000, 500); }
    else if (strcmp(cmd, "get_status") == 0) { BLEManager::sendStatus(state); }
}

void handleFallAlert() {
    Serial.println("[ALERT] FALL DETECTED!");
    screenWake(); state.lastTouchMs = millis(); UI::showFallAlert();
    for (int i = 0; i < 3; i++) { tone(BUZZER_PIN, 1400, 200); delay(250); tone(BUZZER_PIN, 1800, 200); delay(250); }
    noTone(BUZZER_PIN);
    BLEManager::sendFallAlert(state);
    if (WiFi.status() == WL_CONNECTED) { WiFiManager::sendFallNotification(state); }
}