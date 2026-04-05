#include <Arduino.h>

#ifdef ARDUINO_NRF52_ADAFRUIT
  #include "cryptography/nrf_elliptic_curve_cryptography.hpp"
#else
  #include <mbedtls/ecp.h>
  #include "cryptography/mpi_elliptic_curve_cryptography.hpp"
#endif

/**
 * DEBUG, not meant for production.
 *
 * If the program starts right away,
 * the serial monitor connects after the setup code runs.
 * This gives enough time for not missing the serial output.
 */
void serial_monitor_delay() {
#ifdef ARDUINO_NRF52_ADAFRUIT
    int delay_seconds = 5;
#else
    int delay_seconds = 10;
#endif
    for (int i = 0; i < delay_seconds; i++) {
        Serial.print("Timeout : ");
        Serial.print(delay_seconds - i);
        Serial.println("s");
        delay(1000);
    }
}

// ---------------------------------------------------------------------------
// nRF52840 — CryptoCell-310 via Adafruit_nRFCrypto
// ---------------------------------------------------------------------------
#ifdef ARDUINO_NRF52_ADAFRUIT

void print_buf(const uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
    }
}

void setup() {
    Serial.begin(115200);
    serial_monitor_delay();

    // static: keys are ~1700 bytes each; two instances would overflow the 4096-byte FreeRTOS task stack
    Serial.println("init alice...");
    static NrfECDHScheme alice;
    if (!alice.begin()) { Serial.println("alice.begin() FAILED"); while(1); }

    Serial.println("init bob...");
    static NrfECDHScheme bob;
    if (!bob.begin()) { Serial.println("bob.begin() FAILED"); while(1); }

    Serial.println("computing public keys...");
    NrfPublicKey alice_pub = alice.getPublicKey();
    NrfPublicKey bob_pub   = bob.getPublicKey();

    Serial.println("computing shared secrets...");
    NrfBuffer shared_alice = alice.computeSharedSecret(bob.publicKeyObj());
    NrfBuffer shared_bob   = bob.computeSharedSecret(alice.publicKeyObj());

    Serial.print("Alice public key : "); print_buf(alice_pub.data, NrfPublicKey::SIZE); Serial.println();
    Serial.print("Bob   public key : "); print_buf(bob_pub.data,   NrfPublicKey::SIZE); Serial.println();
    Serial.print("Shared (Alice)   : "); print_buf(shared_alice.data, NrfBuffer::SIZE); Serial.println();
    Serial.print("Shared (Bob)     : "); print_buf(shared_bob.data,   NrfBuffer::SIZE); Serial.println();

    NrfBuffer message = {};
    message.data[0] = 0xDE; message.data[1] = 0xAD;
    message.data[2] = 0xBE; message.data[3] = 0xEF;

    NrfBuffer ciphertext = alice.encrypt(message, shared_alice);
    NrfBuffer decrypted  = bob.decrypt(ciphertext, shared_bob);

    Serial.print("\nMessage    : "); print_buf(message.data,    NrfBuffer::SIZE); Serial.println();
    Serial.print("Ciphertext : "); print_buf(ciphertext.data, NrfBuffer::SIZE); Serial.println();
    Serial.print("Decrypted  : "); print_buf(decrypted.data,  NrfBuffer::SIZE); Serial.println();
    Serial.print("Match      : ");
    Serial.println(memcmp(message.data, decrypted.data, NrfBuffer::SIZE) == 0 ? "YES" : "NO");
}

// ---------------------------------------------------------------------------
// ESP32 — P-256 via mbedTLS MPI
// ---------------------------------------------------------------------------
#else

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

    ECDHMpiScheme scheme;

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

    MpiECPoint message    = scheme.create_message(4);
    MpiECPoint ciphertext = scheme.encrypt_message(message, shared_alice);
    MpiECPoint decrypted  = scheme.decrypt_message(ciphertext, shared_bob);

    Serial.print("\nMessage    : "); print_point(message);    Serial.println();
    Serial.print("Ciphertext : "); print_point(ciphertext); Serial.println();
    Serial.print("Decrypted  : "); print_point(decrypted);  Serial.println();
    Serial.print("Match      : "); Serial.println(points_equal(message, decrypted) ? "YES" : "NO");
}

#endif

void loop() { }
