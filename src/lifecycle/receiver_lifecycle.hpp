#pragma once

#include "lifecycle.hpp"

class ReceiverLifeCycle : public LifeCycle {
public:
    void startLifeCycle() override {
        this->radioInit(DISCOVERY_CHANNEL);
        HandshakePacket peer = this->startDiscovering();
        uint8_t shared[32] = {};
        this->_crypto->computeSharedSecret(peer.content.public_key, shared, sizeof(shared));
        this->_crypto->setSharedKey(shared);
        this->_fhop = FHop::fromSecret(shared);
        this->startReceiving();
    }

private:
    void startReceiving() {
        static uint8_t enc_buf[DataPacket::SIZE]       = {};
        static uint8_t data_buf[DataPacket::SIZE]      = {};
        static uint8_t ack_buf[ResponsePacket::SIZE]   = {ResponsePacket::ACK,  0, 0, 0};
        static uint8_t nack_buf[ResponsePacket::SIZE]  = {ResponsePacket::NACK, 0, 0, 0};

        uint8_t ch        = this->_fhop.next();
        bool    have_data = false;

        while (true) {
            if (!have_data) {
                this->radioInit(ch, DataPacket::SIZE);
                have_data = this->rxPacket(enc_buf, RX_TIMEOUT_MS);
                if (!have_data) {
                    this->radioInit(ch, ResponsePacket::SIZE);
                    this->txPacket(nack_buf);
                    continue;
                }
            }
            have_data = false;

            this->_crypto->decrypt(enc_buf, data_buf, DataPacket::SIZE);

            constexpr uint8_t PAYLOAD_SIZE = DataPacket::SIZE - 1;
            uint8_t chunk = (data_buf[0] & DataPacket::IS_LAST)
                ? strnlen((char*)&data_buf[1], PAYLOAD_SIZE)
                : PAYLOAD_SIZE;
            memcpy(this->_message + this->_message_len, &data_buf[1], chunk);
            this->_message_len += chunk;

            this->radioInit(ch, ResponsePacket::SIZE);
            this->txPacket(ack_buf);

            if (data_buf[0] & DataPacket::IS_LAST) {
                this->_message[this->_message_len] = '\0';
                Log::println("=== Message ===");
                Log::println(this->_message);
                Log::println("Transmission complete.");
                break;
            }

            // Time-division: listen on ch_next; re-send ACK on ch if TX missed it.
            uint8_t ch_next = this->_fhop.next();
            while (true) {
                this->radioInit(ch_next, DataPacket::SIZE);
                if (this->rxPacket(enc_buf, RX_WINDOW_MS)) {
                    have_data = true;
                    break;
                }
                this->radioInit(ch, ResponsePacket::SIZE);
                this->txPacket(ack_buf);
            }
            ch = ch_next;
        }
    }

    HandshakePacket startDiscovering() {
        const uint8_t id[] = {0xCA, 0xFE, 0xBA, 0xBE};
        HandshakePacket response = HandshakePacket::build(id, this->_crypto);

        Log::println("=== Receiver ===");

        static uint8_t tx_buf[HandshakePacket::SIZE];
        response.toBytes(tx_buf);

        static uint8_t rx_buf[HandshakePacket::SIZE] = {};

        while (true) {
            if (!this->rxPacket(rx_buf, 0xFFFFFFFF)) continue;

            uint8_t rssi = NRF_RADIO->RSSISAMPLE;
            if (rssi >= (uint8_t)(-RSSI_HANDSHAKE_THRESHOLD_DBM)) continue;

            HandshakePacket beacon = HandshakePacket::fromBytes(rx_buf);

            delay(1); // turnaround guard
            this->txPacket(tx_buf);
            return beacon;
        }
    }
};
