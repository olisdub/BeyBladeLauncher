#pragma once
#include <Arduino.h>
#include <functional>
#include "BleConnectionState.h"

// Base class providing connection state + callback wiring.
class BleLink {
public:
    using StateCallback = std::function<void(BleConnectionState)>;

    virtual ~BleLink() = default;

    // Start BLE role (implemented by derived class)
    virtual void begin(const char* deviceName) = 0;

    // Call periodically from loop()
    virtual void update() = 0;

    bool isConnected() const { return _connected; }
    BleConnectionState state() const { return _state; }

    // Called whenever state changes
    void onStateChange(StateCallback cb) { _callback = cb; }

protected:
    void setState(BleConnectionState st);
    void setConnected(bool connected);

private:
    bool _connected = false;
    BleConnectionState _state = BleConnectionState::IDLE;
    StateCallback _callback = nullptr;
};
