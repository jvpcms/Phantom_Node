# Phantom_Node

ECDH key exchange and encryption over NIST P-256 targeting the ESP32 microcontroller, with a toy ECC implementation for desktop testing.

## Dependencies

**Desktop build**
- `g++` with C++17 support

**ESP32 build**
- [`arduino-cli`](https://arduino.github.io/arduino-cli/)
- ESP32 Arduino core: `arduino-cli core install esp32:esp32`

## Commands

| Command | Description |
|---|---|
| `make run` | Compile and run the desktop C++ executable |
| `make build` | Compile the sketch for ESP32 |
| `make upload` | Compile and upload to the board |
| `make monitor` | Open the serial monitor (exit with Ctrl+C) |
| `make flash` | Compile, upload, and open the serial monitor |

The serial port defaults to `/dev/ttyUSB0`. Override it with:

```
make flash PORT=/dev/ttyACM0
```

All build artifacts are written to `build/` at the project root.

## Structure

```
src/
  main/
    main.cpp          # Desktop entry point (toy ECC over small curve)
    main.ino          # ESP32 entry point (P-256 via mbedTLS)
  cryptography/
    elliptic_curve_cryptography.hpp      # Toy ECC (long arithmetic)
    mpi_elliptic_curve_cryptography.hpp  # P-256 via mbedTLS MPI
    discrete_log_cryptography.hpp        # DLP-based scheme (toy)
    abstract_cryptography/               # CRTP base classes
  math/
    modular.hpp       # Modular exponentiation
```
