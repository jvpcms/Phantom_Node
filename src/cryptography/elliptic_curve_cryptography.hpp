#pragma once

#include "abstract_cryptography/cryptography_scheme.hpp"
#include "abstract_cryptography/group_element.hpp"
#include "math/modular.hpp"

/**
 * A point on a short Weierstrass elliptic curve: y² = x³ + ax + b (mod p).
 *
 * The group operation (combine) is point addition via the chord-and-tangent
 * rule. The identity element is the point at infinity. The inverse of a
 * point (x, y) is (x, -y mod p).
 *
 * Returns by value — no heap allocation, lives on the stack.
 *
 * TOY MODEL — NOT CRYPTOGRAPHICALLY SECURE:
 *   - Uses native long (32-bit on ESP32). Intermediate multiplications
 *     can overflow for primes larger than ~46340 (sqrt of 2^31).
 *   - Real ECC uses 256-bit primes. For production, replace long with
 *     mbedtls_mpi and use mbedtls_ecp_point for the group operation.
 */
class ECPoint : public GroupElement<ECPoint> {
public:
    long x, y;
    long a, b, p;
    bool is_infinity;

    /** Default constructor — uninitialized, required by CryptographyScheme. */
    ECPoint() : x(0), y(0), a(0), b(0), p(0), is_infinity(true) {}

    /** Constructs the identity element (point at infinity). */
    ECPoint(long a, long b, long p)
        : x(0), y(0), a(a), b(b), p(p), is_infinity(true) {}

    /** Constructs a regular point (x, y) on the curve. */
    ECPoint(long x, long y, long a, long b, long p)
        : x(x), y(y), a(a), b(b), p(p), is_infinity(false) {}

    /**
     * Returns this + other using the chord-and-tangent rule.
     *
     * Cases handled:
     *   - Either operand is infinity → return the other
     *   - P + (-P) → return infinity
     *   - P == Q   → point doubling (tangent line)
     *   - P != Q   → point addition (secant line)
     */
    ECPoint combine(const ECPoint& other) const {
        if (is_infinity) return other;
        if (other.is_infinity) return *this;

        // P + (-P) = infinity
        if (x == other.x && (y + other.y) % p == 0)
            return ECPoint(a, b, p);

        long m;
        if (x == other.x && y == other.y) {
            // Point doubling: m = (3x² + a) * (2y)^-1 mod p
            long num = (3 * (x % p) % p * (x % p) % p + a % p + p) % p;
            long den = math_utils::mod_pow(2 * y % p, p - 2, p);
            m = num * den % p;
        } else {
            // Point addition: m = (y2 - y1) * (x2 - x1)^-1 mod p
            long num = ((other.y - y) % p + p) % p;
            long den = math_utils::mod_pow((other.x - x + p) % p, p - 2, p);
            m = num * den % p;
        }

        long x3 = ((m * m % p) - x - other.x % p + 2 * p) % p;
        long y3 = ((m * ((x - x3 + p) % p)) % p - y % p + p) % p;
        return ECPoint(x3, y3, a, b, p);
    }

    /** Returns (x, -y mod p) — the reflection of this point over the x-axis. */
    ECPoint invert() const {
        if (is_infinity) return *this;
        return ECPoint(x, (-y % p + p) % p, a, b, p);
    }
};


/**
 * Toy ECDH scheme over a short Weierstrass curve.
 *
 * The private key is a scalar k. The public key is k*G (scalar multiplication
 * of the generator point G). The shared secret is k*(other's public key) = k*l*G.
 * Encryption masks a message point M by adding the shared secret: C = M + S.
 * Decryption recovers M via: M = C + (-S).
 *
 * Scalar multiplication uses the double-and-add algorithm: O(log k) point additions.
 *
 * TOY MODEL — NOT CRYPTOGRAPHICALLY SECURE:
 *   - Uses native long (32-bit on ESP32), limiting safe primes to ~46340.
 *   - Real ECDH uses 256-bit curves (e.g. P-256, secp256k1).
 *   - For production, use mbedTLS: mbedtls_ecdh_compute_shared().
 */
class ECDHScheme : public CryptographyScheme<ECDHScheme, ECPoint, long> {
public:
    ECDHScheme(long gx, long gy, long a, long b, long p) {
        generator = ECPoint(gx, gy, a, b, p);
    }

    /** Computes k*G via double-and-add. */
    ECPoint compute_public_value(long k) {
        return scalar_mul(generator, k);
    }

    /** Computes k*(other's public key) = k*l*G. */
    ECPoint compute_shared_secret(const ECPoint& other_public, long k) {
        return scalar_mul(other_public, k);
    }

    /** Embeds a scalar message as a curve point: value*G. */
    ECPoint create_message(long value) {
        return scalar_mul(generator, value);
    }

private:
    /** Double-and-add scalar multiplication: computes k*P in O(log k) steps. */
    ECPoint scalar_mul(ECPoint P, long k) const {
        ECPoint result(P.a, P.b, P.p);  // start with identity (point at infinity)
        while (k > 0) {
            if (k & 1) result = result.combine(P);
            P = P.combine(P);
            k >>= 1;
        }
        return result;
    }
};
