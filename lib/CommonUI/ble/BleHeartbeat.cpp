#include "BleHeartbeat.h"

namespace {
constexpr uint8_t PING_BYTES[] = {'P', 'I', 'N', 'G'};
constexpr uint8_t PONG_BYTES[] = {'P', 'O', 'N', 'G'};
constexpr size_t PING_LEN = sizeof(PING_BYTES);
constexpr size_t PONG_LEN = sizeof(PONG_BYTES);
} // namespace

BleHeartbeat::BleHeartbeat(Role role,
                           uint32_t pingIntervalMs,
                           TxFn txFn)
    : role_(role),
      pingIntervalMs_(pingIntervalMs),
      txFn_(txFn),
      lastPingMs_(millis()),
      lastPongMs_(millis()) {}

void BleHeartbeat::update() {
    const uint32_t now = millis();

    if (role_ == Role::CLIENT) {
        const uint32_t elapsed = now - lastPingMs_;
        if (elapsed >= pingIntervalMs_) {
            duePing_ = true;
        }
    }
}

void BleHeartbeat::onPacket(const uint8_t* data, size_t len) {
    if (!data || len == 0) return;

    if (isPingPayload(data, len)) {
        if (role_ == Role::SERVER) {
            sendPong();
        }
        return;
    }

    if (role_ == Role::CLIENT && isPongPayload(data, len)) {
        lastPongMs_ = millis();
        awaitingPong_ = false;
    }
}

bool BleHeartbeat::isPingPayload(const uint8_t* data, size_t len) {
    if (len != PING_LEN) return false;
    for (size_t i = 0; i < PING_LEN; ++i) {
        if (data[i] != PING_BYTES[i]) return false;
    }
    return true;
}

bool BleHeartbeat::isPongPayload(const uint8_t* data, size_t len) {
    if (len != PONG_LEN) return false;
    for (size_t i = 0; i < PONG_LEN; ++i) {
        if (data[i] != PONG_BYTES[i]) return false;
    }
    return true;
}

bool BleHeartbeat::sendPing() {
    if (!txFn_) return false;
    if (!shouldSendPing()) return false;
    return txFn_(PING_BYTES, PING_LEN);
}

bool BleHeartbeat::sendPong() {
    if (!txFn_) return false;
    return txFn_(PONG_BYTES, PONG_LEN);
}

bool BleHeartbeat::shouldSendPing() {
    if (role_ != Role::CLIENT) return false;

    const uint32_t now = millis();
    if (!duePing_ && (now - lastPingMs_) >= pingIntervalMs_) {
        duePing_ = true;
    }

    if (!duePing_) return false;

    // Arm the wait-for-PONG window
    lastPingMs_ = now;
    duePing_ = false;
    awaitingPong_ = true;
    return true;
}

void BleHeartbeat::resetTimers() {
    const uint32_t now = millis();
    lastPingMs_ = now;
    lastPongMs_ = now;
    awaitingPong_ = false;
    duePing_ = false;
}
