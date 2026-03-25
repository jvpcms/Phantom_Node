#include <memory>

#include "abstract_cryptography/cryptography_scheme.hpp"
#include "abstract_cryptography/group_element.hpp"

/**
 * A group element representing an integer modulo a prime, under multiplication.
 *
 * The group operation (combine) is modular multiplication: (a * b) mod p.
 * The inverse is computed via Fermat's Little Theorem: a^(p-2) mod p,
 * which is valid only when p is prime.
 *
 * NOTE: uses native long (32-bit on ESP32). This limits values and moduli
 * to ~2 billion, making this unsuitable for real cryptographic security.
 * For production use, replace with an arbitrary-precision type (e.g. mbedtls_mpi).
 */
class IntElement : public GroupElement<IntElement> {
public:
    long value;
    long mod;

    IntElement() : value(0), mod(0) {}
    IntElement(long v, long p) : value(v), mod(p) {}

    /** Returns (this * other) mod p. */
    std::unique_ptr<IntElement> combine(const IntElement& other) const override {
        return std::make_unique<IntElement>((value * other.value) % mod, mod);
    }

    /** Fast modular exponentiation via repeated squaring: base^exp mod mod. */
    static long mod_pow(long base, long exp, long mod) {
        long result = 1;
        base %= mod;
        while (exp > 0) {
            if (exp % 2 == 1) result = result * base % mod;
            base = base * base % mod;
            exp /= 2;
        }
        return result;
    }

    /** Returns the modular inverse via Fermat's Little Theorem: value^(p-2) mod p. */
    std::unique_ptr<IntElement> invert() const override {
        return std::make_unique<IntElement>(mod_pow(value, mod - 2, mod), mod);
    }
};


/**
 * Toy implementation of a Diffie-Hellman scheme over integers mod a prime.
 *
 * Implements the discrete logarithm problem (DLP) in the multiplicative
 * group Z_p*: the public key is g^x mod p, and the shared secret is
 * (other's public key)^x mod p = g^(xy) mod p.
 *
 * TOY MODEL — NOT CRYPTOGRAPHICALLY SECURE:
 *   - Uses native long (32-bit on ESP32), limiting the prime p to ~2 billion.
 *   - Real DLP security requires primes of at least 2048 bits.
 *   - Intended for educational purposes and testing the scheme abstraction.
 *   - For production, use DLPMpiScheme (mbedtls_mpi) or an elliptic curve scheme.
 */
class DLPScheme : public CryptographyScheme<IntElement, long> {
public:
    DLPScheme(long g, long p) {
        generator = IntElement(g, p);
    }

    /** Computes g^private_key mod p. */
    IntElement compute_public_value(long private_key) override {
        return IntElement(IntElement::mod_pow(generator.value, private_key, generator.mod), generator.mod);
    }

    /** Computes other_public^private_key mod p (= g^(ab) mod p). */
    IntElement compute_shared_secret(const IntElement& other_public, long private_key) override {
        return IntElement(IntElement::mod_pow(other_public.value, private_key, generator.mod), generator.mod);
    }

    /** Wraps a scalar message value into an IntElement with the scheme's modulus. */
    IntElement create_message(long value) override {
        return IntElement(value, generator.mod);
    }
};
