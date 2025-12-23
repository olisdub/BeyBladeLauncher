#pragma once
#include "BleLink.h"

// NimBLE-Arduino (ESP32) include
#include <NimBLEDevice.h>

// Simple Peripheral (GATT Server) wrapper.
// Exposes one service (optional) and provides connection state callbacks.
//
// Typical use:
//   BlePeripheralLink ble;
//   ble.begin("BLH_Header");
//   ble.startAdvertising();
class BlePeripheralLink : public BleLink {
public:
    BlePeripheralLink();

    // Create server and start advertising.
    void begin(const char* deviceName) override;

    // Peripheral does not require periodic work, but keep for symmetry.
    void update() override;

    // Optional: create a service (for later command channel)
    NimBLEService* createService(const NimBLEUUID& uuid);

    // Start / stop advertising manually if you want
    void startAdvertising();
    void stopAdvertising();

    NimBLEServer* server() const { return _server; }

private:
    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        explicit ServerCallbacks(BlePeripheralLink* owner) : _owner(owner) {}
        void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
        void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
    private:
        BlePeripheralLink* _owner;
    };

    NimBLEServer* _server = nullptr;
    NimBLEAdvertising* _adv = nullptr;
};
