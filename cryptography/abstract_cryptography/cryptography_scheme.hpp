#pragma once

template <typename ElementType, typename ScalarType>
class CryptographyScheme {
protected:
    ElementType generator;
    // Domain parameters (like the prime p or the curve) would be encapsulated in the ElementType logic

public:
    virtual ElementType compute_public_value(ScalarType private_key) = 0;
    virtual ElementType compute_shared_secret(const ElementType& other_public, ScalarType private_key) = 0;
    virtual ElementType create_message(ScalarType value) = 0;

    // The 'g' function: Masking a message M with a shared secret S
    virtual ElementType encrypt_message(const ElementType& M, const ElementType& shared_secret) {
        return *M.combine(shared_secret);
    }

    virtual ElementType decrypt_message(const ElementType& C, const ElementType& shared_secret) {
        return *C.combine(*shared_secret.invert());
    }
};
