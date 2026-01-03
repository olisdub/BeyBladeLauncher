#include "ble/BleServerBBLH.h"
#include "esp_log.h"

// TAGs (same spirit as BBLC)
static const char* TAG = "BBLH_BLE";

// UUIDs (keep identical to BBLC)
static const NimBLEUUID UUID_BBLH_SERVICE(
    "a1b2c3d4-0001-4000-8000-000000000001"
);

static const NimBLEUUID UUID_BBLH_CMD(
    "a1b2c3d4-0002-4000-8000-000000000001"
);

static const NimBLEUUID UUID_BBLH_STATUS(
    "a1b2c3d4-0003-4000-8000-000000000001"
);

BleServerBBLH::BleServerBBLH()
    : serverCallbacks_(*this),
      cmdCallbacks_(*this) {}

void BleServerBBLH::begin() {
    NimBLEDevice::init("BBLH");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    server_ = NimBLEDevice::createServer();
    server_->setCallbacks(&serverCallbacks_);

    serverAddress_ = NimBLEDevice::getAddress();

    setupGatt();
    startAdvertising();
}

void BleServerBBLH::loop() {
    // Symmetry with BBLC. Nothing to run here for NimBLE server for now.
}

void BleServerBBLH::onStateChange(StateCallback cb) {
    stateCb_ = cb;
}

void BleServerBBLH::onCommand(CommandCallback cb) {
    cmdCb_ = cb;
}

void BleServerBBLH::notifyStatus(const char* text) {
    if (!chrStatus_) return;

    // Notify only if a client is connected
    if (server_ && server_->getConnectedCount() > 0) {
        chrStatus_->setValue(text);
        chrStatus_->notify();
        ESP_LOGD(TAG, "Notify STATUS: %s", text);
    }
}

void BleServerBBLH::setState(BleState s) {
    if (state_ == s) return;
    state_ = s;

    ESP_LOGI(TAG, "State -> %d", static_cast<int>(state_));

    if (stateCb_) stateCb_(state_);
}

void BleServerBBLH::setupGatt() {
    service_ = server_->createService(UUID_BBLH_SERVICE);

    // CMD: Write (client -> server)
    chrCmd_ = service_->createCharacteristic(
        UUID_BBLH_CMD,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    chrCmd_->setCallbacks(&cmdCallbacks_);

    // STATUS: Notify (server -> client)
    chrStatus_ = service_->createCharacteristic(
        UUID_BBLH_STATUS,
        NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
    );
    chrStatus_->setValue("READY");

    service_->start();

    ESP_LOGI(TAG, "GATT ready (service + characteristics)");
}

void BleServerBBLH::startAdvertising() {
    NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();

    adv->reset();
    adv->setAppearance(0x0000);
    adv->addServiceUUID(UUID_BBLH_SERVICE);

    // Scan response packet
    NimBLEAdvertisementData scanResponse;
    scanResponse.setName("BBLH");
    adv->setScanResponseData(scanResponse);

    adv->start();

    ESP_LOGI(TAG, "Advertising started (%s)",
        serverAddress_.toString().c_str());
    setState(BleState::ADVERTISING);
}

// ===== Server callbacks =====
void BleServerBBLH::ServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    parent_.lastClientAddress_ = connInfo.getAddress();
    parent_.hasClientAddress_ = true;

    ESP_LOGI(TAG, "Client connected from %s",
        parent_.lastClientAddress_.toString().c_str());

    parent_.setState(BleState::CONNECTED);
}

void BleServerBBLH::ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    parent_.lastClientAddress_ = connInfo.getAddress();
    parent_.hasClientAddress_ = true;

    ESP_LOGW(TAG,
            "Client disconnected from %s (reason=%d)",
            parent_.lastClientAddress_.toString().c_str(),
            reason);

    parent_.setState(BleState::DISCONNECTED);
    parent_.startAdvertising();
}

// ===== CMD write callback =====
void BleServerBBLH::CmdCallbacks::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
    std::string value = pCharacteristic->getValue();

    ESP_LOGI(TAG, "CMD write (%d bytes)", static_cast<int>(value.size()));

    if (value.size() == 0) return;

    if (parent_.cmdCb_) {
        parent_.cmdCb_(
            reinterpret_cast<const uint8_t*>(value.data()),
            value.size()
        );
    }

    parent_.notifyStatus("CMD_RX");
}
