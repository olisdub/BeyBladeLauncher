#pragma once
#include <Arduino.h>

// Simple single-pin status LED helper.
// - Not connected: slow blink
// - Connected: steady ON
class BleStatusLed {
public:
    explicit BleStatusLed(uint8_t pin, bool activeHigh = true);

    void begin();
    void setConnected(bool connected);

    // Call in loop()
    void update();

    // Optional tuning
    void setBlinkPeriodMs(uint16_t periodMs) { _blinkPeriodMs = periodMs; }

private:
    void writePin(bool on);

    uint8_t _pin;
    bool _activeHigh;
    bool _connected = false;

    uint16_t _blinkPeriodMs = 800;
    unsigned long _lastToggle = 0;
    bool _blinkState = false;
};
