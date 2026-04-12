#pragma once

#include <Arduino.h>

struct HandshakePacket {
    // P-256 sizes
    static constexpr uint8_t DEVICE_ID_SIZE  = 4;
    static constexpr uint8_t PUBLIC_KEY_SIZE = 65; // uncompressed: 04 || X (32) || Y (32)
    static constexpr uint8_t CONTENT_SIZE    = DEVICE_ID_SIZE + PUBLIC_KEY_SIZE; // 69
    static constexpr uint8_t SIGNATURE_SIZE  = 64; // ECDSA P-256: r (32) || s (32)
    static constexpr uint8_t SIZE            = CONTENT_SIZE + SIGNATURE_SIZE;   // 133

    struct Content {
        uint8_t device_id[DEVICE_ID_SIZE];
        uint8_t public_key[PUBLIC_KEY_SIZE];
    } content;

    uint8_t signature[SIGNATURE_SIZE];

    void toBytes(uint8_t* buf) const {
        memcpy(buf,                    &content, CONTENT_SIZE);
        memcpy(buf + CONTENT_SIZE, signature,   SIGNATURE_SIZE);
    }

    static HandshakePacket fromBytes(const uint8_t* buf) {
        HandshakePacket p;
        memcpy(&p.content,   buf,                CONTENT_SIZE);
        memcpy(p.signature,  buf + CONTENT_SIZE, SIGNATURE_SIZE);
        return p;
    }

    void print() const {
        Serial.print("device_id:  ");
        for (uint8_t i = 0; i < DEVICE_ID_SIZE; i++) {
            if (content.device_id[i] < 0x10) Serial.print("0");
            Serial.print(content.device_id[i], HEX);
            Serial.print(" ");
        }
        Serial.println();

        Serial.print("public_key: ");
        for (uint8_t i = 0; i < PUBLIC_KEY_SIZE; i++) {
            if (content.public_key[i] < 0x10) Serial.print("0");
            Serial.print(content.public_key[i], HEX);
            Serial.print(" ");
        }
        Serial.println();

        Serial.print("signature:  ");
        for (uint8_t i = 0; i < SIGNATURE_SIZE; i++) {
            if (signature[i] < 0x10) Serial.print("0");
            Serial.print(signature[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
};
