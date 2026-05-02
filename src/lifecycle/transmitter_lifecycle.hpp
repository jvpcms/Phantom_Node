#pragma once

#include "lifecycle.hpp"

class TransmitterLifeCycle : public LifeCycle {
public:
    void startLifeCycle() override {
        this->radioInit(DISCOVERY_CHANNEL);
        HandshakePacket peer = this->startDiscoverable();
        uint8_t shared[32] = {};
        this->_crypto->computeSharedSecret(peer.content.public_key, shared, sizeof(shared));
        Log::print("Shared secret: ");
        for (uint8_t i = 0; i < sizeof(shared); i++) {
            if (shared[i] < 0x10) Log::print("0");
            Log::print(shared[i], HEX);
            Log::print(" ");
        }
        Log::println();
    }

private:
    HandshakePacket startDiscoverable() {
        const uint8_t id[] = {0xDE, 0xAD, 0xBE, 0xEF};
        HandshakePacket beacon = HandshakePacket::build(id, this->_crypto);

        static uint8_t tx_buf[HandshakePacket::SIZE];
        static uint8_t rx_buf[HandshakePacket::SIZE];
        beacon.toBytes(tx_buf);

        Log::println("=== Emitter ===");
        beacon.print();

        while (true) {
            this->txPacket(tx_buf);

            if (!this->rxPacket(rx_buf, BEACON_INTERVAL_MS)) continue;

            HandshakePacket response = HandshakePacket::fromBytes(rx_buf);

            Log::println("=== Received handshake ===");
            response.print();

            bool valid = this->_crypto->verify(
                reinterpret_cast<const uint8_t*>(&response.content),
                HandshakePacket::CONTENT_SIZE,
                response.signature,
                HandshakePacket::SIGNATURE_SIZE,
                response.content.public_key
            );

            Log::println(valid ? "Signature valid — paired." : "Signature invalid — ignoring.");

            if (valid) return response;
        }
    }
};
