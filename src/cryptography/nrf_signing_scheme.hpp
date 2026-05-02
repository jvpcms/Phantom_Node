#pragma once

#include <Adafruit_nRFCrypto.h>
#include "nrf_cc310/include/crys_ecpki_ecdsa.h"
#include "nrf_cc310/include/crys_ecpki_build.h"
#include "nrf_cc310/include/crys_ecpki_domain.h"
#include "nrf_cc310/include/crys_ecpki_dh.h"
#include "nrf_cc310/include/crys_rnd.h"
#include "nrf_cc310/include/ssi_aes.h"
#include "signing_scheme.hpp"

/**
 * ECDSA P-256 signing using the nRF52840 CryptoCell-310.
 *
 * sign()   — calls CRYS_ECDSA_Sign with SHA-256 hashing, produces 64-byte (r||s) signature.
 * verify() — rebuilds the public key from raw bytes via CRYS_ECPKI_BuildPublKey, then
 *             calls CRYS_ECDSA_Verify.
 *
 * Note: CRYS_ECPKI_BuildPublKey (mode 1 partial check) may fail on this CC310 build
 * — the same limitation documented in nrf_elliptic_curve_cryptography.hpp for ECDH.
 * Verification is architecturally correct; the import path may need a workaround once
 * cross-device key exchange is tested.
 */
class NrfSigningScheme : public SigningScheme {
public:
    bool begin() override {
        nRFCrypto.begin();
        if (!this->_ecc.begin())                                       return false;
        if (!this->_private_key.begin(CRYS_ECPKI_DomainID_secp256r1)) return false;
        if (!this->_public_key.begin(CRYS_ECPKI_DomainID_secp256r1))  return false;
        return nRFCrypto_ECC::genKeyPair(this->_private_key, this->_public_key);
    }

    bool sign(const uint8_t* data, uint32_t len,
              uint8_t* sig_out, uint32_t& sig_size) override {
        uint8_t priv_raw[32];
        this->_private_key.toRaw(priv_raw, sizeof(priv_raw));

        CRYS_ECPKI_UserPrivKey_t priv_key;
        CRYSError_t build_err = CRYS_ECPKI_BuildPrivKey(
            this->_private_key.getDomain(),
            priv_raw, sizeof(priv_raw),
            &priv_key
        );
        memset(priv_raw, 0, sizeof(priv_raw)); // clear key material from stack
        if (build_err != CRYS_OK) return false;

        auto* ctx = (CRYS_ECDSA_SignUserContext_t*)
                    rtos_malloc(sizeof(CRYS_ECDSA_SignUserContext_t));
        if (!ctx) return false;

        CRYSError_t err = CRYS_ECDSA_Sign(
            nRFCrypto.Random.getContext(),
            CRYS_RND_GenerateVector,
            ctx,
            &priv_key,
            CRYS_ECPKI_HASH_SHA256_mode,
            const_cast<uint8_t*>(data), len,
            sig_out, &sig_size
        );

        rtos_free(ctx);
        return err == CRYS_OK;
    }

    bool verify(const uint8_t* data, uint32_t len,
                const uint8_t* sig, uint32_t sig_size,
                const uint8_t* pub_key_bytes) override {
        // Rebuild public key from raw 65-byte uncompressed point
        CRYS_ECPKI_UserPublKey_t pub_key;
        const CRYS_ECPKI_Domain_t* domain =
            CRYS_ECPKI_GetEcDomain(CRYS_ECPKI_DomainID_secp256r1);

        CRYSError_t build_err = CRYS_ECPKI_BuildPublKey(
            domain,
            const_cast<uint8_t*>(pub_key_bytes), 65,
            &pub_key
        );
        if (build_err != CRYS_OK) return false;

        auto* ctx = (CRYS_ECDSA_VerifyUserContext_t*)
                    rtos_malloc(sizeof(CRYS_ECDSA_VerifyUserContext_t));
        if (!ctx) return false;

        CRYSError_t err = CRYS_ECDSA_Verify(
            ctx,
            &pub_key,
            CRYS_ECPKI_HASH_SHA256_mode,
            const_cast<uint8_t*>(sig), sig_size,
            const_cast<uint8_t*>(data), len
        );

        rtos_free(ctx);
        return err == CRYS_OK;
    }

