#pragma once

#include <Arduino.h>

/**
 * USB-safe logger. All methods are no-ops when USB VBUS is not present,
 * preventing Serial writes from blocking on battery-powered operation.
 */
namespace Log {

inline bool available() {
    return NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk;
}

template<typename T>
inline void print(T value) {
    if (available()) Serial.print(value);
}

template<typename T>
inline void print(T value, int fmt) {
    if (available()) Serial.print(value, fmt);
}

template<typename T>
inline void println(T value) {
    if (available()) Serial.println(value);
}

inline void println() {
    if (available()) Serial.println();
}

}
