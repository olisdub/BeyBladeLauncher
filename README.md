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

---

## BLE Robustness: Heartbeat & Watchdog

To improve the reliability of the BLE connection between **BBLC** (client) and **BBLH** (server),
a **heartbeat + watchdog mechanism** has been introduced.

This mechanism is designed to detect:
- Silent BLE link freezes
- Client or server firmware hangs
- Lost connections without proper disconnect events

The goal is to ensure **automatic recovery** without blocking the main loop or resetting the MCU.

---

### Heartbeat mechanism

A lightweight **PING / PONG** exchange is used:

- **BBLC (client)** periodically sends a `PING`
- **BBLH (server)** immediately replies with a `PONG`
- Each valid heartbeat message confirms that:
  - the BLE link is alive
  - both main loops are still running

Heartbeat messages are exchanged **only when the BLE state is CONNECTED**.

---

### Watchdog mechanism

Each side (BBLC and BBLH) embeds a **local BLE watchdog**:

- The watchdog is reset (`kick`) whenever a valid heartbeat or BLE message is received
- If no heartbeat is received within a defined timeout:
  - the connection is considered stalled
  - a controlled recovery is triggered

Recovery behavior:
- **BBLC**: disconnects and restarts scanning
- **BBLH**: forces client disconnect and restarts advertising

No blocking calls, delays, or MCU resets are used.

---

### Shared implementation (CommonUI)

The heartbeat and watchdog logic is implemented as **shared, BLE-agnostic components**:

- `BleHeartbeat`
  - Handles heartbeat timing and PING/PONG recognition
  - Works for both client and server roles
  - Does not depend on NimBLE or BLE transport

- `BleWatchdog`
  - Tracks connection liveness using timeouts
  - Contains no BLE or recovery logic
  - Used by BBLC and BBLH to trigger role-specific recovery

All BLE transport and recovery decisions remain inside
`BleClientBBLC` and `BleServerBBLH`.

---

### Design principles

- Non-blocking (`millis()` based)
- No logic inside BLE callbacks
- Clear separation of responsibilities
- Explicit BLE state handling
- Suitable for embedded / real-time systems

This design makes the BLE connection more robust while keeping
the codebase maintainable and extensible.

