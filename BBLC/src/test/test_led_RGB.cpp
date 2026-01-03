/* #include <Arduino.h>
#include "lib/StatusLed.h"
#include "lib/BleStatus.h"

// =========================
// Hardware (GPIO réel !)
// =========================
static constexpr uint8_t STATUS_LED_PIN = 2; // PAS D0

// =========================
// Objects
// =========================
StatusLed<STATUS_LED_PIN> statusLed;
BleStatus<StatusLed<STATUS_LED_PIN>> bleStatus(statusLed);

// =========================
// Test states
// =========================
static const BleState TEST_STATES[] = {
    BleState::BOOT,
    BleState::SCANNING,
    BleState::CONNECTING,
    BleState::CONNECTED,
    BleState::DISCONNECTED,
    BleState::ERROR
};

uint8_t stateIndex = 0;
uint32_t lastChange = 0;
static constexpr uint32_t STATE_DURATION_MS = 4000;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("=== StatusLed + BleStatus test ===");

    statusLed.begin();
    bleStatus.update(TEST_STATES[stateIndex]);
}

void loop() {
    statusLed.update();

    uint32_t now = millis();
    if (now - lastChange >= STATE_DURATION_MS) {
        lastChange = now;
        stateIndex = (stateIndex + 1) %
            (sizeof(TEST_STATES) / sizeof(TEST_STATES[0]));

        bleStatus.update(TEST_STATES[stateIndex]);
    }
}
 */