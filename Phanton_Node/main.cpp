#include "bits/stdc++.h"
#include "cryptography/discrete_log_cryptography.hpp"
using namespace std;

int main() {
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

    cout << "Alice public key : " << alice_pub.value  << '\n';
    cout << "Bob   public key : " << bob_pub.value    << '\n';
    cout << "Shared (Alice)   : " << shared_alice.value << '\n';
    cout << "Shared (Bob)     : " << shared_bob.value   << '\n';

    // Encryption / decryption
    IntElement message = scheme.create_message(5);
    cout << "\nOriginal message : " << message.value << '\n';

    IntElement ciphertext = scheme.encrypt_message(message, shared_alice);
    cout << "Ciphertext       : " << ciphertext.value << '\n';

    IntElement decrypted = scheme.decrypt_message(ciphertext, shared_bob);
    cout << "Decrypted        : " << decrypted.value << '\n';

    return 0;
}
