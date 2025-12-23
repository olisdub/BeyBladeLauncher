#include <BleCentralLink.h>
#include <BleStatusLed.h>

// GPIO for on-board LED varies by board.
static constexpr uint8_t LED_PIN = 2;

// Match the peripheral by advertised name:
BleCentralLink ble("BLH_Header");
BleStatusLed led(LED_PIN);

void setup() {
  Serial.begin(115200);
  led.begin();

  ble.onStateChange([&](BleConnectionState st){
    led.setConnected(st == BleConnectionState::CONNECTED);
    Serial.print("Central state: ");
    Serial.println((int)st);
  });

  ble.begin("BLC_Controller");
}

void loop() {
  ble.update();
  led.update();
}
