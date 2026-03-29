#pragma once

namespace math_utils {

/**
 * Fast modular exponentiation via repeated squaring.
 *
 * Computes base^exp mod mod in O(log exp) multiplications.
 * Requires mod > 1 and exp >= 0.
 *
 * NOTE: uses native long. On ESP32 this is 32-bit, so intermediate
 * products (base * base) can overflow for large values. For
 * cryptographic use replace long with an arbitrary-precision type.
 */
inline long mod_pow(long base, long exp, long mod) {
    long result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) result = result * base % mod;
        base = base * base % mod;
        exp /= 2;
    }
    return result;
}

} // namespace math_utils
