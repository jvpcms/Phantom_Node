#include <memory>

#include "abstract_cryptography/cryptography_scheme.hpp"
#include "abstract_cryptography/group_element.hpp"

class IntElement : public GroupElement<IntElement> {
public:
    long value;
    long mod;

    IntElement() : value(0), mod(0) {}
    IntElement(long v, long p) : value(v), mod(p) {}

    std::unique_ptr<IntElement> combine(const IntElement& other) const override {
        return std::make_unique<IntElement>((value * other.value) % mod, mod);
    }

    static long mod_pow(long base, long exp, long mod) {
        long result = 1;
        base %= mod;
        while (exp > 0) {
            if (exp % 2 == 1) result = result * base % mod;
            base = base * base % mod;
            exp /= 2;
        }
        return result;
    }

    std::unique_ptr<IntElement> invert() const override {
        return std::make_unique<IntElement>(mod_pow(value, mod - 2, mod), mod);
    }
};


class DLPScheme : public CryptographyScheme<IntElement, long> {
public:
    DLPScheme(long g, long p) {
        generator = IntElement(g, p);
    }

    IntElement compute_public_value(long private_key) override {
        return IntElement(IntElement::mod_pow(generator.value, private_key, generator.mod), generator.mod);
    }

    IntElement compute_shared_secret(const IntElement& other_public, long private_key) override {
        return IntElement(IntElement::mod_pow(other_public.value, private_key, generator.mod), generator.mod);
    }

    IntElement create_message(long value) override {
        return IntElement(value, generator.mod);
    }
};
