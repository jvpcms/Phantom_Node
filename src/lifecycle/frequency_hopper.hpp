#pragma once

#include <stdint.h>
#include <string.h>

static constexpr uint8_t  FHOP_CHANNEL_MIN   = 0;
static constexpr uint8_t  FHOP_CHANNEL_MAX   = 99;
static constexpr uint8_t  FHOP_CHANNEL_COUNT = FHOP_CHANNEL_MAX - FHOP_CHANNEL_MIN + 1;

/** xorshift32 PRNG-based frequency hopper producing channels in [FHOP_CHANNEL_MIN, FHOP_CHANNEL_MAX]. */
class FHop {
public:
    /** Seeds with 1 (xorshift32 requires a non-zero state). */
    FHop() : _state(1) {}

    /** Seeds with the given value; substitutes 1 if seed is zero. */
    explicit FHop(uint32_t seed) : _state(seed == 0 ? 1 : seed) {}

    /** Derives the seed from the first 4 bytes of a 32-byte ECDH shared secret. */
    static FHop fromSecret(const uint8_t* shared_secret) {
        uint32_t seed;
        memcpy(&seed, shared_secret, sizeof(seed));
        return FHop(seed);
    }

    /** Advances the xorshift32 state and returns the next channel in [FHOP_CHANNEL_MIN, FHOP_CHANNEL_MAX]. */
    uint8_t next() {
        this->_state ^= this->_state << 13;
        this->_state ^= this->_state >> 17;
        this->_state ^= this->_state << 5;
        return FHOP_CHANNEL_MIN + (this->_state % FHOP_CHANNEL_COUNT);
    }

private:
    uint32_t _state;
};
