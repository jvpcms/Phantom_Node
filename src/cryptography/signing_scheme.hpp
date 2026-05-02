#pragma once

#include <stdint.h>

/**
 * Abstract interface for a signing scheme.
 * Typed as a pointer in LifeCycle to allow swapping implementations.
 */
class SigningScheme {
public:
    virtual bool begin() = 0;

    /** Signs `data` with the local private key. Writes signature into `sig_out`. */
    virtual bool sign(const uint8_t* data, uint32_t len,
                      uint8_t* sig_out, uint32_t& sig_size) = 0;

    /**
     * Verifies `sig` over `data` using the provided raw public key bytes
     * (65-byte uncompressed P-256: 04 || X || Y).
     */
    virtual bool verify(const uint8_t* data, uint32_t len,
                        const uint8_t* sig, uint32_t sig_size,
                        const uint8_t* pub_key_bytes) = 0;

    /** Exports the local public key as a 65-byte uncompressed point. */
    virtual void getPublicKey(uint8_t* out) = 0;

    /** ECDH using the same key pair: computes the shared secret from the peer's raw 65-byte public key. */
    virtual bool computeSharedSecret(const uint8_t* peer_pub_raw, uint8_t* out, uint8_t out_len) = 0;

    virtual ~SigningScheme() = default;
};
