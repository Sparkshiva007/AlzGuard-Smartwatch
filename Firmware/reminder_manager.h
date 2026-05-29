#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "alzguard_types.h"

// ── Forward declarations for external modules ──
namespace UI {
    void showReminderAlert(const char *label, ReminderType type);
    void refreshReminders();
}
namespace BLEManager {
    void sendReminderAlert(const char *label);
}
namespace WiFiManager {
    void sendReminderNotification(const char *label, uint8_t h, uint8_t m);
}

extern void tone(uint8_t pin, unsigned int frequency, unsigned long duration);
extern void noTone(uint8_t pin);

// ── THE FIX: Declared in global scope so the linker finds it! ──
extern void screenWake(); 

#define BUZZER_PIN 42

namespace ReminderManager {

void triggerReminder(const Reminder &r);

static const uint8_t MAX_REMINDERS = 20;
static Reminder reminders[MAX_REMINDERS];
static uint8_t  reminderCount = 0;
static Preferences *_prefs    = nullptr;

static const int TONE_FREQ[]  = {880, 660, 550, 770, 440, 1000};
static const int TONE_DUR[]   = {200, 150, 100, 180, 120, 250};

void _saveToNVS() {
    if (!_prefs) return;
    StaticJsonDocument<2048> doc;
    JsonArray arr = doc.createNestedArray("rem");
    for (uint8_t i = 0; i < reminderCount; i++) {
        JsonObject o = arr.createNestedObject();
        o["id"]   = reminders[i].id;
        o["h"]    = reminders[i].hour;
        o["m"]    = reminders[i].minute;
        o["lbl"]  = reminders[i].label;
        o["typ"]  = (uint8_t)reminders[i].type;
        o["days"] = reminders[i].repeatDays;
        o["en"]   = reminders[i].enabled;
    }
    String s;
    serializeJson(doc, s);
    _prefs->putString("reminders", s);
}

void _loadFromNVS() {
    if (!_prefs) return;
    String s = _prefs->getString("reminders", "");
    if (s.length() < 5) return;

    StaticJsonDocument<2048> doc;
    if (deserializeJson(doc, s) != DeserializationError::Ok) return;

    JsonArray arr = doc["rem"];
    reminderCount = 0;
    for (JsonObject o : arr) {
        if (reminderCount >= MAX_REMINDERS) break;
        reminders[reminderCount].id          = o["id"]   | 0;
        reminders[reminderCount].hour        = o["h"]    | 0;
        reminders[reminderCount].minute      = o["m"]    | 0;
        reminders[reminderCount].type        = (ReminderType)(o["typ"] | 0);
        reminders[reminderCount].repeatDays  = o["days"] | 0x7F;
        reminders[reminderCount].enabled     = o["en"]   | true;
        reminders[reminderCount].firedToday  = false;
        strlcpy(reminders[reminderCount].label, o["lbl"] | "Reminder", sizeof(reminders[reminderCount].label));
        reminderCount++;
    }
    Serial.printf("[REM] Loaded %d reminders\n", reminderCount);
}

void begin(Preferences *prefs) {
    _prefs = prefs;
    _loadFromNVS();
}

void add(const Reminder &r) {
    for (uint8_t i = 0; i < reminderCount; i++) {
        if (reminders[i].id == r.id) {
            reminders[i] = r;
            _saveToNVS();
            return;
        }
    }
    if (reminderCount < MAX_REMINDERS) {
        reminders[reminderCount++] = r;
        _saveToNVS();
    }
}

void remove(uint8_t id) {
    for (uint8_t i = 0; i < reminderCount; i++) {
        if (reminders[i].id == id) {
            for (uint8_t j = i; j < reminderCount - 1; j++) {
                reminders[j] = reminders[j + 1];
            }
            reminderCount--;
            _saveToNVS();
            return;
        }
    }
}

uint8_t getAll(Reminder **out) {
    *out = reminders;
    return reminderCount;
}

void triggerReminder(const Reminder &r) {
    Serial.printf("[REM] Firing: %s at %02d:%02d\n", r.label, r.hour, r.minute);

    // Call the global screen wake function
    screenWake();

    UI::showReminderAlert(r.label, r.type);

    int freq = TONE_FREQ[(uint8_t)r.type];
    int dur  = TONE_DUR[(uint8_t)r.type];
    for (int p = 0; p < 4; p++) {
        tone(BUZZER_PIN, freq, dur); delay(dur + 60);
        tone(BUZZER_PIN, freq + 200, dur); delay(dur + 60);
    }
    noTone(BUZZER_PIN);

    BLEManager::sendReminderAlert(r.label);
    WiFiManager::sendReminderNotification(r.label, r.hour, r.minute);
}

void tick(uint8_t hour, uint8_t minute) {
    if (hour == 0 && minute == 0) {
        for (uint8_t i = 0; i < reminderCount; i++) reminders[i].firedToday = false;
    }
    for (uint8_t i = 0; i < reminderCount; i++) {
        Reminder &r = reminders[i];
        if (!r.enabled || r.firedToday) continue;
        if (r.hour == hour && r.minute == minute) {
            r.firedToday = true;
            triggerReminder(r);
        }
    }
}

String toJson() {
    StaticJsonDocument<2048> doc;
    JsonArray arr = doc.createNestedArray("reminders");
    for (uint8_t i = 0; i < reminderCount; i++) {
        JsonObject o = arr.createNestedObject();
        o["id"]    = reminders[i].id; o["hour"]  = reminders[i].hour;
        o["min"]   = reminders[i].minute; o["label"] = reminders[i].label;
        o["type"]  = (uint8_t)reminders[i].type; o["days"]  = reminders[i].repeatDays;
        o["en"]    = reminders[i].enabled;
    }
    String s; serializeJson(doc, s);
    return s;
}

} // namespace ReminderManager