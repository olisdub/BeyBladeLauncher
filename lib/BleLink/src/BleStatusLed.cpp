#include "BleStatusLed.h"

BleStatusLed::BleStatusLed(uint8_t pin, bool activeHigh)
: _pin(pin), _activeHigh(activeHigh) {}

void BleStatusLed::begin() {
    pinMode(_pin, OUTPUT);
    writePin(false);
}

void BleStatusLed::setConnected(bool connected) {
    _connected = connected;
    // Reset blink when state changes
    _lastToggle = millis();
    _blinkState = false;
}

void BleStatusLed::writePin(bool on) {
    if (_activeHigh) {
        digitalWrite(_pin, on ? HIGH : LOW);
    } else {
        digitalWrite(_pin, on ? LOW : HIGH);
    }
}

void BleStatusLed::update() {
    if (_connected) {
        writePin(true);
        return;
    }

    unsigned long now = millis();
    if (now - _lastToggle >= _blinkPeriodMs) {
        _lastToggle = now;
        _blinkState = !_blinkState;
        writePin(_blinkState);
    }
}
