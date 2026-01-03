#include <Arduino.h>
#include "esp_log.h"
#include "ble/BleClientBBLC.h"
#include "ble/BleStatus.h"
#include "led/StatusLed.h"

static const char* TAG = "MAIN";
// =========================
// Hardware
// =========================
static constexpr uint8_t STATUS_LED_PIN = 2;

// =========================
// Objects
// =========================
StatusLed<STATUS_LED_PIN> statusLed;
BleStatus<StatusLed<STATUS_LED_PIN>> bleStatus(statusLed);
BleClientBBLC bleClient;

// =========================
// Setup
// =========================
void setup() {
    Serial.begin(115200);
    delay(1000);

    esp_log_level_set("NimBLE", ESP_LOG_DEBUG);
    esp_log_level_set("ble_gap", ESP_LOG_DEBUG);
    esp_log_level_set("ble_hs", ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "BBLC BLE Client started");

    // Init LED
    statusLed.begin();

    // Init BLE client
    bleClient.begin();

    // Bind BLE state → LED
    bleClient.onStateChange([](BleState state) {
        ESP_LOGI(TAG, "BLE state -> %s (%d)", bleStateToString(state), state);
        bleStatus.update(state);
    });

    // Start scanning explicitly
    bleClient.startScan();
}

// =========================
// Loop
// =========================
void loop() {
    bleClient.loop();
    statusLed.update();   // ✅ indispensable pour les animations (SCANNING, CONNECTING…)

    // Optionnel : heartbeat pour vérifier que le loop tourne
    static uint32_t lastBeat = 0;
    if (millis() - lastBeat > 2000) {
        lastBeat = millis();
        BleState state = bleClient.getState();
        ESP_LOGD(TAG, "BLE current state = %s", bleStateToString(state));
    }


    delay(10);
}