    void getPublicKey(uint8_t* out) override {
        this->_public_key.toRaw(out, 65);
    }

    bool computeSharedSecret(const uint8_t* peer_pub_raw, uint8_t* out, uint8_t out_len) override {
        const CRYS_ECPKI_Domain_t* domain = CRYS_ECPKI_GetEcDomain(CRYS_ECPKI_DomainID_secp256r1);
        CRYS_ECPKI_UserPublKey_t pub_key;
        if (CRYS_ECPKI_BuildPublKey(domain, const_cast<uint8_t*>(peer_pub_raw), 65, &pub_key) != CRYS_OK)
            return false;

        uint8_t priv_raw[32];
        this->_private_key.toRaw(priv_raw, sizeof(priv_raw));
        CRYS_ECPKI_UserPrivKey_t priv_key;
        CRYSError_t err = CRYS_ECPKI_BuildPrivKey(
            this->_private_key.getDomain(), priv_raw, sizeof(priv_raw), &priv_key
        );
        memset(priv_raw, 0, sizeof(priv_raw));
        if (err != CRYS_OK) return false;

        CRYS_ECDH_TempData_t* temp = (CRYS_ECDH_TempData_t*) rtos_malloc(sizeof(CRYS_ECDH_TempData_t));
        if (!temp) return false;
        uint32_t secret_size = out_len;
        err = CRYS_ECDH_SVDP_DH(&pub_key, &priv_key, out, &secret_size, temp);
        rtos_free(temp);
        return err == CRYS_OK;
    }

    void setSharedKey(const uint8_t* key_bytes) override {
        memcpy(this->_aes_key, key_bytes, sizeof(this->_aes_key));
    }

    bool encrypt(const uint8_t* in, uint8_t* out, uint8_t len) override {
        return this->aesCtr(in, out, len);
    }

    bool decrypt(const uint8_t* in, uint8_t* out, uint8_t len) override {
        return this->aesCtr(in, out, len);
    }

private:
    nRFCrypto_ECC            _ecc;
    nRFCrypto_ECC_PrivateKey _private_key;
    nRFCrypto_ECC_PublicKey  _public_key;
    uint8_t                  _aes_key[16] = {};

    // AES-128-CTR using only the encrypt path (CC310 decrypt is non-functional).
    // Keystream = AES_encrypt(key, zero_nonce); both encrypt and decrypt are keystream XOR data.
    bool aesCtr(const uint8_t* in, uint8_t* out, uint8_t len) {
        uint8_t nonce[16]     = {};
        uint8_t keystream[16] = {};

        SaSiAesUserContext_t ctx;
        SaSiAesUserKeyData_t key_data = { this->_aes_key, sizeof(this->_aes_key) };

        SaSiError_t err;

        err = SaSi_AesInit(&ctx, SASI_AES_ENCRYPT, SASI_AES_MODE_ECB, SASI_AES_PADDING_NONE);
        if (err != SASI_OK) { Log::print("AesInit err="); Log::print(err, HEX); Log::println(); return false; }

        err = SaSi_AesSetKey(&ctx, SASI_AES_USER_KEY, &key_data, sizeof(key_data));
        if (err != SASI_OK) { Log::print("AesSetKey err="); Log::print(err, HEX); Log::println(); return false; }

        size_t out_size = sizeof(keystream);
        err = SaSi_AesFinish(&ctx, sizeof(nonce), nonce, sizeof(nonce), keystream, &out_size);
        if (err != SASI_OK) { Log::print("AesFinish err="); Log::print(err, HEX); Log::println(); return false; }

        for (uint8_t i = 0; i < len; i++) out[i] = in[i] ^ keystream[i];
        return true;
    }
};
