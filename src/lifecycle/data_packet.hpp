#pragma once

#include <stdint.h>

struct DataPacket {
    static constexpr uint8_t IS_LAST = 0x01;
    uint8_t flags;
    uint8_t payload[15];
    static constexpr uint8_t SIZE = 16;
};

struct ResponsePacket {
    static constexpr uint8_t ACK  = 0xAC;
    static constexpr uint8_t NACK = 0x4E;
    uint8_t type;
    uint8_t _pad[3];
    static constexpr uint8_t SIZE = 4;
};
