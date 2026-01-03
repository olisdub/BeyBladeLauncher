# BeyBladeLauncher

Wireless Beyblade Launcher based on two ESP32-C3 boards communicating over **Bluetooth Low Energy (BLE)**.

The project is split into a **controller (client)** and a **launcher head (server)**, with a clean and explicit BLE architecture.

---

## Projects

- **BBLC** — *BeyBlade Launcher Controller*  
  BLE **Central / Client**  
  Handles scanning, connection, commands, and user interface (LEDs).

- **BBLH** — *BeyBlade Launcher Header*  
  BLE **Peripheral / Server**  
  Exposes BLE services and characteristics to control the launcher hardware.

---

## Shared components

- **CommonUI**  
  Shared logic for:
  - BLE connection state abstraction
  - Status LED handling (RGB / single LED)
  - UI-independent BLE state representation

This module is used by both BBLC and BBLH to ensure consistent behavior.

---

## Tooling & Hardware

- **PlatformIO**
- **ESP32-C3**
- **NimBLE (ESP32 BLE stack)**

---

## BLE State Diagram

### BBLC — BLE Client (Central)

```text
IDLE -> SCANNING -> CONNECTING -> CONNECTED
          ^              |
          |              v
        DISCONNECTED <----
