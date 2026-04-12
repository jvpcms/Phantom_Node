#pragma once

#include "lifecycle.hpp"

class TransmitterLifeCycle : public LifeCycle {
public:
    void startLifeCycle() override {
        radioInit(RADIO_CHANNEL_EMIT);
        startDiscoverable();
    }

private:
    void startDiscoverable() {
        // Build content: device_id + public_key
        HandshakePacket beacon;
        beacon.content.device_id[0] = 0xDE; beacon.content.device_id[1] = 0xAD;
        beacon.content.device_id[2] = 0xBE; beacon.content.device_id[3] = 0xEF;
        _crypto->getPublicKey(beacon.content.public_key);

        // Sign the content bytes
        uint32_t sig_size = HandshakePacket::SIGNATURE_SIZE;
        _crypto->sign(
            reinterpret_cast<const uint8_t*>(&beacon.content),
            HandshakePacket::CONTENT_SIZE,
            beacon.signature, sig_size
        );

        static uint8_t tx_buf[HandshakePacket::SIZE];
        static uint8_t rx_buf[HandshakePacket::SIZE];
        beacon.toBytes(tx_buf);

        Serial.println("Discoverable: sending signed beacon...");
        beacon.print();

        while (true) {
            txPacket(tx_buf);
            Serial.println("Beacon sent, listening for 100ms...");

            bool received = rxPacket(rx_buf, 100);

            if (received && memcmp(rx_buf, tx_buf, HandshakePacket::SIZE) == 0) {
                Serial.println("Paired!");
                return;
            }
        }
    }
};
