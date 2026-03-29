#pragma once

/**
 * Base for a public-key cryptography scheme built on a group, using CRTP.
 *
 * Models the Diffie-Hellman family: a private key is a scalar, the public
 * key is generator^private_key, and a shared secret is derived by applying
 * the same operation to the other party's public key. Encryption and
 * decryption mask/unmask a message via the group operation.
 *
 * No virtual methods — all dispatch is resolved at compile time.
 * encrypt_message and decrypt_message are provided here as shared algorithms;
 * compute_public_value, compute_shared_secret, and create_message must be
 * implemented by Derived.
 *
 * @tparam Derived       The concrete scheme subclass (CRTP pattern).
 * @tparam ElementType   A GroupElement subclass representing a group element.
 * @tparam ScalarType    The type used for private keys and raw message values.
 */
template <typename Derived, typename ElementType, typename ScalarType>
class CryptographyScheme {
protected:
    ElementType generator;

public:
    /** Encrypts message M by combining it with the shared secret. */
    ElementType encrypt_message(const ElementType& M, const ElementType& shared_secret) {
        return M.combine(shared_secret);
    }

    /** Decrypts ciphertext C by combining it with the inverse of the shared secret. */
    ElementType decrypt_message(const ElementType& C, const ElementType& shared_secret) {
        return C.combine(shared_secret.invert());
    }
};
