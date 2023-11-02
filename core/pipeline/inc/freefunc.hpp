#pragma once


#include <concepts>
#include <cstdint>
#include <climits>
#include <variant>
#include <stdexcept>


namespace pipeline::freefunc
{
    template <std::unsigned_integral U> [[nodiscard]] inline constexpr
    auto checkBitInRange(const std::integral auto position) noexcept
        -> bool
    {
        static_assert(CHAR_BIT == 8u);
        return position >= 0 && position < (sizeof(U) * CHAR_BIT);
    }

    [[nodiscard]] static inline constexpr
    auto checkBitInRange(const std::unsigned_integral auto n, const std::integral auto position) noexcept
        -> bool
    {
        return freefunc::checkBitInRange<decltype(n)>(position);
    }

    template <std::unsigned_integral U> [[nodiscard]] constexpr
    auto testBit(const U n, const std::integral auto position) noexcept
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInRange(n, position)) {
            return std::domain_error("Bit position out of range");
        }

        return static_cast<bool>(n & (static_cast<U>(1) << position));
    }

    [[nodiscard]] constexpr
    auto testBitAll(const std::unsigned_integral auto n) noexcept
        -> std::variant<bool, std::domain_error>
    {
        static_assert(CHAR_BIT == 8u);
        const std::size_t n_size{ sizeof(n) * CHAR_BIT - 1 };

        if constexpr (sizeof(n) == sizeof(std::uintmax_t))
        {
            using return_t = std::variant<bool, std::domain_error>;

            if (const return_t result{ freefunc::testBit(n, n_size) };
                result.index() == 0 && !std::get<0>(result))
            {
                return false;
            }
            else if (result.index() != 0)
            {
                return result;
            }
        }

        const std::unsigned_integral auto mask{ (static_cast<std::uintmax_t>(1) << n_size) - 1 };
        return (n & mask) == mask;
    }

    [[nodiscard]] static constexpr
    auto testBitAll(const std::unsigned_integral auto n, const std::size_t last_bits) noexcept
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInRange(n, last_bits)) {
            return std::domain_error("last_bits out of range");
        }

        if (!freefunc::checkBitInRange(n, last_bits + 1)) {
            return freefunc::testBitAll(n);
        }

        const std::unsigned_integral auto mask{ (static_cast<std::uintmax_t>(1) << (last_bits + 1)) - 1 };
        return (n & mask) == mask;
    }

    [[nodiscard]] constexpr
    auto testBitAny(const std::unsigned_integral auto n) noexcept
        -> std::variant<bool, std::domain_error>
    {
        return static_cast<bool>(n);
    }

    [[nodiscard]] constexpr
    auto testBitAny(const std::unsigned_integral auto n, const std::size_t last_bits) noexcept
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInRange(n, last_bits)) {
            return std::domain_error("last_bits out of range");
        }

        if (!freefunc::checkBitInRange(n, last_bits + 1)) {
            return freefunc::testBitAny(n);
        }

        const std::unsigned_integral auto mask{ (static_cast<std::uintmax_t>(1) << (last_bits + 1)) - 1 };
        return static_cast<bool>(n & mask);
    }

    template <typename T>
    concept valid_variant_type = requires(T param) {
        { param.index() } -> std::convertible_to<std::size_t>;
        requires std::convertible_to<decltype(std::get<0>(param)), bool>;
    };

    [[nodiscard]] constexpr
    auto testBitNone(const std::unsigned_integral auto n) noexcept
        -> std::variant<bool, std::domain_error>
    {
        static_assert(valid_variant_type<decltype(freefunc::testBitAny(n))>);
        
        if (const auto result{ freefunc::testBitAny(n) };
            result.index() != 0)
        {
            return result;
        }
        else
        {
            return !std::get<0>(result);
        }
    }

    [[nodiscard]] constexpr
    auto testBitNone(const std::unsigned_integral auto n, const std::size_t last_bits)
        -> std::variant<bool, std::domain_error>
    {
        static_assert(valid_variant_type<decltype(freefunc::testBitAny(n, last_bits))>);

        if (const auto result{ freefunc::testBitAny(n, last_bits) };
            result.index() == 0)
        {
            return !std::get<0>(result);
        }
        else
        {
            return result;
        }
    }

    template <std::unsigned_integral U> constexpr
    auto setBit(std::add_lvalue_reference_t<U> n, const std::integral auto position)
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInRange<U>(position)) {
            return std::domain_error("position out of range");
        }

        n |= (static_cast<U>(1) << position);

        return true;
    }

    template <std::unsigned_integral U> constexpr
    auto setBit(std::add_lvalue_reference_t<U> n) noexcept
        -> std::variant<bool, std::domain_error>
    {
        n |= static_cast<U>(-1);
        return true;
    }

    template <std::unsigned_integral U> constexpr
    auto resetBit(std::add_lvalue_reference_t<U> n, const std::integral auto position)
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInRange<U>(position)) {
            return std::domain_error("position our of range");
        }

        static_assert(valid_variant_type<decltype(freefunc::testBit<U>(n, position))>);

        if (const auto result{ freefunc::testBit<U>(n, position) };
            result.index() != 0)
        {
            return std::domain_error("freefunc::testBit function failed");
        }
        else if (!std::get<0>(result))
        {
            return true;
        }

        n ^= (static_cast<U>(1) << position);
        return true;
    }

    inline constexpr
    auto resetBit(std::unsigned_integral auto& n) noexcept
        -> std::variant<bool, std::domain_error>
    {
        n &= 0u;
        return true;
    }

    template <std::unsigned_integral U> constexpr
    auto flipBit(std::add_lvalue_reference_t<U> n, const std::integral auto position)
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInRange<U>(position)) {
            return std::domain_error("position out of range");
        }

        n ^= (static_cast<U>(1) << position);
        return true;
    }

    inline constexpr
    auto flipBit(std::unsigned_integral auto& n) noexcept
        -> std::variant<bool, std::domain_error>
    {
        n = ~n;
        return true;
    }

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
        static constexpr bool small = sizeof(U) < sizeof(std::uint32_t);

        using type = std::conditional_t<small, std::uint32_t, U>;
    };

    template <typename T>
    using make_promoted_t = typename MakePromoted<T>::type;

    template <typename T> [[nodiscard]] static inline constexpr
    auto promote(const T n) noexcept
        -> make_promoted_t<T>
    {
        return n;
    }

    template <class T, template <class...> class Template>
    struct is_specialization : std::false_type {};

    template <template <class...> class Template, class... Args>
    struct is_specialization<Template<Args...>, Template> : std::true_type {};
}
