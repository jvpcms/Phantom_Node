# cryptography

CRTP-based cryptography module implementing Diffie-Hellman key exchange and ElGamal-style encryption. All schemes share the same `encrypt_message` / `decrypt_message` logic in the base class — only the group element changes.

## Schemes

**Discrete Logarithm (`discrete_log_cryptography.hpp`)** — toy model. Group elements are integers mod a prime, using multiplicative group DH. Cryptographic security requires primes of at least 2048 bits; the ESP32's native `long` is 32-bit, so this implementation is for educational purposes only and provides no real security.

**Elliptic Curve — toy (`elliptic_curve_cryptography.hpp`)** — generic ECC over a short Weierstrass curve with `long` coordinates. Runs on desktop for testing the group arithmetic and scheme logic. Not suitable for the ESP32 at cryptographic scale for the same integer-size reason as DL.

**Elliptic Curve — MPI (`mpi_elliptic_curve_cryptography.hpp`)** — production-grade implementation over NIST P-256 (128-bit security) using the mbedTLS MPI library, which leverages the ESP32's hardware bignum accelerator. This is the scheme used in `main.ino`.

## Abstractions

- `GroupElement<Derived>` — CRTP marker; concrete types implement `combine` (group operation) and `invert` (group inverse)
- `CryptographyScheme<Derived, ElementType, ScalarType>` — provides `encrypt_message` and `decrypt_message` on top of any `GroupElement`
