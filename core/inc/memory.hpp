/**
 * @file  memory.hpp
 * @brief The memory model of prototype CPU
 * 
 * @author Yetong (Tony) Li
 * @date Sep, 2023
*/

#pragma once


#include <concepts>
#include <array>
#include <climits>
#include <variant>


namespace prototype::memory
{
    template <std::unsigned_integral Width, std::size_t Size>
    class Memory
    {
    public:
        using memory_width_type = Width;
        using memory_size_type  = std::size_t;
        using memory_model_type = std::array<memory_width_type, Size>;

        const memory_size_type m_memory_size  = Size;
        const memory_size_type m_memory_width = sizeof(memory_width_type) * CHAR_BIT;

    public:
        [[nodiscard]] inline constexpr
        auto checkAddressInrange(const memory_size_type address) noexcept
            -> bool
        {
            return address >= 0 && address < m_memory_size;
        }

        constexpr
        auto write(const memory_width_type data, const memory_size_type address) noexcept
            -> std::variant<bool, std::domain_error>
        {
            if (!checkAddressInrange(address)) {
                return std::domain_error("Address out of range.");
            }

            m_memory[address] = std::move(data);

            return true;
        }

        [[nodiscard]] constexpr
        auto read(const memory_size_type address) noexcept
            -> std::variant<memory_width_type, std::domain_error>
        {
            if (!checkAddressInrange(address)) {
                return std::domain_error("Address out of range.");
            }

            return m_memory.at(address);
        }

        constexpr inline
        auto clear() noexcept
            -> void
        {
            m_memory.fill(0x0);
        }

        constexpr
        auto clear(const memory_size_type begin, const memory_size_type end) noexcept
            -> std::variant<bool, std::domain_error>
        {
            if (begin == 0 && end == m_memory_size - 1) {
                clear();
                return true;
            }

            if (!checkAddressInrange(begin) || !checkAddressInrange(end)) {
                return std::domain_error("Address out of range.");
            }

            for (memory_size_type i{ begin }; i <= end; ++i) {
                m_memory[i] = 0x0;
            }

            return true;
        }

    private:
        memory_model_type m_memory{};
    };
}
