/**
 * @file  freefunc.hpp
 * @brief A collection of useful free functions
 * 
 * @author Yetong (Tony) Li
 * @date Sep, 2023
*/

#pragma once


#include <concepts>
#include <cstdint>
#include <climits>
#include <variant>
#include <stdexcept>


namespace prototype::freefunc
{
    template <std::unsigned_integral uint> [[nodiscard]] static inline constexpr
    auto checkBitInrange(const std::size_t position) noexcept
        -> bool
    {
        return position >= 0 && position < sizeof(uint) * CHAR_BIT;
    }

    [[nodiscard]] static inline constexpr
    auto checkBitInrange(const std::unsigned_integral auto n, const std::size_t position) noexcept
        -> bool
    {
        return freefunc::checkBitInrange<decltype(n)>(position);
    }

    template <std::unsigned_integral uint> [[nodiscard]] static constexpr
    auto testBit(const uint n, const std::size_t position) noexcept
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInrange(n, position))
        {
            return std::domain_error("Bit position out of bound.");
        }

        return std::variant<bool, std::domain_error>(
            static_cast<bool>(n & (static_cast<uint>(1) << position))
        );
    }

    [[nodiscard]] static constexpr
    auto testBitAll(const std::unsigned_integral auto n) noexcept
        -> std::variant<bool, std::domain_error>
    {
        const std::size_t n_size = sizeof(n) * CHAR_BIT - 1;
        if (sizeof(n) == sizeof(std::uintmax_t))
        {
            using return_t = std::variant<bool, std::domain_error>;
            if (
                const return_t result = freefunc::testBit(n, n_size);
                result.index() == 0 && !std::get<0>(result)
                )
            {
                return false;
            }
            else if (result.index() != 0)
            {
                return result;
            }
        }

        const auto mask = (static_cast<std::uintmax_t>(1) << n_size) - 1;
        return (n & mask) == mask;
    }

    [[nodiscard]] static constexpr
    auto testBitAll(const std::unsigned_integral auto n, const std::size_t last_nbit) noexcept
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInrange(n, last_nbit))
        {
            return std::domain_error("Last nbits out of bound.");
        }
        
        if (!freefunc::checkBitInrange(n, last_nbit + 1))
        {
            return freefunc::testBitAll(n);
        }

        const auto mask = (static_cast<std::uintmax_t>(1) << (last_nbit + 1)) - 1;
        return (n & mask) == mask;
    }

    [[nodiscard]] static inline constexpr
    auto testBitAny(const std::unsigned_integral auto n) noexcept
        -> bool
    {
        return static_cast<bool>(n);
    }

    [[nodiscard]] static constexpr
    auto testBitAny(const std::unsigned_integral auto n, const std::size_t last_nbit) noexcept
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInrange(n, last_nbit))
        {
            return std::domain_error("Last nbits out of bound.");
        }

        if (!freefunc::checkBitInrange(n, last_nbit + 1))
        {
            return freefunc::testBitAny(n);
        }

        const auto mask = (static_cast<std::uintmax_t>(1) << (last_nbit + 1)) - 1;
        return static_cast<bool>(n & mask);
    }

    [[nodiscard]] static inline constexpr
    auto testBitNone(const std::unsigned_integral auto n) noexcept
        -> bool
    {
        return !freefunc::testBitAny(n);
    }

    [[nodiscard]] static constexpr
    auto testBitNone(const std::unsigned_integral auto n, const std::size_t last_nbit) noexcept
        -> std::variant<bool, std::domain_error>
    {
        using return_t = std::variant<bool, std::domain_error>;

        if (
            const return_t result = freefunc::testBitAny(n, last_nbit);
            result.index() == 0
            )
        {
            return !std::get<0>(result);
        }
        else
        {
            return result;
        }
    }

    template <std::unsigned_integral uint> static constexpr
    auto setBit(uint& n, const std::size_t position) noexcept
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInrange<uint>(position))
        {
            return std::domain_error("Bit position out of bound.");
        }

        n |= (static_cast<uint>(1) << position);
        return true;
    }

    template <std::unsigned_integral uint> static inline constexpr
    auto setBit(uint& n) noexcept
        -> std::variant<bool, std::domain_error>
    {
        n |= static_cast<uint>(-1);
        return true;
    }

    template <typename... Poses>
    concept valid_positions = requires
    {
        requires
        (std::same_as<std::size_t, Poses> || ...);
    };

    static constexpr
    auto setBit(std::unsigned_integral auto&         n,
                const std::same_as<std::size_t> auto position,
                const valid_positions auto...        positions) noexcept
        -> std::variant<bool, std::domain_error>
    {
        using return_t = std::variant<bool, std::domain_error>;

        if (
            const return_t result = freefunc::setBit(n, position);
            result.index() != 0
            )
        {
            return result;
        }

        return_t result;
        if constexpr (sizeof...(positions) > 0)
        {
            result = freefunc::setBit(n, positions...);
        }

        return result;
    }

    template <std::unsigned_integral uint> static constexpr
    auto resetBit(uint& n, const std::size_t position) noexcept
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInrange<uint>(position))
        {
            return std::domain_error("Bit position out of bound.");
        }

        using return_t = std::variant<bool, std::domain_error>;

        if (
            const return_t result = freefunc::testBit<uint>(n, position);
            result.index() != 0
            )
        {
            return std::domain_error("freefunc::TestBit function failed.");
        }
        else if (!std::get<0>(result))
        {
            return true;
        }

        n ^= (static_cast<uint>(1) << position);
        return true;
    }

    static inline constexpr
    auto resetBit(std::unsigned_integral auto& n) noexcept
        -> std::variant<bool, std::domain_error>
    {
        n &= 0x0;
        return true;
    }

    static constexpr
    auto resetBit(std::unsigned_integral auto&         n,
                  const std::same_as<std::size_t> auto position,
                  const valid_positions auto...        positions) noexcept
        -> std::variant<bool, std::domain_error>
    {
        using return_t = std::variant<bool, std::domain_error>;

        if (
            const return_t result = freefunc::resetBit(n, position);
            result.index() != 0
            )
        {
            return result;
        }

        return_t result;
        if constexpr (sizeof...(positions) > 0)
        {
            result = freefunc::resetBit(n, positions...);
        }

        return result;
    }

    [[nodiscard]] static inline constexpr
    auto pos(const std::unsigned_integral auto position) noexcept
        -> std::size_t
    {
        return static_cast<std::size_t>(position);
    }

    template <std::unsigned_integral uint> static constexpr
    auto flipBit(uint& n, const std::size_t position) noexcept
        -> std::variant<bool, std::domain_error>
    {
        if (!freefunc::checkBitInrange<uint>(position))
        {
            return std::domain_error("Bit position out of bound.");
        }

        n ^= (static_cast<uint>(1) << position);
        return true;
    }

    static inline constexpr
    auto flipBit(std::unsigned_integral auto& n) noexcept
        -> std::variant<bool, std::domain_error>
    {
        n = ~n;
        return true;
    }

    static constexpr
    auto flipBit(std::unsigned_integral auto&         n,
                 const std::same_as<std::size_t> auto position,
                 const valid_positions auto...        positions) noexcept
        -> std::variant<bool, std::domain_error>
    {
        using return_t = std::variant<bool, std::domain_error>;
        
        if (
            const return_t result = freefunc::flipBit(n, position);
            result.index() != 0
            )
        {
            return result;
        }

        return_t result;
        if constexpr (sizeof...(positions) > 0)
        {
            result = freefunc::flipBit(n, positions...);
        }

        return result;
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

    template <std::unsigned_integral uint>
    struct MakePromoted
    {
        static constexpr bool small = sizeof(uint) < sizeof(std::uint32_t);

        using type = std::conditional_t<small, std::uint32_t, uint>;
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
