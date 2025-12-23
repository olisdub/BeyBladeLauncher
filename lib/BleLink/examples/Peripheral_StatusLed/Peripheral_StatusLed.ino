#include <BlePeripheralLink.h>
#include <BleStatusLed.h>

// GPIO for on-board LED varies by board.
// For Seeed XIAO ESP32-C3, built-in LED is usually on GPIO 2 (check your board).
static constexpr uint8_t LED_PIN = 2;

BlePeripheralLink ble;
BleStatusLed led(LED_PIN);

void setup() {
  Serial.begin(115200);
  led.begin();

  ble.onStateChange([&](BleConnectionState st){
    led.setConnected(st == BleConnectionState::CONNECTED);
    Serial.print("Peripheral state: ");
    Serial.println((int)st);
  });

  ble.begin("BLH_Header");
}

void loop() {
  ble.update();
  led.update();
}
