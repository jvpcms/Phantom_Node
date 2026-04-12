#pragma once

#include <Arduino.h>

// Radio channel for discovery (frequency = 2400 + DISCOVERY_CHANNEL MHz)
static constexpr uint8_t  DISCOVERY_CHANNEL            = 255;

// How long the emitter listens for a handshake response after each beacon (ms)
static constexpr uint32_t BEACON_INTERVAL_MS           = 100;

// Minimum RSSI for the receiver to respond to a handshake (dBm).
// Signals weaker than this are ignored.
static constexpr int8_t   RSSI_HANDSHAKE_THRESHOLD_DBM = -50;
