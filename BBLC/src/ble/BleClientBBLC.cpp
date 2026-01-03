#include "BleClientBBLC.h"
#include "esp_log.h"

static const char* TAG = "BLE";

static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 3000;
static constexpr uint32_t WATCHDOG_TIMEOUT_MS = 8000;
static constexpr uint32_t WATCHDOG_WARN_MS = 2000;

namespace {
void watchdogLog(const char* msg) {
    ESP_LOGW(TAG, "%s", msg);
}
}

// UUIDs attendus cote BBLH
static const NimBLEUUID BBLH_SERVICE_UUID("a1b2c3d4-0001-4000-8000-000000000001");
static const NimBLEUUID BBLH_CMD_UUID("a1b2c3d4-0002-4000-8000-000000000001");
static const NimBLEUUID BBLH_STATUS_UUID("a1b2c3d4-0003-4000-8000-000000000001");

// ==========================
// Constructor
// ==========================
BleClientBBLC::BleClientBBLC()
    : heartbeat_(BleHeartbeat::Role::CLIENT, HEARTBEAT_INTERVAL_MS),
      watchdog_(WATCHDOG_TIMEOUT_MS, WATCHDOG_WARN_MS, watchdogLog),
      scanCallbacks_(*this),
      clientCallbacks_(*this) {}

// ==========================
// Public API
// ==========================
void BleClientBBLC::begin() {
    NimBLEDevice::init("BBLC");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);

    scan_ = NimBLEDevice::getScan();
    scan_->setScanCallbacks(&scanCallbacks_, false);
    scan_->setInterval(45);
    scan_->setWindow(15);
    scan_->setActiveScan(true);

    setState(BleState::BOOT);
}

void BleClientBBLC::loop() {
    connectIfPending();

    if (state_ == BleState::CONNECTED) {
        heartbeat_.update();

        if (heartbeat_.shouldSendPing()) {
            static const uint8_t kPing[] = {'P','I','N','G'};
            sendCommand(kPing, sizeof(kPing), false);
        }

        watchdog_.update();
        if (watchdog_.isExpired()) {
            ESP_LOGE(TAG, "Watchdog expired, disconnecting");
            disconnect();
            setState(BleState::DISCONNECTED);
            startScan();
        }
    }
}

void BleClientBBLC::startScan() {
    ESP_LOGI(TAG, "Start scanning");

    pendingConnect_ = false;
    scan_->stop();               // eviter les overlaps de scan
    scan_->clearResults();       // optionnel mais sain
    seenAdvertisers_.clear();

    scan_->start(0, false);

    setState(BleState::SCANNING);
}

void BleClientBBLC::disconnect() {
    if (client_ && client_->isConnected()) {
        client_->disconnect();
    }
}

BleState BleClientBBLC::getState() const {
    return state_;
}

void BleClientBBLC::onStateChange(StateCallback cb) {
    stateCallback_ = cb;
}

bool BleClientBBLC::sendCommand(const uint8_t* data, size_t len, bool response) {
    if (!chrCmd_ || !client_ || !client_->isConnected()) {
        ESP_LOGW(TAG, "sendCommand: client not ready");
        return false;
    }

    bool ok = chrCmd_->writeValue(data, len, response);
    ESP_LOGI(TAG, "Send CMD (%u bytes) -> %s", static_cast<unsigned>(len), ok ? "ok" : "fail");
    return ok;
}

// ==========================
// Internal logic
// ==========================
void BleClientBBLC::setState(BleState newState) {
    if (state_ == newState) {
        return;
    }

    state_ = newState;

    ESP_LOGI(TAG, "State -> %d", static_cast<int>(state_));

    if (stateCallback_) {
        stateCallback_(state_);
    }
}

void BleClientBBLC::requestConnect(const NimBLEAddress& address) {
    targetAddress_ = address;
    pendingConnect_ = true;
}

void BleClientBBLC::connectIfPending() {
    if (!pendingConnect_) {
        return;
    }

    pendingConnect_ = false;
    setState(BleState::CONNECTING);

    if (client_) {
        NimBLEDevice::deleteClient(client_);
        client_ = nullptr;
    }

    client_ = NimBLEDevice::createClient();
    client_->setClientCallbacks(&clientCallbacks_);

    ESP_LOGI(TAG, "Connecting to %s", targetAddress_.toString().c_str());

    if (!client_->connect(targetAddress_)) {
        ESP_LOGE(TAG, "Connection failed");
        NimBLEDevice::deleteClient(client_);
        client_ = nullptr;
        setState(BleState::DISCONNECTED);
        startScan();
        return;
    }

    if (!setupRemoteCharacteristics()) {
        ESP_LOGE(TAG, "Remote setup failed, restart scan");
        client_->disconnect();
        setState(BleState::ERROR);
        startScan();
        return;
    }

    heartbeat_.resetTimers();
    watchdog_.kick();
    setState(BleState::CONNECTED);
}

// ==========================
// ScanCallbacks
// ==========================
BleClientBBLC::ScanCallbacks::ScanCallbacks(BleClientBBLC& parent)
    : parent_(parent) {}

