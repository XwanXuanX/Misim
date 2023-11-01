#pragma once


#include <array>
#include <vector>
#include <climits>
#include <cstdint>
#include <concepts>
#include <cassert>
#include <variant>
#include <stdexcept>


namespace pipeline::memory
{
    using byte_type = std::uint8_t;
    static_assert(sizeof(byte_type) * CHAR_BIT == 8u);

    struct ArrayTag  {};
    struct VectorTag {};

    template <typename Tag>
    concept valid_tag_type = std::same_as<Tag, ArrayTag> || std::same_as<Tag, VectorTag>;

    template <valid_tag_type ConstructionTag, std::size_t MemorySize_>
    class Memory
    {
    public:
        using slot_type = byte_type;
        using size_type = std::size_t;

        using array_model  = std::array<slot_type, MemorySize_>;
        using vector_model = std::vector<slot_type>;

        static_assert(
            std::is_default_constructible_v<array_model> &&
            std::is_default_constructible_v<vector_model>
        );

        using model_type = std::conditional_t<
            std::is_same_v<ConstructionTag, ArrayTag>,
            array_model, vector_model
        >;

        const size_type m_memory_size { MemorySize_ };
        const size_type m_memory_width{ sizeof(slot_type) * CHAR_BIT };
    
    public:
        [[nodiscard]] explicit constexpr
        Memory() noexcept
            : m_memory{}
        {
            if constexpr (std::is_same_v<model_type, vector_model>) {
                static_assert(
                    requires {
                        { m_memory.resize(static_cast<std::size_t>(0)) } -> std::same_as<void>;
                    },
                    "Invalid memory model type."
                );
                // Enlarge the vector to desired size
                m_memory.resize(m_memory_size);
            }

            static_assert(requires { { m_memory.size() } -> std::convertible_to<std::size_t>; });
            assert(m_memory.size() == m_memory_size);
        }

    public:
        [[nodiscard]] inline constexpr
        std::unsigned_integral auto getMemorySize() const noexcept
        {
            assert(m_memory.size() == m_memory_size);
            return m_memory.size();
        }

        [[nodiscard]] inline constexpr
        auto checkAddrInRange(const std::integral auto addr) const noexcept
            -> bool
        {
            return addr >= 0 && addr < getMemorySize();
        }

        constexpr
        auto write(const slot_type data, const std::integral auto addr)
            -> std::variant<bool, std::domain_error>
        {
            if (!checkAddrInRange(addr)) {
                return std::domain_error("invalid range addr");
            }

            static_assert(
                requires {
                    { m_memory.operator[](static_cast<std::size_t>(0)) }
                        -> std::same_as<std::add_lvalue_reference_t<slot_type>>;
                },
                "Invalid memory model type."
            );

            if constexpr (sizeof(slot_type) > 8u) {
                m_memory[addr] = std::move(data);
            }
            else {
                m_memory[addr] = data;
            }

            return true;
        }

        [[nodiscard]] constexpr
        auto read(const std::integral auto addr) const
            -> std::variant<slot_type, std::domain_error>
        {
            if (!checkAddrInRange(addr)) {
                return std::domain_error("invalid range addr");
            }

            static_assert(
                requires { m_memory.at(static_cast<std::size_t>(0)); },
                "Invalid memory model type."
            );

            return m_memory.at(addr);
        }

        inline constexpr
        auto clear() noexcept
            -> void
        {
            if constexpr (std::is_same_v<model_type, array_model>) {
                m_memory.fill(0u);
            }
            else if constexpr (std::is_same_v<model_type, vector_model>) {
                m_memory.assign(m_memory_size, 0u);
                assert(getMemorySize() == m_memory_size);
            }
            else {
                assert(false);
            }
        }

    private:
        model_type m_memory{};
    };
}
