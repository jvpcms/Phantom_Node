#pragma once

#include <memory>

/**
 * Abstract base for an element of a mathematical group.
 *
 * A group is a set with a binary operation (combine) that is closed,
 * associative, has an identity element, and where every element has an
 * inverse. Concrete subclasses define the group — e.g. integers mod a
 * prime under multiplication, or points on an elliptic curve under addition.
 *
 * Uses CRTP so that combine() and invert() return unique_ptr<Derived>
 * instead of unique_ptr<GroupElement>, avoiding casts at the call site.
 *
 * @tparam Derived  The concrete subclass (CRTP pattern).
 */
template <typename Derived>
class GroupElement {
public:
    virtual ~GroupElement() = default;

    /** Returns a new element equal to this combined with other (group operation). */
    virtual std::unique_ptr<Derived> combine(const Derived& other) const = 0;

    /** Returns a new element that is the group-theoretic inverse of this. */
    virtual std::unique_ptr<Derived> invert() const = 0;
};
