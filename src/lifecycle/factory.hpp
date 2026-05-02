#pragma once

#include "transmitter_lifecycle.hpp"
#include "receiver_lifecycle.hpp"

/** Selects whether the device acts as a transmitter (beacon emitter) or receiver. */
enum class OperationMode {
    TRANSMITTER,
    RECEIVER
};

/** Allocates and returns the LifeCycle instance for the given mode. Caller owns the pointer. */
LifeCycle* getLifeCycle(OperationMode mode) {
    switch (mode) {
        case OperationMode::TRANSMITTER: return new TransmitterLifeCycle();
        case OperationMode::RECEIVER:    return new ReceiverLifeCycle();
    }
    return nullptr;
}
