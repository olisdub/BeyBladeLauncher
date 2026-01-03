#pragma once

#include <Arduino.h>
#include <stdint.h>

/**
 * @brief Heartbeat helper for BLE client/server roles (ping/pong).
 *
 * - Non-blocking, uses millis()
 * - No BLE dependency (transport done via callback)
 * - Works for client (periodic PING) and server (PONG reply)
 * - Provides helpers to identify PING/PONG payloads
 */
class BleHeartbeat {
public:
    enum class Role { CLIENT, SERVER };

    // Transport callback: optional, used by server to answer PONG automatically.
    using TxFn = bool (*)(const uint8_t* data, size_t len);

    BleHeartbeat(Role role,
                 uint32_t pingIntervalMs,
                 TxFn txFn = nullptr);

    // Set/replace the transport callback.
    void setTx(TxFn txFn) { txFn_ = txFn; }

    // Advance state machine; call from loop().
    void update();

    // Client-side: true when it is time to send a PING (also arms awaitingPong).
    bool shouldSendPing();

    // Reset timing (call on successful (re)connection).
    void resetTimers();

    // Inject incoming payload (raw bytes); server replies to PING, client records PONG.
    void onPacket(const uint8_t* data, size_t len);

    // Helpers to recognize heartbeat payloads.
    static bool isPingPayload(const uint8_t* data, size_t len);
    static bool isPongPayload(const uint8_t* data, size_t len);

    // True if client is waiting for a pong after last ping.
    bool isAwaitingPong() const { return awaitingPong_; }

private:
    bool sendPing(); // only used if txFn_ is provided
    bool sendPong();

    Role role_;
    uint32_t pingIntervalMs_;
    TxFn txFn_;

    uint32_t lastPingMs_;
    uint32_t lastPongMs_;
    bool awaitingPong_ = false;
    bool duePing_ = false;
};
