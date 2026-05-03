# Phantom_Node — API Reference

## Structure

```
src/
├── config.hpp
├── cryptography
│   ├── abstract_cryptography
│   │   ├── cryptography_scheme.cpp
│   │   ├── cryptography_scheme.hpp
│   │   ├── group_element.cpp
│   │   └── group_element.hpp
│   ├── discrete_log_cryptography.hpp
│   ├── docs.md
│   ├── elliptic_curve_cryptography.hpp
│   ├── mpi_elliptic_curve_cryptography.hpp
│   ├── nrf_elliptic_curve_cryptography.hpp
│   ├── nrf_signing_scheme.hpp
│   └── signing_scheme.hpp
├── desktop
│   └── main.cpp
├── lifecycle
│   ├── data_packet.hpp
│   ├── factory.hpp
│   ├── frequency_hopper.hpp
│   ├── handshake_packet.hpp
│   ├── lifecycle.hpp
│   ├── receiver_lifecycle.hpp
│   └── transmitter_lifecycle.hpp
├── logger.hpp
├── main
│   ├── docs.md
│   └── main.ino
└── math
    ├── docs.md
    └── modular.hpp
```

---

## main


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

---

## cryptography


CRTP-based cryptography module implementing Diffie-Hellman key exchange and ElGamal-style encryption. All schemes share the same `encrypt_message` / `decrypt_message` logic in the base class — only the group element changes.

## Schemes

**Discrete Logarithm (`discrete_log_cryptography.hpp`)** — toy model. Group elements are integers mod a prime, using multiplicative group DH. Cryptographic security requires primes of at least 2048 bits; the ESP32's native `long` is 32-bit, so this implementation is for educational purposes only and provides no real security.

**Elliptic Curve — toy (`elliptic_curve_cryptography.hpp`)** — generic ECC over a short Weierstrass curve with `long` coordinates. Runs on desktop for testing the group arithmetic and scheme logic. Not suitable for the ESP32 at cryptographic scale for the same integer-size reason as DL.

**Elliptic Curve — MPI (`mpi_elliptic_curve_cryptography.hpp`)** — production-grade implementation over NIST P-256 (128-bit security) using the mbedTLS MPI library, which leverages the ESP32's hardware bignum accelerator. This is the scheme used in `main.ino`.

## Abstractions

- `GroupElement<Derived>` — CRTP marker; concrete types implement `combine` (group operation) and `invert` (group inverse)
- `CryptographyScheme<Derived, ElementType, ScalarType>` — provides `encrypt_message` and `decrypt_message` on top of any `GroupElement`

### `cryptography/abstract_cryptography/cryptography_scheme.hpp`

#### `template <typename Derived, typename ElementType, typename ScalarType> class CryptographyScheme`

Base for a public-key cryptography scheme built on a group, using CRTP.
Models the Diffie-Hellman family: a private key is a scalar, the public
key is generator^private_key, and a shared secret is derived by applying
the same operation to the other party's public key. Encryption and
decryption mask/unmask a message via the group operation.
No virtual methods — all dispatch is resolved at compile time.
encrypt_message and decrypt_message are provided here as shared algorithms;
compute_public_value, compute_shared_secret, and create_message must be
implemented by Derived.
@tparam Derived       The concrete scheme subclass (CRTP pattern).
@tparam ElementType   A GroupElement subclass representing a group element.
@tparam ScalarType    The type used for private keys and raw message values.

#### `ElementType encrypt_message(const ElementType& M, const ElementType& shared_secret)`

Encrypts message M by combining it with the shared secret.

#### `ElementType decrypt_message(const ElementType& C, const ElementType& shared_secret)`

Decrypts ciphertext C by combining it with the inverse of the shared secret.

### `cryptography/abstract_cryptography/group_element.hpp`

#### `template <typename Derived> class GroupElement`

Marker base for a group element using CRTP.
A group is a set with a binary operation (combine) that is closed,
associative, has an identity element, and where every element has an
inverse. Concrete subclasses define the group — e.g. integers mod a
prime under multiplication, or points on an elliptic curve under addition.
No virtual methods — dispatch is resolved at compile time through the
template parameter, eliminating vtable overhead and enabling inlining.
No heap allocation — operations return by value and live on the stack.
Derived must implement:
Derived combine(const Derived& other) const;
Derived invert() const;
@tparam Derived  The concrete subclass (CRTP pattern).

### `cryptography/discrete_log_cryptography.hpp`

#### `class IntElement : public GroupElement<IntElement>`

