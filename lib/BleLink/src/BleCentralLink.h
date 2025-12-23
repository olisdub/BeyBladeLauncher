#pragma once
#include "BleLink.h"
#include <NimBLEDevice.h>

// Simple Central (GATT Client) wrapper.
// Scans for a peripheral by advertised name OR by service UUID, then connects.
//
// Usage:
//   BleCentralLink ble("BLH_Header"); // target name
//   ble.begin("BLC_Controller");
//   ble.update(); // in loop
class BleCentralLink : public BleLink {
public:
    // Match by name (recommended for prototypes)
    explicit BleCentralLink(const char* targetDeviceName);

    // Match by service UUID (more robust when names can change)
    explicit BleCentralLink(const NimBLEUUID& targetServiceUuid);

    void begin(const char* deviceName) override;
    void update() override;

    // Optional: set scan interval / window (units: 0.625ms)
    void setScanParams(uint16_t interval = 45, uint16_t window = 15, bool activeScan = true);

    // Force disconnect / rescan
    void disconnect();

    NimBLEClient* client() const { return _client; }

private:
    void startScan();
    bool shouldConnectTo(const NimBLEAdvertisedDevice* dev) const;
    bool connectTo(const NimBLEAdvertisedDevice* dev);

    class ClientCallbacks : public NimBLEClientCallbacks {
    public:
        explicit ClientCallbacks(BleCentralLink* owner) : _owner(owner) {}
        void onConnect(NimBLEClient* pClient) override;
        void onDisconnect(NimBLEClient* pClient, int reason) override;
    private:
        BleCentralLink* _owner;
    };

    class AdvertisedCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    public:
        explicit AdvertisedCallbacks(BleCentralLink* owner) : _owner(owner) {}
        void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override;
    private:
        BleCentralLink* _owner;
    };

    NimBLEClient* _client = nullptr;
    NimBLEScan* _scan = nullptr;

    // matching mode
    String _targetName;
    NimBLEUUID _targetServiceUuid;
    bool _useServiceUuid = false;

    // scan settings
    uint16_t _scanInterval = 45;
    uint16_t _scanWindow = 15;
    bool _activeScan = true;

    // internal flags
    volatile bool _connectRequested = false;
    NimBLEAdvertisedDevice _pendingDevice; // copy of last matching device
};
