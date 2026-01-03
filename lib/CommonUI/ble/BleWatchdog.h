#pragma once

#include <Arduino.h>

/**
 * @brief Simple non-blocking watchdog for loop()-driven tasks.
 *
 * - No dynamic allocation
 * - Uses millis()
 * - No BLE dependency
 * - Optional logging hook (function pointer, no Serial inside)
 */
class BleWatchdog {
public:
    using LogFn = void (*)(const char* msg);

    BleWatchdog(uint32_t timeoutMs, uint32_t warnThresholdMs, LogFn logFn = nullptr)
        : timeoutMs_(timeoutMs),
          warnThresholdMs_(warnThresholdMs),
          logFn_(logFn),
          lastKickMs_(millis()) {}

    // Reset the timer and clear expired/warned flags
    void kick() {
        lastKickMs_ = millis();
        expired_ = false;
        warned_ = false;
    }

    // Call regularly from loop(); non-blocking
    void update() {
        if (expired_) return;

        const uint32_t now = millis();
        const uint32_t elapsed = now - lastKickMs_;

        if (elapsed >= timeoutMs_) {
            expired_ = true;
            log("watchdog expired");
            return;
        }

        if (!warned_ && warnThresholdMs_ > 0 && elapsed >= timeoutMs_ - warnThresholdMs_) {
            warned_ = true;
            log("watchdog near timeout");
        }
    }

    bool isExpired() const {
        return expired_;
    }

    bool isNearTimeout() const {
        if (expired_) return false;
        const uint32_t elapsed = millis() - lastKickMs_;
        return warnThresholdMs_ > 0 && elapsed >= timeoutMs_ - warnThresholdMs_;
    }

    void setLogger(LogFn logFn) {
        logFn_ = logFn;
    }

private:
    void log(const char* msg) {
        if (logFn_) logFn_(msg);
    }

    uint32_t timeoutMs_;
    uint32_t warnThresholdMs_;
    LogFn logFn_;
    uint32_t lastKickMs_;
    bool expired_ = false;
    bool warned_ = false;
};
