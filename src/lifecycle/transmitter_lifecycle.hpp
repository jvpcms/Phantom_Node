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

        this->_crypto->setSharedKey(shared);
        this->_fhop = FHop::fromSecret(shared);
        this->startTransmitting();
    }

private:
    void startTransmitting() {
        static const char MESSAGE[] =
            "Lorem ipsum dolor sit amet, consectetur adipiscing elit, "
            "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
            "Ut enim ad minim veniam, quis nostrud exercitation ullamco.";

        constexpr uint16_t MSG_LEN      = sizeof(MESSAGE) - 1;
        constexpr uint8_t  PAYLOAD_SIZE = DataPacket::SIZE - 1;
        constexpr uint16_t TOTAL        = (MSG_LEN + PAYLOAD_SIZE - 1) / PAYLOAD_SIZE;

        uint8_t plain[DataPacket::SIZE]   = {};
        uint8_t enc_buf[DataPacket::SIZE] = {};
        uint8_t ack_buf[AckPacket::SIZE]  = {};

        for (uint16_t i = 0; i < TOTAL; i++) {
            uint16_t offset    = i * PAYLOAD_SIZE;
            uint8_t  chunk     = (MSG_LEN - offset) < PAYLOAD_SIZE ? (MSG_LEN - offset) : PAYLOAD_SIZE;

            plain[0] = (i == TOTAL - 1) ? DataPacket::IS_LAST : 0;
            memset(&plain[1], 0, PAYLOAD_SIZE);
            memcpy(&plain[1], MESSAGE + offset, chunk);

            this->_crypto->encrypt(plain, enc_buf, DataPacket::SIZE);

            uint8_t ch = this->_fhop.next();
            this->radioInit(ch, DataPacket::SIZE);
            this->txPacket(enc_buf);
            Log::print("data sent     ch="); Log::println(ch);
            Log::print("  plain: "); this->logHex(plain, DataPacket::SIZE);
            Log::print("  enc:   "); this->logHex(enc_buf, DataPacket::SIZE);

            this->radioInit(ch, AckPacket::SIZE);
            this->rxPacket(ack_buf, 0xFFFFFFFF);
            Log::print("ack received  ch="); Log::println(ch);
            delay(1000);
        }
        Log::println("Transmission complete.");
    }

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
