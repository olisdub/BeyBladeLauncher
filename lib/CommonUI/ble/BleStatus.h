#pragma once

#include "../led/StatusLed.h"
#include <FastLED.h>

// =======================================================
// Shared BLE state for Client (BBLC) and Server (BBLH)
// =======================================================
enum class BleState {
    // Common
    BOOT,

    // -------- Client BLE (BBLC) --------
    SCANNING,        // Client scanning for server
    CONNECTING,
    CONNECTED,

    // -------- Server BLE (BBLH) --------
    ADVERTISING,     // Server advertising
    CLIENT_CONNECTED,

    // -------- Common --------
    DISCONNECTED,
    ERROR
};

/**
 * @brief Convert a BleState enum to a human-readable string.
 *
 * This function is declared `inline` because it is defined in a header file
 * that is included by multiple translation units (e.g. main.cpp,
 * BleClientBBLC.cpp).
 *
 * Without `inline`, each .cpp file including this header would generate its
 * own definition of the function, leading to a "multiple definition" linker
 * error.
 *
 * Declaring the function `inline` allows the linker to accept multiple
 * identical definitions, as required by the C++ One Definition Rule (ODR),
 * while still keeping the implementation in the header for convenience.
 *
 * @param state Current BLE client state.
 * @return A constant string representing the state.
 */

inline const char* bleStateToString(BleState state) {
    switch (state) {
        case BleState::BOOT:         return "BOOT";
        case BleState::SCANNING:     return "SCANNING";
        case BleState::CONNECTING:   return "CONNECTING";
        case BleState::CONNECTED:    return "CONNECTED";
        case BleState::ADVERTISING:    return "ADVERTISING";
        case BleState::CLIENT_CONNECTED:    return "CLIENT_CONNECTED";
        case BleState::DISCONNECTED: return "DISCONNECTED";
        case BleState::ERROR:        return "ERROR";
        default:                           return "UNKNOWN";
    }
}

// =========================
// BLE semantic layer
// =========================
template<typename StatusLedT>
class BleStatus {
public:
    explicit BleStatus(StatusLedT& led)
        : led(led) {}

    void update(BleState state) {
        switch (state) {

            case BleState::BOOT:
                led.setStyle({LedPattern::PULSE, CRGB::Blue, CRGB::Black, 0, 0});
                break;

            case BleState::SCANNING:
                led.setStyle({LedPattern::BLINK, CRGB(128,0,128), CRGB::Black, 300, 300});
                break;

            case BleState::CONNECTING:
                led.setStyle({LedPattern::PULSE, CRGB::Yellow, CRGB::Black, 0, 0});
                break;

            case BleState::CONNECTED:
                led.setStyle({LedPattern::SOLID, CRGB::Green, CRGB::Black, 0, 0});
                break;
            
            case BleState::ADVERTISING:
                led.setStyle({LedPattern::BLINK, CRGB(128,0,128), CRGB::Black, 500, 500});
                break;

            case BleState::CLIENT_CONNECTED:
                led.setStyle({LedPattern::SOLID, CRGB::Green, CRGB::Black, 0, 0});
                break;

            case BleState::DISCONNECTED:
                led.setStyle({LedPattern::BLINK, CRGB::Red, CRGB::Black, 150, 150});
                break;

            case BleState::ERROR:
                led.setStyle({LedPattern::ALTERNATE, CRGB::Red, CRGB::Blue, 250, 250});
                break;
        }
    }

private:
    StatusLedT& led;
};


