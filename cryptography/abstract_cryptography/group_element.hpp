#pragma once

#include <memory>

template <typename Derived>
class GroupElement {
public:
    virtual ~GroupElement() = default;
    virtual std::unique_ptr<Derived> combine(const Derived& other) const = 0;
    virtual std::unique_ptr<Derived> invert() const = 0;
};
