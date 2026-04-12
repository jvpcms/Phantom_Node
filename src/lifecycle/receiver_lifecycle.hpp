#pragma once

#include "lifecycle.hpp"

class ReceiverLifeCycle : public LifeCycle {
public:
    void startLifeCycle() override {
        radioInit(RADIO_CHANNEL_RECV);
        startScanning();
    }

private:
    void startScanning() {
        static uint8_t rx_buf[HandshakePacket::SIZE] = {};

        Serial.println("Scanning...");

        while (true) {
            bool received = rxPacket(rx_buf, 0xFFFFFFFF);
            if (!received) continue;

            HandshakePacket packet = HandshakePacket::fromBytes(rx_buf);

            // Verification happens here — trust boundary before responding.
            // Uses the sender's public key embedded in content to verify
            // the signature over the content bytes. CC310 handles SHA-256 + ECDSA internally.
            bool valid = _crypto->verify(
                reinterpret_cast<const uint8_t*>(&packet.content),
                HandshakePacket::CONTENT_SIZE,
                packet.signature,
                HandshakePacket::SIGNATURE_SIZE,
                packet.content.public_key
            );

            if (!valid) {
                Serial.println("Beacon received — invalid signature, ignoring.");
                continue;
            }

            Serial.println("Beacon received — signature valid.");
            packet.print();

            delay(1); // turnaround guard
            txPacket(rx_buf);
            Serial.println("Response sent.");
        }
    }
};
