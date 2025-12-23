#include "BleCentralLink.h"

BleCentralLink::BleCentralLink(const char* targetDeviceName)
: _targetName(targetDeviceName ? targetDeviceName : "") {}

BleCentralLink::BleCentralLink(const NimBLEUUID& targetServiceUuid)
: _targetServiceUuid(targetServiceUuid), _useServiceUuid(true) {}

void BleCentralLink::setScanParams(uint16_t interval, uint16_t window, bool activeScan) {
    _scanInterval = interval;
    _scanWindow = window;
    _activeScan = activeScan;
    if (_scan) {
        _scan->setInterval(_scanInterval);
        _scan->setWindow(_scanWindow);
        _scan->setActiveScan(_activeScan);
    }
}

void BleCentralLink::begin(const char* deviceName) {
    NimBLEDevice::init(deviceName);

    _client = NimBLEDevice::createClient();
    _client->setClientCallbacks(new ClientCallbacks(this), false);

    _scan = NimBLEDevice::getScan();
    _scan->setAdvertisedDeviceCallbacks(new AdvertisedCallbacks(this), false);
    _scan->setInterval(_scanInterval);
    _scan->setWindow(_scanWindow);
    _scan->setActiveScan(_activeScan);

    startScan();
}

void BleCentralLink::startScan() {
    if (!_scan) return;
    setState(BleConnectionState::SCANNING);
    // scan indefinitely in background (0 = continuous in NimBLE-Arduino)
    _scan->start(0, nullptr, false);
}

bool BleCentralLink::shouldConnectTo(const NimBLEAdvertisedDevice* dev) const {
    if (!dev) return false;

    if (_useServiceUuid) {
        return dev->isAdvertisingService(_targetServiceUuid);
    }

    if (_targetName.length() > 0 && dev->haveName()) {
        return dev->getName() == _targetName.c_str();
    }

    return false;
}

bool BleCentralLink::connectTo(const NimBLEAdvertisedDevice* dev) {
    if (!_client || !dev) return false;

    setState(BleConnectionState::CONNECTING);

    // Stop scanning to improve connection reliability
    if (_scan) _scan->stop();

    bool ok = _client->connect(dev, false);
    if (!ok) {
        // failed: resume scan
        setConnected(false);
        startScan();
        return false;
    }

    // connected callback will set state
    return true;
}

void BleCentralLink::update() {
    // If already connected, nothing to do
    if (_client && _client->isConnected()) {
        // Ensure state is correct
        setState(BleConnectionState::CONNECTED);
        return;
    }

    // If a matching device was found by callback, try connect
    if (_connectRequested) {
        _connectRequested = false;
        connectTo(&_pendingDevice);
        return;
    }

    // Not connected and not scanning? restart scan
    if (_scan && !_scan->isScanning()) {
        startScan();
    }
}

void BleCentralLink::disconnect() {
    if (_client && _client->isConnected()) {
        _client->disconnect();
    } else {
        startScan();
    }
}

void BleCentralLink::ClientCallbacks::onConnect(NimBLEClient*) {
    _owner->setConnected(true);
    _owner->setState(BleConnectionState::CONNECTED);
}

void BleCentralLink::ClientCallbacks::onDisconnect(NimBLEClient*, int) {
    _owner->setConnected(false);
    // Resume scanning for reconnection
    _owner->startScan();
}

void BleCentralLink::AdvertisedCallbacks::onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
    if (_owner->isConnected()) return;
    if (!_owner->shouldConnectTo(advertisedDevice)) return;

    // Copy device info and request connection in update() context
    _owner->_pendingDevice = *advertisedDevice;
    _owner->_connectRequested = true;
}
