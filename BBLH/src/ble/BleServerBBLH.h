#pragma once

#include <Arduino.h>
#include <NimBLEAddress.h>
#include <NimBLEDevice.h>
#include <functional>

// Uses the same enum as BBLC/BleStatus (important for LED and coherence)
#include "ble/BleStatus.h"

class BleServerBBLH {
public:
    using StateCallback = std::function<void(BleState)>;

    // Application callback when a command is received (raw payload)
    using CommandCallback = std::function<void(const uint8_t* data, size_t len)>;

    BleServerBBLH();

    void begin();
    void loop(); // currently empty, kept for symmetry and future evolution

    void onStateChange(StateCallback cb);
    void onCommand(CommandCallback cb);

    BleState getState() const { return state_; }

    // Send a status notification to the client
    void notifyStatus(const char* text);

    // Local BLE server address
    const NimBLEAddress& getServerAddress() const {
        return serverAddress_;
    }

private:
    void setState(BleState s);

    void setupGatt();
    void startAdvertising();

    // ====== NimBLE Callbacks ======
    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        explicit ServerCallbacks(BleServerBBLH& parent) : parent_(parent) {}
        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
    private:
        BleServerBBLH& parent_;
    };

    class CmdCallbacks : public NimBLECharacteristicCallbacks {
    public:
        explicit CmdCallbacks(BleServerBBLH& parent) : parent_(parent) {}
        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
    private:
        BleServerBBLH& parent_;
    };

private:
    BleState state_ = BleState::BOOT;
    StateCallback stateCb_;
    CommandCallback cmdCb_;

    NimBLEServer* server_ = nullptr;
    NimBLEService* service_ = nullptr;
    NimBLECharacteristic* chrCmd_ = nullptr;
    NimBLECharacteristic* chrStatus_ = nullptr;
    
    ServerCallbacks serverCallbacks_;
    CmdCallbacks cmdCallbacks_;
    bool hasClientAddress_ = false;
    NimBLEAddress serverAddress_;
    NimBLEAddress lastClientAddress_;
};
