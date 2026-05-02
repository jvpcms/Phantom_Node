#include <Arduino.h>

#ifdef ARDUINO_NRF52_ADAFRUIT
  #include <Adafruit_TinyUSB.h>
  #include "lifecycle/factory.hpp"
  #include "logger.hpp"
#endif

// ---------------------------------------------------------------------------
// Build mode: pass -DEMITTER=1 (TX) or -DEMITTER=0 (RX) at compile time.
// Defaults to transmitter if not set.
// ---------------------------------------------------------------------------
#ifndef EMITTER
  #define EMITTER 1
#endif

/** Blocks for N seconds printing a countdown, giving time to open the serial monitor before the lifecycle starts. */
void serial_monitor_delay() {
#ifdef ARDUINO_NRF52_ADAFRUIT
    int delay_seconds = 3;
#else
    int delay_seconds = 3;
#endif
    for (int i = 0; i < delay_seconds; i++) {
        Log::print("Timeout : ");
        Log::print(delay_seconds - i);
        Log::println("s");
        delay(1000);
    }
}

// ---------------------------------------------------------------------------
// nRF52840
// ---------------------------------------------------------------------------
#ifdef ARDUINO_NRF52_ADAFRUIT

/** Waits for the serial monitor, runs the countdown, then starts the lifecycle. */
void setup() {
    if (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) {
        Serial.begin(115200);
        while (!Serial) delay(10);
        serial_monitor_delay();
    }

    // NRF_RADIO requires HFXO (crystal). USB init starts it implicitly on USB-connected
    // boots; on battery we must start it explicitly or the radio busy-waits forever.
    if (!(NRF_CLOCK->HFCLKSTAT & CLOCK_HFCLKSTAT_SRC_Msk)) {
        NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
        NRF_CLOCK->TASKS_HFCLKSTART    = 1;
        while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0);
    }

    OperationMode mode = EMITTER ? OperationMode::TRANSMITTER : OperationMode::RECEIVER;
    getLifeCycle(mode)->startLifeCycle();
}

/** Empty — lifecycle runs to completion in setup(). */
void loop() { }

// ---------------------------------------------------------------------------
// ESP32 — stub
// ---------------------------------------------------------------------------
#else

/** Initialises serial, runs the countdown, then starts the lifecycle. */
void setup() {
    Serial.begin(115200);
    serial_monitor_delay();
}

/** Empty — lifecycle runs to completion in setup(). */
void loop() { }

#endif
