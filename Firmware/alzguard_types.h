#pragma once
#include <Arduino.h>

// ─── Reminder Types ───
enum class ReminderType : uint8_t {
    MEDICINE   = 0,
    WATER      = 1,
    EXERCISE   = 2,
    MEAL       = 3,
    SLEEP      = 4,
    CUSTOM     = 5
};

struct Reminder {
    uint8_t  id;
    uint8_t  hour;
    uint8_t  minute;
    char     label[48];
    ReminderType type;
    uint8_t  repeatDays;   
    bool     enabled;
    bool     firedToday;   
};

// ─── Global Device State ───
struct DeviceState {
    float heartRate    = 0;
    float spo2         = 0;
    float temperature  = 0;
    float humidity     = 0;
    float batteryPct   = 0;
    float batteryVolts = 0;

    float accelX = 0, accelY = 0, accelZ = 0;
    float gyroX  = 0, gyroY  = 0, gyroZ  = 0;
    bool  fallDetected = false;
    uint32_t lastFallTime = 0;

    bool  screenOn       = true;
    uint32_t lastTouchMs = 0;
    uint32_t lastRaiseMs = 0;

    bool bleConnected  = false;
    bool wifiConnected = false;

    uint8_t hour = 12, minute = 0, second = 0;
    uint8_t day  = 1,  month  = 1,  year   = 25;
};