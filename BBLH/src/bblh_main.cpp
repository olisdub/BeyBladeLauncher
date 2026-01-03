#include <Arduino.h>
#include "esp_log.h"

#include "ble/BleServerBBLH.h"
#include "ble/BleStatus.h"
#include "led/StatusLed.h"

// GPIO réel
static constexpr uint8_t STATUS_LED_PIN = 2;

static const char* TAG = "MAIN_BBLH";

StatusLed<STATUS_LED_PIN> statusLed;
BleStatus<StatusLed<STATUS_LED_PIN>> bleStatus(statusLed);
BleServerBBLH bleServer;

void setup() {
    Serial.begin(115200);
    delay(200);

    esp_log_level_set("NimBLE", ESP_LOG_DEBUG);
    esp_log_level_set("ble_gap", ESP_LOG_DEBUG);
    esp_log_level_set("ble_hs", ESP_LOG_DEBUG);

    ESP_LOGI(TAG, "BBLH server starting");

    statusLed.begin();

    bleServer.onStateChange([](BleState s) {
        ESP_LOGI(TAG, "BLE state -> %s (%d)", bleStateToString(s), (int)s);
        bleStatus.update(s);
    });

    bleServer.onCommand([](const uint8_t* data, size_t len) {
        // Exemple simple : dump ASCII si printable
        ESP_LOGI(TAG, "APP command received (%u bytes)", (unsigned)len);

        // Ici tu branches ta logique de launcher.
        // Exemple: si data[0] == 0x01 => fire
    });

    bleServer.begin();
    bleStatus.update(bleServer.getState());
}

void loop() {
    bleServer.loop();
    statusLed.update();   // moteur LED (comme ton test_led_RGB.cpp)

    // Debug périodique
    static uint32_t last = 0;
    if (millis() - last > 3000) {
        last = millis();
        ESP_LOGD(TAG,
         "BLE current state = %s (server=%s)",
         bleStateToString(bleServer.getState()),
         bleServer.getServerAddress().toString().c_str());
    }
}
