#pragma once

/**
 * Marker base for a group element using CRTP.
 *
 * A group is a set with a binary operation (combine) that is closed,
 * associative, has an identity element, and where every element has an
 * inverse. Concrete subclasses define the group — e.g. integers mod a
 * prime under multiplication, or points on an elliptic curve under addition.
 *
 * No virtual methods — dispatch is resolved at compile time through the
 * template parameter, eliminating vtable overhead and enabling inlining.
 * No heap allocation — operations return by value and live on the stack.
 *
 * Derived must implement:
 *   Derived combine(const Derived& other) const;
 *   Derived invert() const;
 *
 * @tparam Derived  The concrete subclass (CRTP pattern).
 */
template <typename Derived>
class GroupElement {};
