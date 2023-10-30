#pragma once


#include <array>
#include <climits>
#include <cstdint>
#include <concepts>
#include <cassert>
#include <variant>
#include <stdexcept>


namespace pipeline::memory
{
    using byte_type = std::uint8_t;
    static_assert(
        sizeof(byte_type) * CHAR_BIT == 8u,
        "Invalid byte_type / CHAR_BIT."
    );

    template <std::size_t Size_>
    class Memory
    {
    public:
        using slot_type  = byte_type;
        using size_type  = std::size_t;
        using model_type = std::array<slot_type, Size_>;

        const size_type m_memory_size  = Size_;
        const size_type m_memory_width = sizeof(slot_type) * CHAR_BIT;

    public:
        [[nodiscard]] inline constexpr
        auto checkAddrInRange(const std::unsigned_integral auto addr) const noexcept
            -> bool
        {
            assert(m_memory.size() == m_memory_size);
            return addr >= 0 && addr < m_memory_size;
        }

        constexpr
        auto write(const slot_type data, const std::unsigned_integral auto addr)
            -> std::variant<bool, std::domain_error>
        {
            if (!checkAddrInRange(addr))
            {
                return std::domain_error("addr outside of valid range.");
            }

            if constexpr (sizeof(slot_type) > 8u)
            {
                m_memory[addr] = std::move(data);
            }
            else
            {
                m_memory[addr] = data;
            }

            return true;
        }

        [[nodiscard]] constexpr
        auto read(const std::unsigned_integral auto addr) const
            -> std::variant<slot_type, std::domain_error>
        {
            if (!checkAddrInRange(addr))
            {
                return std::domain_error("addr outside of valid range.");
            }

            return m_memory.at(addr);
        }

        inline constexpr
        auto clear() noexcept
            -> void
        {
            m_memory.fill(0u);
        }

    private:
        model_type m_memory{};
    };
}
