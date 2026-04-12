#pragma once

#include <Adafruit_nRFCrypto.h>
#include "nrf_cc310/include/crys_ecpki_ecdsa.h"
#include "nrf_cc310/include/crys_ecpki_build.h"
#include "nrf_cc310/include/crys_ecpki_domain.h"
#include "nrf_cc310/include/crys_rnd.h"
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
        if (!_ecc.begin())                                       return false;
        if (!_private_key.begin(CRYS_ECPKI_DomainID_secp256r1)) return false;
        if (!_public_key.begin(CRYS_ECPKI_DomainID_secp256r1))  return false;
        return nRFCrypto_ECC::genKeyPair(_private_key, _public_key);
    }

    bool sign(const uint8_t* data, uint32_t len,
              uint8_t* sig_out, uint32_t& sig_size) override {
        // _private_key._key is private — export to raw bytes and rebuild
        // a CRYS_ECPKI_UserPrivKey_t we can pass to CRYS_ECDSA_Sign.
        uint8_t priv_raw[32];
        _private_key.toRaw(priv_raw, sizeof(priv_raw));

        CRYS_ECPKI_UserPrivKey_t priv_key;
        CRYSError_t build_err = CRYS_ECPKI_BuildPrivKey(
            _private_key.getDomain(),
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
        _public_key.toRaw(out, 65);
    }

private:
    nRFCrypto_ECC            _ecc;
    nRFCrypto_ECC_PrivateKey _private_key;
    nRFCrypto_ECC_PublicKey  _public_key;
};
