#pragma once

#include <Adafruit_nRFCrypto.h>

/**
 * ECDH key exchange and XOR encryption using the nRF52840 CryptoCell-310.
 *
 * The nRFCrypto API wraps Nordic's CC310 hardware accelerator and provides:
 *   - genKeyPair    — hardware-generated P-256 key pair
 *   - SVDP_DH       — ECDH: computes x-coordinate of k*P as 32 raw bytes
 *
 * Public key serialisation note:
 *   CRYS_ECPKI_ExportPublKey outputs a valid 04||X||Y uncompressed point, but
 *   CRYS_ECPKI_BuildPublKey[PartlyCheck] fails to re-import it on this build of
 *   the precompiled CC310 library (mode-1 partial check returns an error; mode-0
 *   size-only check leaves the internal buffer uninitialised causing SVDP_DH to
 *   hang). Cross-device key exchange would require fixing this import path.
 *
 *   For this demo both parties run on the same device, so computeSharedSecret()
 *   takes the peer's nRFCrypto_ECC_PublicKey object directly, bypassing the
 *   broken serialisation round-trip. getPublicKey() still serialises to a 65-byte
 *   buffer for logging and interoperability reference.
 *
 * Because CC310 does not expose raw point arithmetic, encryption and decryption
 * are implemented as XOR with the 32-byte shared secret (a one-time pad), which
 * is valid for messages up to 32 bytes.
 */

// Raw 32-byte message or shared secret buffer.
struct NrfBuffer {
    static constexpr uint8_t SIZE = 32;
    uint8_t data[SIZE] = {0};
};

// Uncompressed public key: 04 || X (32 bytes) || Y (32 bytes).
struct NrfPublicKey {
    static constexpr uint8_t SIZE = 65;
    uint8_t data[SIZE] = {0};
};

/**
 * ECDH scheme over P-256 backed by CryptoCell-310.
 *
 * Each party calls begin() to generate a key pair, exchanges public keys via
 * getPublicKey() (serialised for logging), then calls computeSharedSecret()
 * with the peer's key object. The resulting 32-byte shared secret is used
 * directly as a XOR key.
 */
class NrfECDHScheme {
public:
    bool begin() {
        nRFCrypto.begin(); // initialises the CryptoCell-310 hardware — must be called first
        if (!_ecc.begin())                                       return false;
        if (!_private_key.begin(CRYS_ECPKI_DomainID_secp256r1)) return false;
        if (!_public_key.begin(CRYS_ECPKI_DomainID_secp256r1))  return false;
        return nRFCrypto_ECC::genKeyPair(_private_key, _public_key);
    }

    void end() {
        _private_key.end();
        _public_key.end();
        _ecc.end();
    }

    /** Returns this party's public key serialised as a 65-byte uncompressed point. */
    NrfPublicKey getPublicKey() {
        NrfPublicKey pk;
        _public_key.toRaw(pk.data, NrfPublicKey::SIZE);
        return pk;
    }

    /**
     * Computes the shared secret from the peer's key object.
     * Returns the x-coordinate of private_key * peer_public as 32 bytes.
     */
    NrfBuffer computeSharedSecret(nRFCrypto_ECC_PublicKey& peer_pub) {
        NrfBuffer secret;
        nRFCrypto_ECC::SVDP_DH(_private_key, peer_pub, secret.data, NrfBuffer::SIZE);
        return secret;
    }

    /** Exposes the internal public key object so the peer can call computeSharedSecret(). */
    nRFCrypto_ECC_PublicKey& publicKeyObj() { return _public_key; }

    /**
     * Encrypts a message by XOR-ing it with the shared secret.
     * XOR is its own inverse, so decrypt() is the same operation.
     */
    NrfBuffer encrypt(const NrfBuffer& message, const NrfBuffer& shared_secret) {
        NrfBuffer result;
        for (uint8_t i = 0; i < NrfBuffer::SIZE; i++)
            result.data[i] = message.data[i] ^ shared_secret.data[i];
        return result;
    }

    NrfBuffer decrypt(const NrfBuffer& ciphertext, const NrfBuffer& shared_secret) {
        return encrypt(ciphertext, shared_secret);
    }

private:
    nRFCrypto_ECC            _ecc;
    nRFCrypto_ECC_PrivateKey _private_key;
    nRFCrypto_ECC_PublicKey  _public_key;
};
