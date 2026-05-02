#pragma once

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "handshake_packet.hpp"
#include "cryptography/signing_scheme.hpp"
#include "cryptography/nrf_signing_scheme.hpp"
#include "config.hpp"

#define PACKET_LEN HandshakePacket::SIZE

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

protected:
    static void radioInit(uint8_t channel) {
        NRF_RADIO->POWER = 0;
        NRF_RADIO->POWER = 1;

        NRF_RADIO->MODE      = RADIO_MODE_MODE_Nrf_1Mbit << RADIO_MODE_MODE_Pos;
        NRF_RADIO->FREQUENCY = channel;

        NRF_RADIO->BASE0       = 0x12345678UL;
        NRF_RADIO->PREFIX0     = 0xABUL;
        NRF_RADIO->TXADDRESS   = 0;
        NRF_RADIO->RXADDRESSES = 1;

        NRF_RADIO->PCNF0 = 0;
        NRF_RADIO->PCNF1 = (PACKET_LEN << RADIO_PCNF1_MAXLEN_Pos)  |
                           (PACKET_LEN << RADIO_PCNF1_STATLEN_Pos) |
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
};
