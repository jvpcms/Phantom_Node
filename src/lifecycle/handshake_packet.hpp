#pragma once

#include <Arduino.h>
#include "logger.hpp"
#include "cryptography/signing_scheme.hpp"

/** Wire representation of a discovery handshake: device_id + public_key + ECDSA signature. */
struct HandshakePacket {
    static constexpr uint8_t DEVICE_ID_SIZE  = 4;
    static constexpr uint8_t PUBLIC_KEY_SIZE = 65;
    static constexpr uint8_t CONTENT_SIZE    = DEVICE_ID_SIZE + PUBLIC_KEY_SIZE;
    static constexpr uint8_t SIGNATURE_SIZE  = 64;
    static constexpr uint8_t SIZE            = CONTENT_SIZE + SIGNATURE_SIZE;

    /** Plain fields that are signed: device_id (4 bytes) and public_key (65-byte uncompressed P-256 point). */
    struct Content {
        uint8_t device_id[DEVICE_ID_SIZE];
        uint8_t public_key[PUBLIC_KEY_SIZE];
    } content;

    uint8_t signature[SIGNATURE_SIZE];

    /** Serializes the packet into a flat byte buffer of at least SIZE bytes. */
    void toBytes(uint8_t* buf) const {
        memcpy(buf,               &content,  CONTENT_SIZE);
        memcpy(buf + CONTENT_SIZE, signature, SIGNATURE_SIZE);
    }

    /** Constructs and signs a new packet using the given device_id and crypto context. */
    static HandshakePacket build(const uint8_t device_id[DEVICE_ID_SIZE], SigningScheme* crypto) {
        HandshakePacket p;
        memcpy(p.content.device_id, device_id, DEVICE_ID_SIZE);
        crypto->getPublicKey(p.content.public_key);
        uint32_t sig_size = SIGNATURE_SIZE;
        crypto->sign(
            reinterpret_cast<const uint8_t*>(&p.content),
            CONTENT_SIZE,
            p.signature, sig_size
        );
        return p;
    }

    /** Deserializes a packet from a flat byte buffer of at least SIZE bytes. */
    static HandshakePacket fromBytes(const uint8_t* buf) {
        HandshakePacket p;
        memcpy(&p.content,  buf,                CONTENT_SIZE);
        memcpy(p.signature, buf + CONTENT_SIZE, SIGNATURE_SIZE);
        return p;
    }

    /** Logs device_id, public_key, and signature as hex to the serial output. */
    void print() const {
        Log::print("device_id:  ");
        for (uint8_t i = 0; i < DEVICE_ID_SIZE; i++) {
            if (content.device_id[i] < 0x10) Log::print("0");
            Log::print(content.device_id[i], HEX);
            Log::print(" ");
        }
        Log::println();

        Log::print("public_key: ");
        for (uint8_t i = 0; i < PUBLIC_KEY_SIZE; i++) {
            if (content.public_key[i] < 0x10) Log::print("0");
            Log::print(content.public_key[i], HEX);
            Log::print(" ");
        }
        Log::println();

        Log::print("signature:  ");
        for (uint8_t i = 0; i < SIGNATURE_SIZE; i++) {
            if (signature[i] < 0x10) Log::print("0");
            Log::print(signature[i], HEX);
            Log::print(" ");
        }
        Log::println();
    }
};
