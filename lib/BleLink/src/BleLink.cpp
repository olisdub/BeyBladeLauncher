#include "BleLink.h"

void BleLink::setState(BleConnectionState st) {
    if (_state != st) {
        _state = st;
        if (_callback) _callback(_state);
    }
}

void BleLink::setConnected(bool connected) {
    if (_connected != connected) {
        _connected = connected;
        setState(_connected ? BleConnectionState::CONNECTED : BleConnectionState::IDLE);
    }
}
