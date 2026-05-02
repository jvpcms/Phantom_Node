#pragma once

#include "lifecycle.hpp"

class ReceiverLifeCycle : public LifeCycle {
public:
    void startLifeCycle() override {
        this->radioInit(DISCOVERY_CHANNEL);
        HandshakePacket peer = this->startDiscovering();
        uint8_t shared[32] = {};
        this->_crypto->computeSharedSecret(peer.content.public_key, shared, sizeof(shared));
        Log::print("Shared secret: ");
        for (uint8_t i = 0; i < sizeof(shared); i++) {
            if (shared[i] < 0x10) Log::print("0");
            Log::print(shared[i], HEX);
            Log::print(" ");
        }
        Log::println();

        this->_crypto->setSharedKey(shared);
        this->_fhop = FHop::fromSecret(shared);
        this->startReceiving();
    }

private:
    void startReceiving() {
        static uint8_t enc_buf[DataPacket::SIZE]  = {};
        static uint8_t data_buf[DataPacket::SIZE] = {};
        static uint8_t ack_buf[AckPacket::SIZE]   = {AckPacket::MAGIC};

        while (true) {
            uint8_t ch = this->_fhop.next();

            this->radioInit(ch, DataPacket::SIZE);
            this->rxPacket(enc_buf, 0xFFFFFFFF);
            this->_crypto->decrypt(enc_buf, data_buf, DataPacket::SIZE);

            constexpr uint8_t PAYLOAD_SIZE = DataPacket::SIZE - 1;
            uint8_t chunk = (data_buf[0] & DataPacket::IS_LAST)
                ? strnlen((char*)&data_buf[1], PAYLOAD_SIZE)
                : PAYLOAD_SIZE;
            memcpy(this->_message + this->_message_len, &data_buf[1], chunk);
            this->_message_len += chunk;

            Log::print("data received ch="); Log::println(ch);
            Log::print("  text: ");
            for (uint8_t j = 0; j < chunk; j++) Log::print((char)data_buf[1 + j]);
            Log::println();
            delay(1000);

            this->radioInit(ch, AckPacket::SIZE);
            this->txPacket(ack_buf);
            Log::print("ack sent      ch="); Log::println(ch);

            if (data_buf[0] & DataPacket::IS_LAST) {
                this->_message[this->_message_len] = '\0';
                Log::println("=== Message ===");
                Log::println(this->_message);
                Log::println("Transmission complete.");
                break;
            }
        }
    }

    HandshakePacket startDiscovering() {
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
