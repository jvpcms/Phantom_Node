#pragma once

#include <Adafruit_nRFCrypto.h>
#include "nrf_cc310/include/crys_ecpki_build.h"
#include "nrf_cc310/include/crys_ecpki_domain.h"
#include "nrf_cc310/include/crys_ecpki_dh.h"
#include "logger.hpp"

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

    void print() const {
        for (uint8_t i = 0; i < SIZE; i++) {
            if (this->data[i] < 0x10) Log::print("0");
            Log::print(this->data[i], HEX);
            Log::print(" ");
        }
        Log::println();
    }
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
        nRFCrypto.begin();
        if (!this->_ecc.begin())                                       return false;
        if (!this->_private_key.begin(CRYS_ECPKI_DomainID_secp256r1)) return false;
        if (!this->_public_key.begin(CRYS_ECPKI_DomainID_secp256r1))  return false;
        return nRFCrypto_ECC::genKeyPair(this->_private_key, this->_public_key);
    }

    void end() {
        this->_private_key.end();
        this->_public_key.end();
        this->_ecc.end();
    }

    NrfPublicKey getPublicKey() {
        NrfPublicKey pk;
        this->_public_key.toRaw(pk.data, NrfPublicKey::SIZE);
        return pk;
    }

    NrfBuffer computeSharedSecret(nRFCrypto_ECC_PublicKey& peer_pub) {
        NrfBuffer secret;
        nRFCrypto_ECC::SVDP_DH(this->_private_key, peer_pub, secret.data, NrfBuffer::SIZE);
        return secret;
    }

    NrfBuffer computeSharedSecret(const uint8_t* peer_pub_raw) {
        NrfBuffer secret;

        const CRYS_ECPKI_Domain_t* domain = CRYS_ECPKI_GetEcDomain(CRYS_ECPKI_DomainID_secp256r1);
        CRYS_ECPKI_UserPublKey_t pub_key;
        if (CRYS_ECPKI_BuildPublKey(domain, const_cast<uint8_t*>(peer_pub_raw), NrfPublicKey::SIZE, &pub_key) != CRYS_OK)
            return secret;

        uint8_t priv_raw[32];
        this->_private_key.toRaw(priv_raw, sizeof(priv_raw));
        CRYS_ECPKI_UserPrivKey_t priv_key;
        CRYSError_t err = CRYS_ECPKI_BuildPrivKey(
            this->_private_key.getDomain(), priv_raw, sizeof(priv_raw), &priv_key
        );
        memset(priv_raw, 0, sizeof(priv_raw));
        if (err != CRYS_OK) return secret;

        CRYS_ECDH_TempData_t* temp = (CRYS_ECDH_TempData_t*) rtos_malloc(sizeof(CRYS_ECDH_TempData_t));
        if (!temp) return secret;
        uint32_t secret_size = NrfBuffer::SIZE;
        CRYS_ECDH_SVDP_DH(&pub_key, &priv_key, secret.data, &secret_size, temp);
        rtos_free(temp);

        return secret;
    }

    nRFCrypto_ECC_PublicKey& publicKeyObj() { return this->_public_key; }

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
        return this->encrypt(ciphertext, shared_secret);
    }

private:
    nRFCrypto_ECC            _ecc;
    nRFCrypto_ECC_PrivateKey _private_key;
    nRFCrypto_ECC_PublicKey  _public_key;
};
