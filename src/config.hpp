#pragma once

#include <Arduino.h>

// Radio channel for discovery (frequency = 2400 + DISCOVERY_CHANNEL MHz)
static constexpr uint8_t  DISCOVERY_CHANNEL            = 255;

// How long the emitter listens for a handshake response after each beacon (ms)
static constexpr uint32_t BEACON_INTERVAL_MS           = 1000;

// Minimum RSSI for the receiver to respond to a handshake (dBm).
// Signals weaker than this are ignored.
static constexpr int8_t   RSSI_HANDSHAKE_THRESHOLD_DBM = -70;

// Minimum RSSI for any data-phase packet (data, ACK, NACK).
// Packets weaker than this are treated as lost regardless of CRC.
static constexpr int8_t   RSSI_DATA_THRESHOLD_DBM      = -75;

// How long TX waits after receiving ACK before sending on ch N+1.
// Gives RX time to complete its radio turnaround (~47 us); 5 ms is ~100x margin.
static constexpr uint32_t TURNAROUND_GUARD_MS = 1;

// How long RX waits for a data packet before sending a NACK.
static constexpr uint32_t RX_TIMEOUT_MS = 500;

// How long RX listens on ch N+1 before re-sending ACK on ch N (lost-ACK recovery).
static constexpr uint32_t RX_WINDOW_MS = 50;
