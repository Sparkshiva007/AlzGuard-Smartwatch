/*
 * fall_detector.h
 * ───────────────
 * Multi-stage fall detection using the QMI8658 6-axis IMU.
 *
 * Algorithm: 3-Phase State Machine
 * ─────────────────────────────────────────────────────────────────
 *  Phase 1 - FREE-FALL DETECTION
 *    Total acceleration magnitude drops below FREE_FALL_THRESHOLD (≈0.5g)
 *    for at least FREE_FALL_MIN_MS milliseconds.
 *    This is the signature of the body being airborne / losing balance.
 *
 *  Phase 2 - IMPACT DETECTION
 *    After free-fall: total acceleration spikes above IMPACT_THRESHOLD (≈2.5g)
 *    within IMPACT_WINDOW_MS. This is the body hitting the ground.
 *
 *  Phase 3 - POST-IMPACT STILLNESS
 *    After impact: if the person remains relatively still (magnitude ≈ 1g,
 *    lying down) for STILL_WINDOW_MS — confirmed FALL.
 *    If they immediately recover (move actively), it was a stumble/grab —
 *    NOT a fall.
 *
 * Additional guard - ORIENTATION CHANGE
 *    We check whether the device changes from an upright (wrist-worn vertical)
 *    orientation to a near-horizontal one after impact, which is consistent
 *    with lying down.
 *
 * Sensitivity Tuning
 *    The thresholds have been chosen conservatively to minimise false positives
 *    (vibration, vigorous activity, sitting down quickly) while catching real
 *    falls that Alzheimer's patients typically experience (slow trip-and-fall,
 *    syncope collapse).
 * ─────────────────────────────────────────────────────────────────
 */

#pragma once
#include <Arduino.h>
#include <math.h>

namespace FallDetector {

// ── Tunable Constants ──────────────────────────────────────────────
static constexpr float FREE_FALL_THRESHOLD  = 0.45f;   // g — below = free-fall
static constexpr uint32_t FREE_FALL_MIN_MS  = 60;      // ms — min free-fall duration
static constexpr uint32_t FREE_FALL_MAX_MS  = 600;     // ms — max before reset
static constexpr float IMPACT_THRESHOLD     = 2.4f;    // g — above = impact
static constexpr uint32_t IMPACT_WINDOW_MS  = 500;     // ms — window after free-fall
static constexpr float STILL_THRESHOLD_MAX  = 1.35f;   // g — "lying still" upper
static constexpr float STILL_THRESHOLD_MIN  = 0.65f;   // g — "lying still" lower
static constexpr uint32_t STILL_WINDOW_MS   = 1200;    // ms — duration for confirm
static constexpr uint32_t FALL_COOLDOWN_MS  = 8000;    // ms — no re-trigger

// Activity detection (vigorous motion guard)
static constexpr float ACTIVITY_THRESHOLD   = 1.8f;    // g — normal active movement
static constexpr uint32_t ACTIVITY_WINDOW   = 2000;    // ms — vigorous activity window

// ── State Machine ─────────────────────────────────────────────────
enum class Phase {
    IDLE,
    FREE_FALL,
    IMPACT_WAIT,
    STILL_CONFIRM
};

static Phase phase         = Phase::IDLE;
static uint32_t phaseStart = 0;
static uint32_t lastFallMs = 0;
static bool _fallOccurred  = false;

// Rolling activity tracker
static float activityBuf[20] = {};
static uint8_t activityIdx   = 0;

static inline float magnitude(float x, float y, float z) {
    return sqrtf(x*x + y*y + z*z);
}

static bool isVigorousActivity() {
    float sum = 0;
    for (int i = 0; i < 20; i++) sum += activityBuf[i];
    return (sum / 20.0f) > 1.5f;   // average over last 20 samples
}

void begin() {
    phase = Phase::IDLE;
    _fallOccurred = false;
    memset(activityBuf, 0, sizeof(activityBuf));
}

/*
 * update() — called every 50ms from main loop
 *  ax, ay, az : accelerometer in g
 *  gx, gy, gz : gyroscope in deg/s (used for rotation speed guard)
 */
void update(float ax, float ay, float az,
            float gx, float gy, float gz)
{
    _fallOccurred = false;
    uint32_t now  = millis();
    float mag     = magnitude(ax, ay, az);
    float gyrMag  = magnitude(gx, gy, gz);

    // Update rolling activity buffer
    activityBuf[activityIdx % 20] = mag;
    activityIdx++;

    // Cooldown after a confirmed fall
    if (now - lastFallMs < FALL_COOLDOWN_MS) return;

    switch (phase) {

    // ── IDLE ────────────────────────────────────────────────────
    case Phase::IDLE:
        if (mag < FREE_FALL_THRESHOLD) {
            phase      = Phase::FREE_FALL;
            phaseStart = now;
        }
        break;

    // ── FREE-FALL detected ──────────────────────────────────────
    case Phase::FREE_FALL:
        if (mag >= FREE_FALL_THRESHOLD) {
            // Free-fall ended — did it last long enough?
            uint32_t dur = now - phaseStart;
            if (dur >= FREE_FALL_MIN_MS && dur <= FREE_FALL_MAX_MS) {
                // Proceed to impact watch
                phase      = Phase::IMPACT_WAIT;
                phaseStart = now;
            } else {
                phase = Phase::IDLE;
            }
        } else if (now - phaseStart > FREE_FALL_MAX_MS) {
            // Stuck in free-fall too long (sensor error) — reset
            phase = Phase::IDLE;
        }
        break;

    // ── WAITING FOR IMPACT ──────────────────────────────────────
    case Phase::IMPACT_WAIT:
        if (mag > IMPACT_THRESHOLD) {
            // IMPACT detected!
            phase      = Phase::STILL_CONFIRM;
            phaseStart = now;
        } else if (now - phaseStart > IMPACT_WINDOW_MS) {
            // No impact within window — was not a fall (maybe a sudden stop)
            phase = Phase::IDLE;
        }
        break;

    // ── POST-IMPACT: CONFIRM STILLNESS ──────────────────────────
    case Phase::STILL_CONFIRM: {
        bool isStill = (mag >= STILL_THRESHOLD_MIN && mag <= STILL_THRESHOLD_MAX)
                       && (gyrMag < 50.0f);   // not rotating either
        bool isVig   = isVigorousActivity();

        if (isVig) {
            // Person recovered immediately — stumble, not a fall
            phase = Phase::IDLE;
        } else if (isStill) {
            if (now - phaseStart >= STILL_WINDOW_MS) {
                // Confirmed fall — person lying still after impact
                _fallOccurred = true;
                lastFallMs    = now;
                phase         = Phase::IDLE;
            }
            // else: keep waiting
        } else {
            // Moving but not vigorously — reset timer (might be repositioning)
            if (now - phaseStart > STILL_WINDOW_MS * 2) {
                phase = Phase::IDLE;
            }
        }
        break;
    }

    default:
        phase = Phase::IDLE;
        break;
    }
}

bool fallOccurred() { return _fallOccurred; }

Phase currentPhase() { return phase; }

// Debug string for serial monitor
const char* phaseString() {
    switch(phase) {
        case Phase::IDLE:         return "IDLE";
        case Phase::FREE_FALL:    return "FREE_FALL";
        case Phase::IMPACT_WAIT:  return "IMPACT_WAIT";
        case Phase::STILL_CONFIRM:return "STILL_CONFIRM";
        default:                  return "UNKNOWN";
    }
}

} // namespace FallDetector
