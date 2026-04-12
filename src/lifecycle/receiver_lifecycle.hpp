#pragma once

#include "lifecycle.hpp"

class ReceiverLifeCycle : public LifeCycle {
public:
    void startLifeCycle() override {
        radioInit(DISCOVERY_CHANNEL);

        // Build response packet once — same keys for every response.
        HandshakePacket response;
        response.content.device_id[0] = 0xCA; response.content.device_id[1] = 0xFE;
        response.content.device_id[2] = 0xBA; response.content.device_id[3] = 0xBE;
        _crypto->getPublicKey(response.content.public_key);

        uint32_t sig_size = HandshakePacket::SIGNATURE_SIZE;
        _crypto->sign(
            reinterpret_cast<const uint8_t*>(&response.content),
            HandshakePacket::CONTENT_SIZE,
            response.signature, sig_size
        );

        Log::println("=== Receiver ===");
        response.print();

        static uint8_t tx_buf[HandshakePacket::SIZE];
        response.toBytes(tx_buf);

        startListening(tx_buf);
    }

private:
    void startListening(uint8_t* tx_buf) {
        static uint8_t rx_buf[HandshakePacket::SIZE] = {};

        while (true) {
            if (!rxPacket(rx_buf, 0xFFFFFFFF)) continue;

            uint8_t rssi = NRF_RADIO->RSSISAMPLE;
            if (rssi >= (uint8_t)(-RSSI_HANDSHAKE_THRESHOLD_DBM)) continue; // signal too weak

            HandshakePacket beacon = HandshakePacket::fromBytes(rx_buf);

            Log::println("=== Received beacon ===");
            beacon.print();

            delay(1); // turnaround guard
            txPacket(tx_buf);
            Log::println("Response sent.");
        }
    }
};
