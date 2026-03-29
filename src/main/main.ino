#include <Arduino.h>
#include <mbedtls/ecp.h>
#include "cryptography/mpi_elliptic_curve_cryptography.hpp"

/**
 * DEBUG, not meant for production.
 *
 * If the program starts right away,
 * the serial monitor connects after the setup code runs.
 * This gives enough time for not missing the serial output.
 */
void serial_monitor_delay() {
    int delay_seconds = 10;
    for (int i = 0; i < delay_seconds; i++) {
        Serial.print("Timeout : ");
        Serial.print(delay_seconds - i);
        Serial.println("s");
        delay(1000);
    }
}

/** Prints a curve point as an uncompressed hex string over Serial. */
void print_point(const MpiECPoint& p) {
    unsigned char buf[65]; // 1 prefix byte + 32 bytes X + 32 bytes Y
    size_t len;
    mbedtls_ecp_point_write_binary(
        &p.grp, &p.point,
        MBEDTLS_ECP_PF_UNCOMPRESSED,
        &len, buf, sizeof(buf)
    );
    for (size_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
    }
}

/** Returns true if two points are equal by comparing their binary representations. */
bool points_equal(const MpiECPoint& a, const MpiECPoint& b) {
    unsigned char ba[65], bb[65];
    size_t la, lb;
    mbedtls_ecp_point_write_binary(&a.grp, &a.point, MBEDTLS_ECP_PF_UNCOMPRESSED, &la, ba, sizeof(ba));
    mbedtls_ecp_point_write_binary(&b.grp, &b.point, MBEDTLS_ECP_PF_UNCOMPRESSED, &lb, bb, sizeof(bb));
    if (la != lb) return false;
    return memcmp(ba, bb, la) == 0;
}

void setup() {
    Serial.begin(115200);
    serial_monitor_delay();

    // P-256 curve, generator G built into the group
    ECDHMpiScheme scheme;

    // Key exchange
    long alice_priv = 7;
    long bob_priv   = 11;

    MpiECPoint alice_pub    = scheme.compute_public_value(alice_priv);
    MpiECPoint bob_pub      = scheme.compute_public_value(bob_priv);
    MpiECPoint shared_alice = scheme.compute_shared_secret(bob_pub,   alice_priv);
    MpiECPoint shared_bob   = scheme.compute_shared_secret(alice_pub, bob_priv);

    Serial.print("Alice public key : "); print_point(alice_pub);    Serial.println();
    Serial.print("Bob   public key : "); print_point(bob_pub);      Serial.println();
    Serial.print("Shared (Alice)   : "); print_point(shared_alice); Serial.println();
    Serial.print("Shared (Bob)     : "); print_point(shared_bob);   Serial.println();

    // Encryption / decryption
    MpiECPoint message    = scheme.create_message(4);
    MpiECPoint ciphertext = scheme.encrypt_message(message, shared_alice);
    MpiECPoint decrypted  = scheme.decrypt_message(ciphertext, shared_bob);

    Serial.print("\nMessage    : "); print_point(message);    Serial.println();
    Serial.print("Ciphertext : "); print_point(ciphertext); Serial.println();
    Serial.print("Decrypted  : "); print_point(decrypted);  Serial.println();
    Serial.print("Match      : "); Serial.println(points_equal(message, decrypted) ? "YES" : "NO");
}

void loop() { }
