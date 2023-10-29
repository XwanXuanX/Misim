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
    using byte_type = std::uint_fast8_t;

    static_assert(
        sizeof(byte_type) == 1u &&
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
        bool checkAddrInRange(const std::unsigned_integral auto addr) const noexcept
        {
            assert(m_memory.size() == m_memory_size);
            return addr >=0 && addr < m_memory_size;
        }

        constexpr
        std::variant<bool, std::domain_error>
        write(const slot_type data, const std::unsigned_integral auto addr)
        {
            if (!checkAddrInRange(addr))
            {
                return std::domain_error("addr outside of valid range.");
            }

            if constexpr (sizeof(slot_type) > 8u) {
                m_memory[addr] = std::move(data);
            } else {
                m_memory[addr] = data;
            }

            return true;
        }

        [[nodiscard]] constexpr
        std::variant<slot_type, std::domain_error>
        read(const std::unsigned_integral auto addr)
        {
            if (!checkAddrInRange(addr))
            {
                return std::domain_error("addr outside of valid range.");
            }

            return m_memory.at(addr);
        }

        inline constexpr
        void clear() noexcept
        {
            m_memory.fill(0u);
        }
        
    private:
        model_type m_memory{};
    };
}
