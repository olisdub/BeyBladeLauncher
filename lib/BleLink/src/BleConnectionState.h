#pragma once

// Shared connection state used by both Central and Peripheral implementations.
enum class BleConnectionState {
    IDLE,
    ADVERTISING,
    SCANNING,
    CONNECTING,
    CONNECTED
};
