#pragma once


#include <concepts>
#include <cstdint>
#include <climits>
#include <expected>
#include <stdexcept>


namespace misim
{

namespace freefunc
{

/**
 * Code begin.
*/

#define NODISCARD [[nodiscard]]

// Good to have some sanity check :)
static_assert(CHAR_BIT == 8u);

// using declarations
using std::unsigned_integral;
using std::integral;

// Check if a type is a specialization of some template type
template <class T, template <class...> class Template>
struct is_specialization : std::false_type {};

template <template <class...> class Template, class... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type {};


/**
 * The solution to solve problem with multiply invokes undefined behaviour
 * 
 * Explanation:
 *  Define A & B be unsigned integers, whose size less than signed int (normally 32bits)
 *  If the product of A * B overflows, A & B will be promoted to signed int and multiply.
 *  However, if the result overflows, signed int will invoke undefined behaviour.
 * 
 * Solution:
 *  Use template method to let unsigned integers only promote to unsigned int.
 *  After the operation is done, truncated the result to original size.
 * 
*/

template <std::unsigned_integral U>
struct MakePromoted
{
  static constexpr bool small{ sizeof(U) < sizeof(std::uint32_t) };

  using type = std::conditional_t<small, std::uint32_t, U>;
};

template <typename T>
using make_promoted_t = typename MakePromoted<T>::type;

template <typename T>
NODISCARD constexpr
make_promoted_t<T> promote(const T n) { return n; }


/**
 * Bit checking functions (observers)
*/

// Check if a bit is in range
template <unsigned_integral U>
NODISCARD constexpr
bool checkBitInRange(const integral auto pos)
{
  return pos >= 0 && pos < (sizeof(U) * CHAR_BIT);
}

// Check if a bit is in range
NODISCARD constexpr
bool checkBitInRange(const unsigned_integral auto n, const integral auto pos)
{
  return checkBitInRange<decltype(n)>(pos);
}


using std::expected;

template <typename T, typename E = std::domain_error>
using Expected = expected<T, E>;

// Test if a bit in an unsigned integral is set or no
template <unsigned_integral U>
NODISCARD constexpr
Expected<bool> testBit(const U n, const integral auto pos)
{
  if (!freefunc::checkBitInRange(n, pos)) {
    return std::unexpected(std::domain_error("pos out of range"));
  }

  return static_cast<bool>(n & (static_cast<U>(1) << pos));
}

// Test if an unsigned integral is 0b111..111
NODISCARD constexpr
Expected<bool> testBitAll(const unsigned_integral auto n)
{
  constexpr std::size_t n_size = sizeof(n) * CHAR_BIT - 1;

  if constexpr (sizeof(n) == sizeof(std::uintmax_t)) {
    if (const auto result = testBit(n, n_size); result && !*result) {
      return false;
    } else if (!result) {
      return result;
    }
  }

  const auto mask = (static_cast<std::uintmax_t>(1) << n_size) - 1;

  return (n & mask) == mask;
}

// Test if an unsigned integer has last n bits set to 1
NODISCARD constexpr
Expected<bool> testBitAll(const unsigned_integral auto n, const integral auto last_bits)
{
  if (!checkBitInRange(n, last_bits)) {
    return std::unexpected(std::domain_error("last_bits out of range."));
  }

  // If checking all the bits in front of the last bits, then delegate;
  if (!checkBitInRange(n, last_bits + 1)) {
    return testBitAll(n);
  }

  const auto mask = (static_cast<std::uintmax_t>(1) << (last_bits + 1)) - 1;

  return (n & mask) == mask;
}

// Test if any bit is set to 1
NODISCARD constexpr
Expected<bool> testBitAny(const unsigned_integral auto n)
{
  return static_cast<bool>(n);
}

// Test if any bit is set to 1
NODISCARD constexpr
Expected<bool> testBitAny(const unsigned_integral auto n, const integral auto last_bits)
{
  if (!checkBitInRange(n, last_bits)) {
    return std::unexpected(std::domain_error("last_bits out of range"));
  }

  if (!checkBitInRange(n, last_bits + 1)) {
    return testBitAny(n);
  }

  const auto mask = (static_cast<std::uintmax_t>(1) << (last_bits + 1)) - 1;

  return static_cast<bool>(n & mask);
}

// Test if none of the bit is set to 1
NODISCARD constexpr
Expected<bool> testBitNone(const unsigned_integral auto n)
{
  if (const auto result = testBitAny(n); !result) {
    return result;
  } else {
    return !*result;
  }
}

// Test if none of the bit is set to 1
NODISCARD constexpr
Expected<bool> testBitNone(const unsigned_integral auto n, const integral auto last_bits)
{
  if (const auto result = testBitAny(n, last_bits); result) {
    return !*result;
  } else {
    return result;
  }
}


/**
 * Bit setting functions (mutators)
*/

// Set a particular bit to 1
template <unsigned_integral U>
NODISCARD constexpr
Expected<U> setBit(const U n, const integral auto pos)
{
  if (!checkBitInRange<U>(pos)) {
    return std::unexpected(std::domain_error("pos out of range"));
  } else {
    return n | (static_cast<U>(1) << pos);
  }
}

// Set all bit to 1
template <unsigned_integral U>
NODISCARD constexpr
Expected<U> setBit(const U n) { return n | static_cast<U>(-1); }

// Set a bit to 0
template <unsigned_integral U>
NODISCARD constexpr
Expected<U> resetBit(const U n, const integral auto pos)
{
  if (!checkBitInRange<U>(pos)) {
    return std::unexpected(std::domain_error("pos our of range"));
  }

  if (const auto result = testBit<U>(n, pos); !result) {
    return std::unexpected(std::domain_error("freefunc::testBit function failed"));
  } else if (!*result) {
    return n;
  } else {
    return n ^ (static_cast<U>(1) << pos);
  }
}

// Set all bit to 0
NODISCARD constexpr
auto resetBit(const unsigned_integral auto n) -> Expected<decltype(n)> { return n & 0u; }

// Toggle a bit
template <unsigned_integral U>
NODISCARD constexpr
Expected<U> flipBit(const U n, const integral auto pos)
{
  if (!checkBitInRange<U>(pos)) {
    return std::unexpected(std::domain_error("pos out of range"));
  }

  return n ^ (static_cast<U>(1) << pos);
}

// Toggle all bits
NODISCARD constexpr
auto flipBit(const unsigned_integral auto n) -> Expected<decltype(n)> { return ~n; }


#undef NODISCARD

/**
 * Code ends.
*/

}

}
