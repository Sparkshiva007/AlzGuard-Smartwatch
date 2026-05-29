/*
 * ble_manager.h
 * ─────────────
 * BLE GATT Server for AlzGuard.
 *
 * Custom Service UUID: 12345678-1234-1234-1234-123456789ABC
 *
 * Characteristics:
 *  TX  (notify)  : Device → App  (JSON status packets, alerts)
 *  RX  (write)   : App → Device  (commands: add_reminder, set_time, etc.)
 *  HR  (notify)  : Heart rate + SpO2 (fast updates, 1s interval)
 *  FALL(notify)  : Fall alert
 */

#pragma once
#include <Arduino.h>
#include "alzguard_types.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// ─── UUIDs ────────────────────────────────────────────────────────
#define ALZGUARD_SERVICE_UUID   "12345678-1234-1234-1234-123456789ABC"
#define CHAR_TX_UUID            "12345678-1234-1234-1234-123456789AB1"   // Notify: status
#define CHAR_RX_UUID            "12345678-1234-1234-1234-123456789AB2"   // Write: commands
#define CHAR_VITALS_UUID        "12345678-1234-1234-1234-123456789AB3"   // Notify: vitals
#define CHAR_FALL_UUID          "12345678-1234-1234-1234-123456789AB4"   // Notify: fall alert

namespace BLEManager {

static BLEServer          *pServer      = nullptr;
static BLECharacteristic  *pTxChar     = nullptr;
static BLECharacteristic  *pRxChar     = nullptr;
static BLECharacteristic  *pVitalsChar = nullptr;
static BLECharacteristic  *pFallChar   = nullptr;

static bool     _connected   = false;
static String   _pendingCmd  = "";
static bool     _hasCommand  = false;
static uint32_t lastVitalSend = 0;

// Server callbacks
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *srv) override {
        _connected = true;
        Serial.println("[BLE] Companion connected");
    }
    void onDisconnect(BLEServer *srv) override {
        _connected = false;
        Serial.println("[BLE] Companion disconnected — restarting advertising");
        delay(500);
        srv->startAdvertising();
    }
};

// RX characteristic callbacks
class RxCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *ch) override {
        _pendingCmd = ch->getValue().c_str();
        _hasCommand = (_pendingCmd.length() > 0);
        Serial.printf("[BLE RX] %s\n", _pendingCmd.c_str());
    }
};

void begin(const char *deviceName)
{
    BLEDevice::init(deviceName);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService *svc = pServer->createService(ALZGUARD_SERVICE_UUID);

    // TX — notify device status to app
    pTxChar = svc->createCharacteristic(CHAR_TX_UUID,
                  BLECharacteristic::PROPERTY_NOTIFY);
    pTxChar->addDescriptor(new BLE2902());

    // RX — receive commands from app
    pRxChar = svc->createCharacteristic(CHAR_RX_UUID,
                  BLECharacteristic::PROPERTY_WRITE |
                  BLECharacteristic::PROPERTY_WRITE_NR);
    pRxChar->setCallbacks(new RxCallbacks());

    // Vitals — fast HR/SpO2/temp
    pVitalsChar = svc->createCharacteristic(CHAR_VITALS_UUID,
                      BLECharacteristic::PROPERTY_NOTIFY);
    pVitalsChar->addDescriptor(new BLE2902());

    // Fall Alert
    pFallChar = svc->createCharacteristic(CHAR_FALL_UUID,
                    BLECharacteristic::PROPERTY_NOTIFY);
    pFallChar->addDescriptor(new BLE2902());

    svc->start();

    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(ALZGUARD_SERVICE_UUID);
    adv->setScanResponse(true);
    adv->setMinPreferred(0x06);
    BLEDevice::startAdvertising();

    Serial.printf("[BLE] Advertising as '%s'\n", deviceName);
}

void loop(const DeviceState &s)
{
    if (!_connected) return;
    uint32_t now = millis();

    // Send vitals every 1 second
    if (now - lastVitalSend >= 1000) {
        lastVitalSend = now;

        StaticJsonDocument<256> doc;
        doc["hr"]   = (int)s.heartRate;
        doc["spo2"] = (int)s.spo2;
        doc["temp"] = s.temperature;
        doc["hum"]  = s.humidity;
        doc["bat"]  = (int)s.batteryPct;
        doc["h"]    = s.hour;
        doc["m"]    = s.minute;

        String out;
        serializeJson(doc, out);
        pVitalsChar->setValue(out.c_str());
        pVitalsChar->notify();
    }
}

void sendStatus(const DeviceState &s)
{
    if (!_connected) return;
    StaticJsonDocument<512> doc;
    doc["type"]  = "status";
    doc["hr"]    = (int)s.heartRate;
    doc["spo2"]  = (int)s.spo2;
    doc["temp"]  = s.temperature;
    doc["hum"]   = s.humidity;
    doc["bat"]   = (int)s.batteryPct;
    doc["batV"]  = s.batteryVolts;
    doc["wifi"]  = s.wifiConnected;
    doc["fall"]  = s.fallDetected;
    doc["h"]     = s.hour;
    doc["m"]     = s.minute;
    doc["sec"]   = s.second;

    String out;
    serializeJson(doc, out);
    pTxChar->setValue(out.c_str());
    pTxChar->notify();
}

void sendFallAlert(const DeviceState &s)
{
    if (!_connected) return;
    StaticJsonDocument<128> doc;
    doc["type"] = "fall";
    doc["bat"]  = (int)s.batteryPct;
    doc["h"]    = s.hour;
    doc["m"]    = s.minute;

    String out;
    serializeJson(doc, out);
    pFallChar->setValue(out.c_str());
    pFallChar->notify();
    Serial.println("[BLE] Fall alert sent");
}

void sendReminderAlert(const char *label)
{
    if (!_connected) return;
    StaticJsonDocument<128> doc;
    doc["type"]  = "reminder";
    doc["label"] = label;

    String out;
    serializeJson(doc, out);
    pTxChar->setValue(out.c_str());
    pTxChar->notify();
}

bool isConnected()  { return _connected;   }
bool hasCommand()   { return _hasCommand;  }
String getCommand() {
    _hasCommand = false;
    return _pendingCmd;
}

} // namespace BLEManager
