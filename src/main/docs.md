# Phantom_Node

Phantom_Node is a cryptographic communication node targeting the ESP32 microcontroller. It implements Diffie-Hellman key exchange and ElGamal-style encryption over elliptic curves, using NIST P-256 via the mbedTLS library that ships with the ESP32 Arduino core.

The project also includes toy implementations of the same schemes over small integer groups and small curves, used for desktop testing and for understanding the underlying mathematics without the complexity of arbitrary-precision arithmetic.

## Goals

- Establish a shared secret between two nodes over an untrusted channel using ECDH
- Encrypt and decrypt messages using the shared secret as a group mask
- Run on constrained hardware (ESP32, ~320 KB RAM, 32-bit integers) with cryptographic-grade security via mbedTLS

## Entry points

**`main.ino`** — ESP32 build. Runs the full P-256 ECDH key exchange and encryption cycle over Serial, using `ECDHMpiScheme` backed by mbedTLS MPI. Output is printed as uncompressed hex point coordinates (SEC 1 format, `04 || X || Y`).

**`main.cpp`** — Desktop build. Runs the same key exchange and encryption cycle using the toy `ECDHScheme` over a small hand-picked curve (`y² = x³ + 2x + 3 mod 97`, generator `G = (3, 6)`). Used to verify the group arithmetic and scheme logic without an ESP32.

## Key exchange overview

```
Alice                              Bob
  |                                 |
  |  alice_priv (scalar)            |  bob_priv (scalar)
  |  alice_pub  = alice_priv * G    |  bob_pub  = bob_priv * G
  |                                 |
  |  <-------- exchange public keys -------->
  |                                 |
  |  shared = alice_priv * bob_pub  |  shared = bob_priv * alice_pub
  |         = alice_priv * bob_priv * G     (same value on both sides)
```

Encryption: `ciphertext = message + shared`
Decryption: `message    = ciphertext + (-shared)`

Both operations are point addition on the curve, so decryption is exact with no information loss.

## Hardware

Tested on **ESP32-D0WD-V3** (revision 3.1), 240 MHz dual-core, 4 MB flash.
The mbedTLS MPI operations use the ESP32's hardware bignum accelerator (RSA accelerator peripheral), which speeds up the modular multiplications underlying P-256 scalar multiplication.
