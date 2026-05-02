#pragma once

#include "lifecycle.hpp"

class ReceiverLifeCycle : public LifeCycle {
public:
    void startLifeCycle() override {
        this->radioInit(DISCOVERY_CHANNEL);
        HandshakePacket peer = this->startListening();
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
    HandshakePacket startListening() {
        const uint8_t id[] = {0xCA, 0xFE, 0xBA, 0xBE};
        HandshakePacket response = HandshakePacket::build(id, this->_crypto);

        Log::println("=== Receiver ===");
        response.print();

        static uint8_t tx_buf[HandshakePacket::SIZE];
        response.toBytes(tx_buf);

        static uint8_t rx_buf[HandshakePacket::SIZE] = {};

        while (true) {
            if (!this->rxPacket(rx_buf, 0xFFFFFFFF)) continue;

            uint8_t rssi = NRF_RADIO->RSSISAMPLE;
            if (rssi >= (uint8_t)(-RSSI_HANDSHAKE_THRESHOLD_DBM)) continue;

            HandshakePacket beacon = HandshakePacket::fromBytes(rx_buf);

            Log::println("=== Received beacon ===");
            beacon.print();

            delay(1); // turnaround guard
            this->txPacket(tx_buf);
            Log::println("Response sent.");
            return beacon;
        }
    }
};
