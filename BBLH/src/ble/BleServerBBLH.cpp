#include "ble/BleServerBBLH.h"
#include "esp_log.h"

// TAGs (same spirit as BBLC)
static const char* TAG = "BBLH_BLE";
static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 3000;
static constexpr uint32_t WATCHDOG_TIMEOUT_MS = 8000;
static constexpr uint32_t WATCHDOG_WARN_MS = 2000;

namespace {
void watchdogLog(const char* msg) {
    ESP_LOGW(TAG, "%s", msg);
}
}

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
    : heartbeat_(BleHeartbeat::Role::SERVER, HEARTBEAT_INTERVAL_MS),
      watchdog_(WATCHDOG_TIMEOUT_MS, WATCHDOG_WARN_MS, watchdogLog),
      serverCallbacks_(*this),
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
    if (!isClientConnected()) {
        return;
    }

    heartbeat_.update();
    watchdog_.update();

    if (watchdog_.isExpired()) {
        handleWatchdogExpiry();
    }
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
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY
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
    clientConnected_ = false;
    hasConnHandle_ = false;

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
    parent_.hasConnHandle_ = true;
    parent_.lastConnHandle_ = connInfo.getConnHandle();
    parent_.clientConnected_ = true;

    parent_.heartbeat_.resetTimers();
    parent_.watchdog_.kick();

    ESP_LOGI(TAG, "Client connected from %s",
        parent_.lastClientAddress_.toString().c_str());

    parent_.setState(BleState::CLIENT_CONNECTED);
}

void BleServerBBLH::ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    parent_.lastClientAddress_ = connInfo.getAddress();
    parent_.hasClientAddress_ = true;
    parent_.clientConnected_ = false;
    parent_.hasConnHandle_ = false;

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

    const uint8_t* data = reinterpret_cast<const uint8_t*>(value.data());
    const size_t len = value.size();

    if (BleHeartbeat::isPingPayload(data, len)) {
        static const uint8_t PONG[] = {'P','O','N','G'};
        parent_.notifyCommandCharacteristic(PONG, sizeof(PONG));
        parent_.notifyStatus("PONG");
        parent_.watchdog_.kick();
        return;
    }

    if (parent_.cmdCb_) {
        parent_.cmdCb_(data, len);
    }

    parent_.notifyStatus("CMD_RX");
}

bool BleServerBBLH::isClientConnected() const {
    return clientConnected_ && server_ && server_->getConnectedCount() > 0;
}

void BleServerBBLH::handleWatchdogExpiry() {
    ESP_LOGE(TAG, "Watchdog expired, disconnecting client");

    if (server_ && hasConnHandle_) {
        server_->disconnect(lastConnHandle_);
    }

    clientConnected_ = false;
    hasConnHandle_ = false;

    setState(BleState::DISCONNECTED);
    startAdvertising();
}

bool BleServerBBLH::notifyCommandCharacteristic(const uint8_t* data, size_t len) {
    if (!data || len == 0) return false;
    if (!chrCmd_ || !server_ || !server_->getConnectedCount()) return false;

    chrCmd_->setValue(data, len);
    return chrCmd_->notify();
}
