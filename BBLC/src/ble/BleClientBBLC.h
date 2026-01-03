#pragma once

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <functional>
#include <string>
#include <vector>

#include "ble/BleStatus.h"   // pour BleState

struct BleAdvertiserInfo {
    NimBLEAddress address;
    std::string name;
    int rssi;

    bool hasServiceUUID;
    bool hasServiceData;
    bool hasManufacturerData;

    uint32_t lastSeenMs;
};

class BleClientBBLC {
public:
    using StateCallback = std::function<void(BleState)>;

    BleClientBBLC();

    void begin();
    void loop();

    void startScan();
    void disconnect();
    bool sendCommand(const uint8_t* data, size_t len, bool response = false);
    static void onStatusNotify(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify);

    BleState getState() const;

    void onStateChange(StateCallback cb);

private:
    // ===== Internal helpers =====
    void setState(BleState newState);
    void requestConnect(const NimBLEAddress& address);
    void connectIfPending();

    // ===== BLE callbacks =====
    class ScanCallbacks : public NimBLEScanCallbacks {
    public:
        explicit ScanCallbacks(BleClientBBLC& parent);
        void onResult(const NimBLEAdvertisedDevice* device) override;
    private:
        BleClientBBLC& parent_;
    };

    class ClientCallbacks : public NimBLEClientCallbacks {
    public:
        explicit ClientCallbacks(BleClientBBLC& parent);
        void onConnect(NimBLEClient* client);
        void onDisconnect(NimBLEClient* client);
    private:
        BleClientBBLC& parent_;
    };

    // Liste des adresses vues pendant le scan
    std::vector<BleAdvertiserInfo> seenAdvertisers_;

    bool updateAdvertiser(const NimBLEAdvertisedDevice* device);
    bool setupRemoteCharacteristics();

private:
    // ===== State =====
    BleState state_ = BleState::BOOT;
    StateCallback stateCallback_;

    // ===== BLE objects =====
    NimBLEClient* client_ = nullptr;
    NimBLEScan* scan_ = nullptr;
    NimBLERemoteService* bblhService_ = nullptr;
    NimBLERemoteCharacteristic* chrCmd_ = nullptr;
    NimBLERemoteCharacteristic* chrStatus_ = nullptr;

    ScanCallbacks scanCallbacks_;
    ClientCallbacks clientCallbacks_;

    // ===== Connection workflow =====
    bool pendingConnect_ = false;
    NimBLEAddress targetAddress_;
};
