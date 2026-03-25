#pragma once

/**
 * Abstract base for a public-key cryptography scheme built on a group.
 *
 * Models the Diffie-Hellman family of schemes: a private key is a scalar,
 * the public key is generator^private_key, and a shared secret is derived
 * by exponentiating the other party's public key with your private key.
 * Encryption and decryption mask/unmask a message using the shared secret
 * via the group operation.
 *
 * Domain parameters (the generator, the prime modulus, the curve) are
 * encapsulated inside ElementType and set at construction time.
 *
 * @tparam ElementType  A GroupElement subclass representing a group element.
 * @tparam ScalarType   The type used for private keys and raw message values.
 */
template <typename ElementType, typename ScalarType>
class CryptographyScheme {
protected:
    ElementType generator;

public:
    /** Computes the public key: generator^private_key. */
    virtual ElementType compute_public_value(ScalarType private_key) = 0;

    /** Computes the shared secret: other_public^private_key. */
    virtual ElementType compute_shared_secret(const ElementType& other_public, ScalarType private_key) = 0;

    /** Wraps a raw scalar into a group element with the scheme's parameters. */
    virtual ElementType create_message(ScalarType value) = 0;

    /** Encrypts message M by combining it with the shared secret (M * S mod p). */
    virtual ElementType encrypt_message(const ElementType& M, const ElementType& shared_secret) {
        return *M.combine(shared_secret);
    }

    /** Decrypts ciphertext C by combining it with the inverse of the shared secret. */
    virtual ElementType decrypt_message(const ElementType& C, const ElementType& shared_secret) {
        return *C.combine(*shared_secret.invert());
    }
};
