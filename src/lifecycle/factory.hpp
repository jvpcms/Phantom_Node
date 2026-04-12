#pragma once

#include "transmitter_lifecycle.hpp"
#include "receiver_lifecycle.hpp"

enum class OperationMode {
    TRANSMITTER,
    RECEIVER
};

LifeCycle* getLifeCycle(OperationMode mode) {
    switch (mode) {
        case OperationMode::TRANSMITTER: return new TransmitterLifeCycle();
        case OperationMode::RECEIVER:    return new ReceiverLifeCycle();
    }
    return nullptr;
}