A group element representing an integer modulo a prime, under multiplication.
The group operation (combine) is modular multiplication: (a * b) mod p.
The inverse is computed via Fermat's Little Theorem: a^(p-2) mod p,
which is valid only when p is prime.
Returns by value — no heap allocation, lives on the stack.
NOTE: uses native long (32-bit on ESP32). This limits values and moduli
to ~2 billion, making this unsuitable for real cryptographic security.
For production use, replace with an arbitrary-precision type (e.g. mbedtls_mpi).

#### `IntElement combine(const IntElement& other) const`

Returns (this * other) mod p.

#### `IntElement invert() const`

Returns the modular inverse via Fermat's Little Theorem: value^(p-2) mod p.

#### `class DLPScheme : public CryptographyScheme<DLPScheme, IntElement, long>`

Toy implementation of a Diffie-Hellman scheme over integers mod a prime.
Implements the discrete logarithm problem (DLP) in the multiplicative
group Z_p*: the public key is g^x mod p, and the shared secret is
(other's public key)^x mod p = g^(xy) mod p.
TOY MODEL — NOT CRYPTOGRAPHICALLY SECURE:
- Uses native long (32-bit on ESP32), limiting the prime p to ~2 billion.
- Real DLP security requires primes of at least 2048 bits.
- Intended for educational purposes and testing the scheme abstraction.
- For production, use DLPMpiScheme (mbedtls_mpi) or an elliptic curve scheme.

#### `IntElement compute_public_value(long private_key)`

Computes g^private_key mod p.

#### `IntElement compute_shared_secret(const IntElement& other_public, long private_key)`

Computes other_public^private_key mod p (= g^(ab) mod p).

#### `IntElement create_message(long value)`

Wraps a scalar message value into an IntElement with the scheme's modulus.

### `cryptography/elliptic_curve_cryptography.hpp`

#### `class ECPoint : public GroupElement<ECPoint>`

A point on a short Weierstrass elliptic curve: y² = x³ + ax + b (mod p).
The group operation (combine) is point addition via the chord-and-tangent
rule. The identity element is the point at infinity. The inverse of a
point (x, y) is (x, -y mod p).
Returns by value — no heap allocation, lives on the stack.
TOY MODEL — NOT CRYPTOGRAPHICALLY SECURE:
- Uses native long (32-bit on ESP32). Intermediate multiplications
can overflow for primes larger than ~46340 (sqrt of 2^31).
- Real ECC uses 256-bit primes. For production, replace long with
mbedtls_mpi and use mbedtls_ecp_point for the group operation.

#### `ECPoint() : x(0), y(0), a(0), b(0), p(0), is_infinity(true)`

Default constructor — uninitialized, required by CryptographyScheme.

#### `ECPoint(long a, long b, long p) : x(0), y(0), a(a), b(b), p(p), is_infinity(true)`

Constructs the identity element (point at infinity).

#### `ECPoint(long x, long y, long a, long b, long p) : x(x), y(y), a(a), b(b), p(p), is_infinity(false)`

Constructs a regular point (x, y) on the curve.

#### `ECPoint combine(const ECPoint& other) const`

Returns this + other using the chord-and-tangent rule.
Cases handled:
- Either operand is infinity → return the other
- P + (-P) → return infinity
- P == Q   → point doubling (tangent line)
- P != Q   → point addition (secant line)

#### `ECPoint invert() const`

Returns (x, -y mod p) — the reflection of this point over the x-axis.

#### `class ECDHScheme : public CryptographyScheme<ECDHScheme, ECPoint, long>`

Toy ECDH scheme over a short Weierstrass curve.
The private key is a scalar k. The public key is k*G (scalar multiplication
of the generator point G). The shared secret is k*(other's public key) = k*l*G.
Encryption masks a message point M by adding the shared secret: C = M + S.
Decryption recovers M via: M = C + (-S).
Scalar multiplication uses the double-and-add algorithm: O(log k) point additions.
TOY MODEL — NOT CRYPTOGRAPHICALLY SECURE:
- Uses native long (32-bit on ESP32), limiting safe primes to ~46340.
- Real ECDH uses 256-bit curves (e.g. P-256, secp256k1).
- For production, use mbedTLS: mbedtls_ecdh_compute_shared().

#### `ECPoint compute_public_value(long k)`

Computes k*G via double-and-add.

#### `ECPoint compute_shared_secret(const ECPoint& other_public, long k)`

Computes k*(other's public key) = k*l*G.

#### `ECPoint create_message(long value)`

Embeds a scalar message as a curve point: value*G.

#### `ECPoint scalar_mul(ECPoint P, long k) const`

Double-and-add scalar multiplication: computes k*P in O(log k) steps.

### `cryptography/mpi_elliptic_curve_cryptography.hpp`

#### `class MpiECPoint : public GroupElement<MpiECPoint>`

Elliptic curve point backed by mbedtls_ecp_point and mbedtls_mpi.
Uses the ESP32's hardware MPI accelerator for big integer arithmetic.
Curve is NIST P-256 (secp256r1) — 256-bit prime field, 128-bit security.
Implements the Rule of Three: destructor, copy constructor, and copy
assignment operator are all defined because mbedtls structs own heap
memory that must be explicitly freed and deep-copied.
NOTE: mbedTLS 3.x marks struct fields private via MBEDTLS_PRIVATE().
If compilation fails on field access, wrap field names with that macro
e.g. grp.MBEDTLS_PRIVATE(N), point.MBEDTLS_PRIVATE(Y).

#### `MpiECPoint combine(const MpiECPoint& other) const`

Returns this + other (point addition) via R = 1*P + 1*Q.
Uses mbedtls_ecp_muladd which requires no RNG.

#### `MpiECPoint invert() const`

Returns -P by computing (N-1)*P where N is the group order.
(N-1)*P = -P because N*P = identity in the group.

#### `class ECDHMpiScheme : public CryptographyScheme<ECDHMpiScheme, MpiECPoint, long>`

ECDH scheme over NIST P-256 using mbedTLS.
Private keys are native long (32-bit), converted internally to mbedtls_mpi.
For full 256-bit private keys, ScalarType should be changed to mbedtls_mpi.
generator field from base class is unused here. Each operation creates
a fresh MpiECPoint (which loads the group via mbedtls_ecp_group_load) and
uses result.grp.G directly — avoiding the silent failure of copying grp.G
after group_load in older code.

#### `MpiECPoint compute_public_value(long private_key)`

Computes private_key * G.

#### `MpiECPoint compute_shared_secret(const MpiECPoint& other_public, long private_key)`

Computes private_key * other_public = private_key * other_private * G.

#### `MpiECPoint create_message(long value)`

Encodes message scalar as message * G.

#### `MpiECPoint scalar_mul_G(long k) const`

Multiplies the group generator G by scalar k. Uses result.grp.G directly.

### `cryptography/nrf_elliptic_curve_cryptography.hpp`

#### `// Raw 32-byte message or shared secret buffer. struct NrfBuffer`

ECDH key exchange and XOR encryption using the nRF52840 CryptoCell-310.
The nRFCrypto API wraps Nordic's CC310 hardware accelerator and provides:
- genKeyPair    — hardware-generated P-256 key pair
- SVDP_DH       — ECDH: computes x-coordinate of k*P as 32 raw bytes
Public key serialisation note:
CRYS_ECPKI_ExportPublKey outputs a valid 04||X||Y uncompressed point, but
CRYS_ECPKI_BuildPublKey[PartlyCheck] fails to re-import it on this build of
the precompiled CC310 library (mode-1 partial check returns an error; mode-0
size-only check leaves the internal buffer uninitialised causing SVDP_DH to
hang). Cross-device key exchange would require fixing this import path.
For this demo both parties run on the same device, so computeSharedSecret()
takes the peer's nRFCrypto_ECC_PublicKey object directly, bypassing the
broken serialisation round-trip. getPublicKey() still serialises to a 65-byte
buffer for logging and interoperability reference.
Because CC310 does not expose raw point arithmetic, encryption and decryption
are implemented as XOR with the 32-byte shared secret (a one-time pad), which
is valid for messages up to 32 bytes.

#### `class NrfECDHScheme`

ECDH scheme over P-256 backed by CryptoCell-310.
Each party calls begin() to generate a key pair, exchanges public keys via
getPublicKey() (serialised for logging), then calls computeSharedSecret()
with the peer's key object. The resulting 32-byte shared secret is used
directly as a XOR key.

#### `NrfBuffer encrypt(const NrfBuffer& message, const NrfBuffer& shared_secret)`

Encrypts a message by XOR-ing it with the shared secret.
XOR is its own inverse, so decrypt() is the same operation.

### `cryptography/nrf_signing_scheme.hpp`

#### `class NrfSigningScheme : public SigningScheme`

ECDSA P-256 signing using the nRF52840 CryptoCell-310.
sign()   — calls CRYS_ECDSA_Sign with SHA-256 hashing, produces 64-byte (r||s) signature.
verify() — rebuilds the public key from raw bytes via CRYS_ECPKI_BuildPublKey, then
calls CRYS_ECDSA_Verify.
Note: CRYS_ECPKI_BuildPublKey (mode 1 partial check) may fail on this CC310 build
— the same limitation documented in nrf_elliptic_curve_cryptography.hpp for ECDH.
Verification is architecturally correct; the import path may need a workaround once
cross-device key exchange is tested.

#### `bool begin() override`

Initialises nRFCrypto and generates a fresh P-256 key pair.

#### `bool sign(const uint8_t* data, uint32_t len, uint8_t* sig_out, uint32_t& sig_size) override`

Signs data with the local private key via CRYS_ECDSA_Sign (SHA-256); produces a 64-byte r||s signature.

#### `bool verify(const uint8_t* data, uint32_t len, const uint8_t* sig, uint32_t sig_size, const uint8_t* pub_key_bytes) override`

Verifies a 64-byte r||s signature over data using the peer's raw 65-byte uncompressed public key.

#### `void getPublicKey(uint8_t* out) override`

Exports the local public key as a 65-byte uncompressed P-256 point (04 || X || Y).

#### `bool computeSharedSecret(const uint8_t* peer_pub_raw, uint8_t* out, uint8_t out_len) override`

ECDH using the signing key pair: derives the shared secret from the peer's raw 65-byte public key.

#### `void setSharedKey(const uint8_t* key_bytes) override`

Copies the first 16 bytes of the ECDH shared secret into the AES-128 key slot.

#### `bool encrypt(const uint8_t* in, uint8_t* out, uint8_t len) override`

Encrypts len bytes using AES-128-CTR.

#### `bool decrypt(const uint8_t* in, uint8_t* out, uint8_t len) override`

Decrypts len bytes using AES-128-CTR (symmetric with encrypt).

### `cryptography/signing_scheme.hpp`

#### `class SigningScheme`

Abstract interface for a signing scheme.
Typed as a pointer in LifeCycle to allow swapping implementations.

#### `virtual bool sign(const uint8_t* data, uint32_t len, uint8_t* sig_out, uint32_t& sig_size) = 0`

Signs `data` with the local private key. Writes signature into `sig_out`.

#### `virtual bool verify(const uint8_t* data, uint32_t len, const uint8_t* sig, uint32_t sig_size, const uint8_t* pub_key_bytes) = 0`

Verifies `sig` over `data` using the provided raw public key bytes
(65-byte uncompressed P-256: 04 || X || Y).

#### `virtual void getPublicKey(uint8_t* out) = 0`

Exports the local public key as a 65-byte uncompressed point.

#### `virtual bool computeSharedSecret(const uint8_t* peer_pub_raw, uint8_t* out, uint8_t out_len) = 0`

ECDH using the same key pair: computes the shared secret from the peer's raw 65-byte public key.

#### `virtual void setSharedKey(const uint8_t* key_bytes) = 0`

Store the first 16 bytes of the ECDH shared secret for AES-128 symmetric operations.

#### `virtual bool encrypt(const uint8_t* in, uint8_t* out, uint8_t len) = 0`

AES-128-ECB encrypt — len must be a multiple of 16.

#### `virtual bool decrypt(const uint8_t* in, uint8_t* out, uint8_t len) = 0`

AES-128-ECB decrypt — len must be a multiple of 16.

---

## desktop

---

## lifecycle

### `lifecycle/data_packet.hpp`

#### `struct DataPacket`

16-byte encrypted data frame. Byte 0 is flags (IS_LAST), bytes 1–15 are payload.

#### `struct ResponsePacket`

4-byte ACK/NACK response frame. The type byte is either ACK (0xAC) or NACK (0x4E).

### `lifecycle/factory.hpp`

#### `enum class OperationMode`

Selects whether the device acts as a transmitter (beacon emitter) or receiver.

#### `LifeCycle* getLifeCycle(OperationMode mode)`

Allocates and returns the LifeCycle instance for the given mode. Caller owns the pointer.

### `lifecycle/frequency_hopper.hpp`

#### `class FHop`

xorshift32 PRNG-based frequency hopper producing channels in [FHOP_CHANNEL_MIN, FHOP_CHANNEL_MAX].

#### `FHop() : _state(1)`

Seeds with 1 (xorshift32 requires a non-zero state).

#### `explicit FHop(uint32_t seed) : _state(seed == 0 ? 1 : seed)`

Seeds with the given value; substitutes 1 if seed is zero.

#### `static FHop fromSecret(const uint8_t* shared_secret)`

Derives the seed from the first 4 bytes of a 32-byte ECDH shared secret.

#### `uint8_t next()`

Advances the xorshift32 state and returns the next channel in [FHOP_CHANNEL_MIN, FHOP_CHANNEL_MAX].

### `lifecycle/handshake_packet.hpp`

#### `struct HandshakePacket`

Wire representation of a discovery handshake: device_id + public_key + ECDSA signature.

#### `struct Content`

Plain fields that are signed: device_id (4 bytes) and public_key (65-byte uncompressed P-256 point).

#### `void toBytes(uint8_t* buf) const`

Serializes the packet into a flat byte buffer of at least SIZE bytes.

#### `static HandshakePacket build(const uint8_t device_id[DEVICE_ID_SIZE], SigningScheme* crypto)`

Constructs and signs a new packet using the given device_id and crypto context.

#### `static HandshakePacket fromBytes(const uint8_t* buf)`

Deserializes a packet from a flat byte buffer of at least SIZE bytes.

#### `void print() const`

Logs device_id, public_key, and signature as hex to the serial output.

### `lifecycle/lifecycle.hpp`

#### `class LifeCycle`

Base class for transmitter and receiver lifecycles.
Owns the crypto context, frequency hopper, and all NRF_RADIO helpers.

#### `LifeCycle()`

Initializes and starts the NrfSigningScheme (key generation).

#### `virtual void startLifeCycle() = 0`

Entry point — runs the full discovery + data transfer sequence.

#### `virtual ~LifeCycle()`

Frees the crypto context.

#### `static void logHex(const uint8_t* buf, uint8_t len)`

Prints a byte array as space-separated hex to the serial log.

#### `static void radioInit(uint8_t channel, uint8_t packet_len = HandshakePacket::SIZE)`

Powers and configures NRF_RADIO for the given channel and static packet length.

#### `static void txPacket(uint8_t* buf)`

Transmits one packet from buf; blocks until the radio disables.

#### `static bool rxPacket(uint8_t* buf, uint32_t timeout_ms)`

Receives one packet into buf; returns true on CRC-OK within timeout_ms. Pass 0xFFFFFFFF to wait indefinitely.

#### `static bool rssiOk()`

Returns true if the last received packet's RSSI meets the data threshold.

#### `static bool rssiHandshakeOk()`

Returns true if the last received packet's RSSI meets the handshake threshold.

### `lifecycle/receiver_lifecycle.hpp`

#### `class ReceiverLifeCycle : public LifeCycle`

Receiver role — listens for a beacon, completes the handshake, then receives encrypted data.

#### `void startLifeCycle() override`

Runs discovery then derives the shared key and starts receiving.

#### `void startReceiving()`

Frequency-hopping receive loop.
Times out with a NACK if no data arrives within RX_TIMEOUT_MS.
After an ACK is sent, uses a time-division loop to re-send the ACK on ch N while
listening on ch N+1, recovering from a lost ACK without clock synchronisation.

#### `HandshakePacket startDiscovering()`

Waits for a beacon with sufficient RSSI, responds with own handshake packet, returns the peer's packet.

### `lifecycle/transmitter_lifecycle.hpp`

#### `class TransmitterLifeCycle : public LifeCycle`

Transmitter role — broadcasts beacons until paired, then sends encrypted data over frequency-hopping channels.

#### `void startLifeCycle() override`

Runs discovery then derives the shared key and starts transmitting.

#### `void startTransmitting()`

Frequency-hopping transmit loop; retransmits the current packet on NACK and advances on ACK.

#### `HandshakePacket startDiscoverable()`

Broadcasts beacon until a valid signed handshake response is received; returns the peer's packet.

---

## math


Standalone math utilities used by the cryptography module. Lives in the `math_utils` namespace to avoid collisions with the C standard `<math.h>`.

## Contents

**`modular.hpp`** — `mod_pow(base, exp, mod)`: fast modular exponentiation via repeated squaring in O(log exp). Used by the discrete logarithm scheme to compute modular inverses via Fermat's little theorem (`a⁻¹ = a^(p-2) mod p`).

All functions are `inline` to satisfy the One Definition Rule when the header is included across multiple translation units.

### `math/modular.hpp`

#### `inline long mod_pow(long base, long exp, long mod)`

Fast modular exponentiation via repeated squaring.
Computes base^exp mod mod in O(log exp) multiplications.
Requires mod > 1 and exp >= 0.
NOTE: uses native long. On ESP32 this is 32-bit, so intermediate
products (base * base) can overflow for large values. For
cryptographic use replace long with an arbitrary-precision type.

