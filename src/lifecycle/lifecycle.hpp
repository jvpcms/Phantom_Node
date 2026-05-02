#pragma once

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "handshake_packet.hpp"
#include "cryptography/signing_scheme.hpp"
#include "cryptography/nrf_signing_scheme.hpp"
#include "config.hpp"
#include "frequency_hopper.hpp"
#include "data_packet.hpp"

class LifeCycle {
public:
    LifeCycle() {
        this->_crypto = new NrfSigningScheme();
        this->_crypto->begin();
    }

    virtual void startLifeCycle() = 0;

    virtual ~LifeCycle() {
        delete this->_crypto;
    }

protected:
    SigningScheme* _crypto;
    FHop           _fhop;
    char           _message[256]  = {};
    uint16_t       _message_len   = 0;

protected:
    static void logHex(const uint8_t* buf, uint8_t len) {
        for (uint8_t i = 0; i < len; i++) {
            if (buf[i] < 0x10) Log::print("0");
            Log::print(buf[i], HEX);
            Log::print(" ");
        }
        Log::println();
    }

    static void radioInit(uint8_t channel, uint8_t packet_len = HandshakePacket::SIZE) {
        NRF_RADIO->POWER = 0;
        NRF_RADIO->POWER = 1;

        NRF_RADIO->MODE      = RADIO_MODE_MODE_Nrf_1Mbit << RADIO_MODE_MODE_Pos;
        NRF_RADIO->FREQUENCY = channel;

        NRF_RADIO->BASE0       = 0x12345678UL;
        NRF_RADIO->PREFIX0     = 0xABUL;
        NRF_RADIO->TXADDRESS   = 0;
        NRF_RADIO->RXADDRESSES = 1;

        NRF_RADIO->PCNF0 = 0;
        NRF_RADIO->PCNF1 = (packet_len << RADIO_PCNF1_MAXLEN_Pos)  |
                           (packet_len << RADIO_PCNF1_STATLEN_Pos) |
                           (4          << RADIO_PCNF1_BALEN_Pos);

        NRF_RADIO->CRCCNF  = (RADIO_CRCCNF_LEN_Two      << RADIO_CRCCNF_LEN_Pos) |
                             (RADIO_CRCCNF_SKIPADDR_Skip << RADIO_CRCCNF_SKIPADDR_Pos);
        NRF_RADIO->CRCPOLY = 0x11021;
        NRF_RADIO->CRCINIT = 0xFFFF;

        // Start RSSI measurement automatically on address match.
        NRF_RADIO->SHORTS = RADIO_SHORTS_ADDRESS_RSSISTART_Msk;
    }

    static void txPacket(uint8_t* buf) {
        NRF_RADIO->PACKETPTR    = (uint32_t)buf;
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_TXEN   = 1;
        while (NRF_RADIO->EVENTS_READY == 0);

        NRF_RADIO->EVENTS_END = 0;
        NRF_RADIO->TASKS_START = 1;
        while (NRF_RADIO->EVENTS_END == 0);

        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TASKS_DISABLE   = 1;
        while (NRF_RADIO->EVENTS_DISABLED == 0);
    }

    // Returns true if a packet was received before timeout_ms elapsed.
    static bool rxPacket(uint8_t* buf, uint32_t timeout_ms) {
        NRF_RADIO->PACKETPTR    = (uint32_t)buf;
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->TASKS_RXEN   = 1;
        while (NRF_RADIO->EVENTS_READY == 0);

        NRF_RADIO->EVENTS_END = 0;
        NRF_RADIO->TASKS_START = 1;

        uint32_t start = millis();
        while (NRF_RADIO->EVENTS_END == 0) {
            if (millis() - start >= timeout_ms) {
                NRF_RADIO->EVENTS_DISABLED = 0;
                NRF_RADIO->TASKS_DISABLE   = 1;
                while (NRF_RADIO->EVENTS_DISABLED == 0);
                return false;
            }
        }

        NRF_RADIO->EVENTS_DISABLED = 0;
        NRF_RADIO->TASKS_DISABLE   = 1;
        while (NRF_RADIO->EVENTS_DISABLED == 0);

        return NRF_RADIO->CRCSTATUS == RADIO_CRCSTATUS_CRCSTATUS_CRCOk;
    }

    // Returns true if the last received packet's RSSI is above the handshake threshold.
    static bool rssiOk() {
        return NRF_RADIO->RSSISAMPLE < (uint8_t)(-RSSI_HANDSHAKE_THRESHOLD_DBM);
    }
};
