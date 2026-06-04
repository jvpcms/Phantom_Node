# Phantom Node

Encrypted, authenticated radio communication between two nRF52840 nodes. Each session uses ECDH over NIST P-256 to derive a shared secret, ECDSA to authenticate handshake packets, AES-128-CTR to encrypt data, and a PRNG-based frequency hopper seeded from the shared secret to resist interception.

## Protocol overview

1. **Discovery** — transmitter broadcasts signed beacons on a fixed channel; receiver responds when RSSI meets the handshake threshold
2. **Key exchange** — both sides derive the same shared secret via ECDH using the nRF52840 CryptoCell-310
3. **Data transfer** — encrypted packets are sent over a frequency-hopping sequence known only to the paired nodes; each packet is confirmed with ACK/NACK

## Hardware

**Adafruit Feather nRF52840** × 2 — one flashed as transmitter (`-DEMITTER=1`), one as receiver (`-DEMITTER=0`).

## Setup

```
arduino-cli config add board_manager.additional_urls \
    https://adafruit.github.io/arduino-board-index/package_adafruit_index.json
arduino-cli core update-index
arduino-cli core install adafruit:nrf52
make setup-nrf
python3 -m venv .venv && .venv/bin/pip install -r requirements.txt
```

## Commands

| Command | Description |
|---|---|
| `make flash-emit` | Build transmitter firmware, flash via UF2, open serial monitor |
| `make flash-recv` | Build receiver firmware, flash via UF2, open serial monitor |
| `make monitor-emit` | Open serial monitor for transmitter (`PORT1`, default `/dev/ttyACM1`) |
| `make monitor-recv` | Open serial monitor for receiver (`PORT2`, default `/dev/ttyACM0`) |
| `make run` | Run desktop build (toy ECC, no radio) |

Override ports:

```
make flash-emit PORT1=/dev/ttyACM0
```

Build artifacts go to `build/nrf52840/`.

## Structure

```
src/
  main/
    main.ino              # nRF52840 entry point
  lifecycle/
    transmitter_lifecycle.hpp   # beacon, handshake, encrypted TX
    receiver_lifecycle.hpp      # discovery, handshake, encrypted RX
    frequency_hopper.hpp        # xorshift32 channel sequence
    handshake_packet.hpp        # discovery frame (device_id + public key + ECDSA sig)
    data_packet.hpp             # encrypted data frame + ACK/NACK
    lifecycle.hpp               # radio helpers, RSSI filtering, base class
    factory.hpp                 # allocates TX or RX lifecycle by mode
  cryptography/
    nrf_signing_scheme.hpp      # ECDSA P-256 + AES-128-CTR via CryptoCell-310
    nrf_elliptic_curve_cryptography.hpp  # ECDH via CryptoCell-310
    mpi_elliptic_curve_cryptography.hpp  # ECDH via mbedTLS MPI (reference)
    elliptic_curve_cryptography.hpp      # toy ECC over small curve (desktop)
    discrete_log_cryptography.hpp        # toy DLP scheme (desktop)
    abstract_cryptography/              # CRTP base classes
  config.hpp              # RSSI thresholds, timing, channel constants
  logger.hpp              # USB-safe serial logger
  math/
    modular.hpp           # modular exponentiation
  desktop/
    main.cpp              # desktop entry point (toy ECC)
```
