#pragma once

#include <mbedtls/ecp.h>
#include <mbedtls/bignum.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#include "abstract_cryptography/cryptography_scheme.hpp"
#include "abstract_cryptography/group_element.hpp"

/**
 * Module-level RNG shared across all MpiECPoint and ECDHMpiScheme operations.
 *
 * mbedTLS 3.x requires a non-NULL RNG for mbedtls_ecp_mul (enforced by
 * internal checks; passing NULL returns MBEDTLS_ERR_ECP_BAD_INPUT_DATA and
 * leaves the result point at the identity element).
 *
 * A single CTR-DRBG instance seeded from the hardware entropy source is
 * initialized on first use and reused for all subsequent ecp_mul calls.
 */
namespace mpi_ecc_rng {
    static mbedtls_entropy_context  entropy;
    static mbedtls_ctr_drbg_context ctr_drbg;
    static bool initialized = false;

    inline void ensure_init() {
        if (initialized) return;
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                               reinterpret_cast<const unsigned char*>("mpi_ecc"), 7);
        initialized = true;
    }
} // namespace mpi_ecc_rng


/**
 * Elliptic curve point backed by mbedtls_ecp_point and mbedtls_mpi.
 *
 * Uses the ESP32's hardware MPI accelerator for big integer arithmetic.
 * Curve is NIST P-256 (secp256r1) — 256-bit prime field, 128-bit security.
 *
 * Implements the Rule of Three: destructor, copy constructor, and copy
 * assignment operator are all defined because mbedtls structs own heap
 * memory that must be explicitly freed and deep-copied.
 *
 * NOTE: mbedTLS 3.x marks struct fields private via MBEDTLS_PRIVATE().
 * If compilation fails on field access, wrap field names with that macro
 * e.g. grp.MBEDTLS_PRIVATE(N), point.MBEDTLS_PRIVATE(Y).
 */
class MpiECPoint : public GroupElement<MpiECPoint> {
public:
    mbedtls_ecp_group grp;
    mbedtls_ecp_point point;

    MpiECPoint() {
        mbedtls_ecp_group_init(&grp);
        mbedtls_ecp_point_init(&point);
        mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
    }

    ~MpiECPoint() {
        mbedtls_ecp_point_free(&point);
        mbedtls_ecp_group_free(&grp);
    }

    MpiECPoint(const MpiECPoint& other) {
        mbedtls_ecp_group_init(&grp);
        mbedtls_ecp_point_init(&point);
        mbedtls_ecp_group_copy(&grp, &other.grp);
        mbedtls_ecp_copy(&point, &other.point);
    }

    MpiECPoint& operator=(const MpiECPoint& other) {
        if (this != &other) {
            mbedtls_ecp_point_free(&point);
            mbedtls_ecp_group_free(&grp);
            mbedtls_ecp_group_init(&grp);
            mbedtls_ecp_point_init(&point);
            mbedtls_ecp_group_copy(&grp, &other.grp);
            mbedtls_ecp_copy(&point, &other.point);
        }
        return *this;
    }

    /**
     * Returns this + other (point addition) via R = 1*P + 1*Q.
     * Uses mbedtls_ecp_muladd which requires no RNG.
     */
    MpiECPoint combine(const MpiECPoint& other) const {
        MpiECPoint result;
        mbedtls_mpi one;
        mbedtls_mpi_init(&one);
        mbedtls_mpi_lset(&one, 1);
        mbedtls_ecp_muladd(&result.grp, &result.point, &one, &point, &one, &other.point);
        mbedtls_mpi_free(&one);
        return result;
    }

    /**
     * Returns -P by computing (N-1)*P where N is the group order.
     * (N-1)*P = -P because N*P = identity in the group.
     */
    MpiECPoint invert() const {
        mpi_ecc_rng::ensure_init();
        MpiECPoint result;
        mbedtls_mpi scalar;
        mbedtls_mpi_init(&scalar);
        mbedtls_mpi_sub_int(&scalar, &grp.N, 1);
        mbedtls_ecp_mul(&result.grp, &result.point, &scalar, &point,
                        mbedtls_ctr_drbg_random, &mpi_ecc_rng::ctr_drbg);
        mbedtls_mpi_free(&scalar);
        return result;
    }
};


/**
 * ECDH scheme over NIST P-256 using mbedTLS.
 *
 * Private keys are native long (32-bit), converted internally to mbedtls_mpi.
 * For full 256-bit private keys, ScalarType should be changed to mbedtls_mpi.
 *
 * generator field from base class is unused here. Each operation creates
 * a fresh MpiECPoint (which loads the group via mbedtls_ecp_group_load) and
 * uses result.grp.G directly — avoiding the silent failure of copying grp.G
 * after group_load in older code.
 */
class ECDHMpiScheme : public CryptographyScheme<ECDHMpiScheme, MpiECPoint, long> {
public:
    /** Computes private_key * G. */
    MpiECPoint compute_public_value(long private_key) {
        return scalar_mul_G(private_key);
    }

    /** Computes private_key * other_public = private_key * other_private * G. */
    MpiECPoint compute_shared_secret(const MpiECPoint& other_public, long private_key) {
        return scalar_mul(other_public, private_key);
    }

    /** Encodes message scalar as message * G. */
    MpiECPoint create_message(long value) {
        return scalar_mul_G(value);
    }

private:
    /** Multiplies the group generator G by scalar k. Uses result.grp.G directly. */
    MpiECPoint scalar_mul_G(long k) const {
        mpi_ecc_rng::ensure_init();
        MpiECPoint result;
        mbedtls_mpi scalar;
        mbedtls_mpi_init(&scalar);
        mbedtls_mpi_lset(&scalar, k);
        mbedtls_ecp_mul(&result.grp, &result.point, &scalar, &result.grp.G,
                        mbedtls_ctr_drbg_random, &mpi_ecc_rng::ctr_drbg);
        mbedtls_mpi_free(&scalar);
        return result;
    }

    MpiECPoint scalar_mul(const MpiECPoint& P, long k) const {
        mpi_ecc_rng::ensure_init();
        MpiECPoint result;
        mbedtls_mpi scalar;
        mbedtls_mpi_init(&scalar);
        mbedtls_mpi_lset(&scalar, k);
        mbedtls_ecp_mul(&result.grp, &result.point, &scalar, &P.point,
                        mbedtls_ctr_drbg_random, &mpi_ecc_rng::ctr_drbg);
        mbedtls_mpi_free(&scalar);
        return result;
    }
};
