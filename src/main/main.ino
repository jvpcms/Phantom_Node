#include <Arduino.h>
#include <memory>
#include "cryptography/discrete_log_cryptography.hpp"

void setup() {
    Serial.begin(115200);
    delay(10000); // [DEBUG] Give time for serial monitor to connect

    // Public parameters: generator g=2, prime p=23
    DLPScheme scheme(2, 23);

    // Key exchange (Diffie-Hellman)
    long alice_priv = 6;
    long bob_priv   = 5;

    IntElement alice_pub = scheme.compute_public_value(alice_priv); // g^a mod p
    IntElement bob_pub   = scheme.compute_public_value(bob_priv);   // g^b mod p

    // Each side computes the shared secret: (other's public)^(own private) mod p = g^(ab) mod p
    IntElement shared_alice = scheme.compute_shared_secret(bob_pub,   alice_priv);
    IntElement shared_bob   = scheme.compute_shared_secret(alice_pub, bob_priv);

    Serial.print("Alice public key : "); Serial.println(alice_pub.value);
    Serial.print("Bob   public key : "); Serial.println(bob_pub.value);
    Serial.print("Shared (Alice)   : "); Serial.println(shared_alice.value);
    Serial.print("Shared (Bob)     : "); Serial.println(shared_bob.value);

    // Encryption / decryption
    IntElement message = scheme.create_message(5);
    Serial.print("\nOriginal message : "); Serial.println(message.value);

    IntElement ciphertext = scheme.encrypt_message(message, shared_alice);
    Serial.print("Ciphertext       : "); Serial.println(ciphertext.value);

    IntElement decrypted = scheme.decrypt_message(ciphertext, shared_bob);
    Serial.print("Decrypted        : "); Serial.println(decrypted.value);
}

void loop() { }