void BleClientBBLC::ScanCallbacks::onResult(
    const NimBLEAdvertisedDevice* device
) {
    if (parent_.state_ != BleState::SCANNING) {
        return;
    }

    bool isNew = parent_.updateAdvertiser(device);

    if (!isNew) {
        return;
    }

    ESP_LOGI(TAG, "[BLE] New advertiser discovered:");
    ESP_LOGI(TAG, "  Addr: %s", device->getAddress().toString().c_str());
    ESP_LOGI(TAG, "  Name: %s", device->haveName() ? device->getName().c_str() : "(none)");
    ESP_LOGI(TAG, "  RSSI: %d", device->getRSSI());
    ESP_LOGI(TAG, "  ServiceUUID: %d", device->haveServiceUUID());
    ESP_LOGI(TAG, "  ServiceData: %d", device->haveServiceData());
    ESP_LOGI(TAG, "  ManufacturerData: %d", device->haveManufacturerData());

    ESP_LOGI(TAG, "[BLE] Total advertisers: %d",
             parent_.seenAdvertisers_.size());

    // Auto-connect si le service BBLH est annonce ou si le nom correspond
    const bool matchService = device->isAdvertisingService(BBLH_SERVICE_UUID);
    const bool matchName = device->haveName() && device->getName() == "BBLH";

    if (matchService || matchName) {
        ESP_LOGI(TAG, "BBLH service detected, preparing connection");
        parent_.scan_->stop();
        parent_.requestConnect(device->getAddress());
    }
}

// ==========================
// ClientCallbacks
// ==========================
BleClientBBLC::ClientCallbacks::ClientCallbacks(BleClientBBLC& parent)
    : parent_(parent) {}

void BleClientBBLC::ClientCallbacks::onConnect(NimBLEClient*) {
    ESP_LOGI(TAG, "Connected (link up)");
}

void BleClientBBLC::ClientCallbacks::onDisconnect(NimBLEClient*) {
    ESP_LOGI(TAG, "Disconnected");
    parent_.setState(BleState::DISCONNECTED);
    parent_.startScan();
}

// ==========================
// Advertiser cache
// ==========================
bool BleClientBBLC::updateAdvertiser(
    const NimBLEAdvertisedDevice* device
) {
    const NimBLEAddress& addr = device->getAddress();

    for (auto& adv : seenAdvertisers_) {
        if (adv.address == addr) {
            adv.rssi = device->getRSSI();
            adv.lastSeenMs = millis();

            if (device->haveName()) {
                adv.name = device->getName();
            }

            adv.hasServiceUUID      = device->haveServiceUUID();
            adv.hasServiceData      = device->haveServiceData();
            adv.hasManufacturerData = device->haveManufacturerData();

            return false; // deja connu
        }
    }

    BleAdvertiserInfo info;
    info.address = addr;
    info.name = device->haveName() ? device->getName() : "";
    info.rssi = device->getRSSI();

    info.hasServiceUUID      = device->haveServiceUUID();
    info.hasServiceData      = device->haveServiceData();
    info.hasManufacturerData = device->haveManufacturerData();

    info.lastSeenMs = millis();

    seenAdvertisers_.push_back(info);
    return true; // nouveau
}

// ==========================
// Remote setup / notifications
// ==========================
bool BleClientBBLC::setupRemoteCharacteristics() {
    bblhService_ = client_ ? client_->getService(BBLH_SERVICE_UUID) : nullptr;
    if (!bblhService_) {
        ESP_LOGE(TAG, "BBLH service not found on peripheral");
        return false;
    }

    chrCmd_ = bblhService_->getCharacteristic(BBLH_CMD_UUID);
    chrStatus_ = bblhService_->getCharacteristic(BBLH_STATUS_UUID);

    if (!chrCmd_ || !chrStatus_) {
        ESP_LOGE(TAG, "Missing CMD or STATUS characteristic");
        return false;
    }

    if (chrStatus_->canNotify() || chrStatus_->canIndicate()) {
        auto cb = std::bind(&BleClientBBLC::onStatusNotify, this,
                            std::placeholders::_1,
                            std::placeholders::_2,
                            std::placeholders::_3,
                            std::placeholders::_4);
        if (!chrStatus_->subscribe(true, cb)) {
            ESP_LOGW(TAG, "Failed to subscribe to STATUS notifications");
        }
    } else {
        ESP_LOGW(TAG, "STATUS characteristic has no notify/indicate");
    }

    ESP_LOGI(TAG, "Remote characteristics ready");
    return true;
}

void BleClientBBLC::onStatusNotify(NimBLERemoteCharacteristic* chr, uint8_t* data, size_t len, bool isNotify) {
    (void)chr;

    if (BleHeartbeat::isPongPayload(data, len)) {
        watchdog_.kick();
        ESP_LOGI(TAG, "PONG received");
        return;
    }

    ESP_LOGI(TAG, "STATUS %s (%u bytes): %.*s",
        isNotify ? "notify" : "indicate",
        static_cast<unsigned>(len),
        static_cast<int>(len),
        reinterpret_cast<const char*>(data));
}
