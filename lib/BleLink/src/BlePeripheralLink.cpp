#include "BlePeripheralLink.h"

BlePeripheralLink::BlePeripheralLink() {}

void BlePeripheralLink::begin(const char* deviceName) {
    NimBLEDevice::init(deviceName);

    _server = NimBLEDevice::createServer();
    _server->setCallbacks(new ServerCallbacks(this));

    _adv = NimBLEDevice::getAdvertising();
    // Reasonable defaults
    _adv->setScanResponse(true);
    _adv->setMinPreferred(0x06);
    _adv->setMaxPreferred(0x12);

    setState(BleConnectionState::ADVERTISING);
    startAdvertising();
}

void BlePeripheralLink::update() {
    // Nothing required.
}

NimBLEService* BlePeripheralLink::createService(const NimBLEUUID& uuid) {
    if (!_server) return nullptr;
    return _server->createService(uuid);
}

void BlePeripheralLink::startAdvertising() {
    if (_adv) {
        _adv->start();
        setState(isConnected() ? BleConnectionState::CONNECTED : BleConnectionState::ADVERTISING);
    }
}

void BlePeripheralLink::stopAdvertising() {
    if (_adv) _adv->stop();
}

void BlePeripheralLink::ServerCallbacks::onConnect(NimBLEServer*, NimBLEConnInfo&) {
    _owner->setConnected(true);
}

void BlePeripheralLink::ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo&, int) {
    _owner->setConnected(false);
    // Resume advertising automatically (same pattern as your Nerf project)
    if (pServer) {
        NimBLEDevice::startAdvertising();
        _owner->setState(BleConnectionState::ADVERTISING);
    }
}
