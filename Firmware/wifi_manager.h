/*
 * wifi_manager.h
 * ──────────────
 * WiFi connectivity + HTTP push notifications to companion backend.
 * Uses a simple REST API (self-hosted or Firebase) for cloud alerts.
 * Falls back gracefully when no WiFi available.
 */

#pragma once
#include <Arduino.h>
#include "alzguard_types.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// ─── Backend Configuration ────────────────────────────────────────
// IMPORTANT: Change BACKEND_URL to your server or Firebase endpoint
// The device ID must match in the companion app
#define BACKEND_URL     "https://your-backend.com/api"
#define DEVICE_ID       "ALZGUARD_001"          // Unique per device

namespace WiFiManager {

static bool _connecting = false;
static uint32_t _connStartMs = 0;

void connect(const String &ssid, const String &pass) {
    if (ssid.length() == 0) return;
    _connecting = true;
    _connStartMs = millis();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.printf("[WiFi] Connecting to %s...\n", ssid.c_str());

    // Non-blocking: connection checked in loop()
}

bool isConnected() { return WiFi.status() == WL_CONNECTED; }

void _postJson(const char *endpoint, const String &payload) {
    if (!isConnected()) return;
    HTTPClient http;
    String url = String(BACKEND_URL) + endpoint;
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Device-ID", DEVICE_ID);
    int code = http.POST(payload);
    Serial.printf("[HTTP] POST %s → %d\n", endpoint, code);
    http.end();
}

void sendFallNotification(const DeviceState &s) {
    StaticJsonDocument<256> doc;
    doc["type"]    = "fall";
    doc["device"]  = DEVICE_ID;
    doc["hour"]    = s.hour;
    doc["minute"]  = s.minute;
    doc["battery"] = (int)s.batteryPct;
    doc["hr"]      = (int)s.heartRate;

    String payload;
    serializeJson(doc, payload);
    _postJson("/alert/fall", payload);
}

void sendReminderNotification(const char *label, uint8_t h, uint8_t m) {
    StaticJsonDocument<256> doc;
    doc["type"]   = "reminder_fired";
    doc["device"] = DEVICE_ID;
    doc["label"]  = label;
    doc["hour"]   = h;
    doc["minute"] = m;

    String payload;
    serializeJson(doc, payload);
    _postJson("/alert/reminder", payload);
}

// Sync time from NTP when connected
void syncNTP() {
    if (!isConnected()) return;
    configTime(0, 0, "pool.ntp.org", "time.google.com");
    Serial.println("[WiFi] NTP sync requested");
}

} // namespace WiFiManager
