/*
 * battery_monitor.h
 * ─────────────────
 * Battery voltage measurement via ADC divider on GPIO1 (BAT_ADC).
 */

#pragma once
#include <Arduino.h>

namespace BatteryMonitor {

// ── THE FIX: Forward declare update() so begin() can use it! ──
void update();

static uint8_t _pin          = 1;   // BAT_ADC = GPIO1
static float   _voltage      = 4.0f;
static float   _percentage   = 100.0f;

// LiPo voltage → percentage lookup table (10 points)
static const float VOLT_TABLE[] = { 3.00f, 3.50f, 3.60f, 3.70f, 3.75f,
                                    3.80f, 3.90f, 4.00f, 4.10f, 4.20f };
static const float PCT_TABLE[]  = {  0.0f, 5.0f, 10.0f, 20.0f, 30.0f,
                                    50.0f, 70.0f, 85.0f, 95.0f, 100.0f };

static float voltToPercent(float v) {
    if (v <= VOLT_TABLE[0]) return 0.0f;
    if (v >= VOLT_TABLE[9]) return 100.0f;
    for (int i = 0; i < 9; i++) {
        if (v >= VOLT_TABLE[i] && v < VOLT_TABLE[i+1]) {
            float t = (v - VOLT_TABLE[i]) / (VOLT_TABLE[i+1] - VOLT_TABLE[i]);
            return PCT_TABLE[i] + t * (PCT_TABLE[i+1] - PCT_TABLE[i]);
        }
    }
    return 50.0f;
}

void begin(uint8_t pin) {
    _pin = pin;
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);   // Full range 0-3.3V
    update(); // Now the compiler knows what this is!
}

void update() {
    // Average 16 samples to reduce noise
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) {
        sum += analogRead(_pin);
        delay(1);
    }
    float adcAvg = sum / 16.0f;

    // VADC = (adcAvg / 4095) * 3.3
    // VBAT = VADC * 3   (voltage divider: 200K + 100K, ratio = 3)
    _voltage    = (adcAvg / 4095.0f) * 3.3f * 3.0f;
    _percentage = voltToPercent(_voltage);

    Serial.printf("[BAT] %.2fV → %.0f%%\n", _voltage, _percentage);
}

float voltage()    { return _voltage;    }
float percentage() { return _percentage; }

bool isCharging() {
    return _voltage > 4.15f;
}

bool isCritical()  { return _percentage < 5.0f;  }
bool isLow()       { return _percentage < 15.0f; }

} // namespace BatteryMonitor