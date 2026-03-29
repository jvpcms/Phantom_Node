#include "bits/stdc++.h"
#include "cryptography/elliptic_curve_cryptography.hpp"
using namespace std;

int main() {
    // Curve: y² = x³ + 2x + 3 (mod 97), generator G = (3, 6)
    // Verify: 6² = 36, 3³ + 2·3 + 3 = 36 ✓
    ECDHScheme scheme(3, 6, 2, 3, 97);

    // Key exchange
    long alice_priv = 7;
    long bob_priv   = 11;

    ECPoint alice_pub = scheme.compute_public_value(alice_priv); // 7*G
    ECPoint bob_pub   = scheme.compute_public_value(bob_priv);   // 11*G

    ECPoint shared_alice = scheme.compute_shared_secret(bob_pub,   alice_priv); // 7*(11*G)
    ECPoint shared_bob   = scheme.compute_shared_secret(alice_pub, bob_priv);   // 11*(7*G)

    cout << "Alice public key : (" << alice_pub.x << ", " << alice_pub.y << ")\n";
    cout << "Bob   public key : (" << bob_pub.x   << ", " << bob_pub.y   << ")\n";
    cout << "Shared (Alice)   : (" << shared_alice.x << ", " << shared_alice.y << ")\n";
    cout << "Shared (Bob)     : (" << shared_bob.x   << ", " << shared_bob.y   << ")\n";

    // Encryption / decryption
    ECPoint message = scheme.create_message(4); // 4*G
    cout << "\nOriginal message : (" << message.x << ", " << message.y << ")\n";

    ECPoint ciphertext = scheme.encrypt_message(message, shared_alice);
    cout << "Ciphertext       : (" << ciphertext.x << ", " << ciphertext.y << ")\n";

    ECPoint decrypted = scheme.decrypt_message(ciphertext, shared_bob);
    cout << "Decrypted        : (" << decrypted.x << ", " << decrypted.y << ")\n";

    return 0;
}
