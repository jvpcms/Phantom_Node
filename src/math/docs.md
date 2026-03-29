# math

Standalone math utilities used by the cryptography module. Lives in the `math_utils` namespace to avoid collisions with the C standard `<math.h>`.

## Contents

**`modular.hpp`** — `mod_pow(base, exp, mod)`: fast modular exponentiation via repeated squaring in O(log exp). Used by the discrete logarithm scheme to compute modular inverses via Fermat's little theorem (`a⁻¹ = a^(p-2) mod p`).

All functions are `inline` to satisfy the One Definition Rule when the header is included across multiple translation units.
