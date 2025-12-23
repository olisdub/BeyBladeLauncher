# BleLink (ESP32 / NimBLE-Arduino)

Reusable BLE link layer for ESP32 projects (ESP32-C3 friendly) with:
- **Peripheral** wrapper: `BlePeripheralLink`
- **Central** wrapper: `BleCentralLink`
- Shared connection state enum: `BleConnectionState`
- Single-pin status LED helper: `BleStatusLed`

## Requirements
- Arduino-ESP32 core
- NimBLE-Arduino (included in Arduino-ESP32 as `NimBLEDevice.h`)

## Install (Arduino IDE)
Copy the `BleLink` folder to:
`<Documents>/Arduino/libraries/BleLink`

Restart Arduino IDE.

## Examples
- `examples/Peripheral_StatusLed`
- `examples/Central_StatusLed`

## Notes
- Central reconnects automatically by restarting scan on disconnect.
- Peripheral restarts advertising automatically on disconnect.
