#pragma once

#include "lifecycle.hpp"

class TransmitterLifeCycle : public LifeCycle {
public:
    void startLifeCycle() override {
        radioInit(DISCOVERY_CHANNEL);
        startDiscoverable();
    }

private:
    void startDiscoverable() {
        HandshakePacket beacon;
        beacon.content.device_id[0] = 0xDE; beacon.content.device_id[1] = 0xAD;
        beacon.content.device_id[2] = 0xBE; beacon.content.device_id[3] = 0xEF;
        _crypto->getPublicKey(beacon.content.public_key);

        uint32_t sig_size = HandshakePacket::SIGNATURE_SIZE;
        _crypto->sign(
            reinterpret_cast<const uint8_t*>(&beacon.content),
            HandshakePacket::CONTENT_SIZE,
            beacon.signature, sig_size
        );

        static uint8_t tx_buf[HandshakePacket::SIZE];
        static uint8_t rx_buf[HandshakePacket::SIZE];
        beacon.toBytes(tx_buf);

        Log::println("=== Emitter ===");
        beacon.print();

        while (true) {
            txPacket(tx_buf);

            if (!rxPacket(rx_buf, BEACON_INTERVAL_MS)) continue;

            HandshakePacket response = HandshakePacket::fromBytes(rx_buf);

            Log::println("=== Received handshake ===");
            response.print();

            bool valid = _crypto->verify(
                reinterpret_cast<const uint8_t*>(&response.content),
                HandshakePacket::CONTENT_SIZE,
                response.signature,
                HandshakePacket::SIGNATURE_SIZE,
                response.content.public_key
            );

            Log::println(valid ? "Signature valid — paired." : "Signature invalid — ignoring.");

            if (valid) return;
        }
    }
};
