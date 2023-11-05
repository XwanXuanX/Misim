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

  template <typename Arg, typename Target>
  concept indexable = requires(Arg arg) {
    { arg.operator[](static_cast<std::size_t>(0)) } -> std::same_as<std::add_lvalue_reference_t<Target>>;    
    { arg.at(static_cast<std::size_t>(0)) };
  };

  template <std::size_t MemorySize_,
    valid_tag_type ConstructionTag = ArrayTag,
    std::unsigned_integral SlotType = byte_type>
  class Memory
  {
  public:
    using slot_type = SlotType;
    using size_type = std::size_t;

    using array_model = std::array<slot_type, MemorySize_>;
    using vector_model = std::vector<slot_type>;

    static_assert(
        std::is_default_constructible_v<array_model> && std::is_default_constructible_v<vector_model>
    );

    using model_type = std::conditional_t<std::is_same_v<ConstructionTag, ArrayTag>, array_model, vector_model>;

    const size_type m_memory_size{ MemorySize_ };
    const size_type m_memory_width{ sizeof(slot_type) * CHAR_BIT };

    template <typename T, typename Error = std::domain_error>
    using return_type = std::variant<T, Error>;

  public:
    [[nodiscard]] explicit constexpr Memory() noexcept
      : m_memory{}
    {
      if constexpr (std::is_same_v<model_type, vector_model>) {
        static_assert(
          requires {
            { m_memory.resize(static_cast<std::size_t>(0)) } -> std::same_as<void>;
          }, "Invalid memory model type."
        );

        m_memory.resize(m_memory_size);
      }

      static_assert(requires { { m_memory.size() } -> std::convertible_to<std::size_t>; });
      assert(m_memory.size() == m_memory_size);
    }

  public:
    [[nodiscard]] inline constexpr std::unsigned_integral auto getMemorySize() const noexcept
    {
      assert(m_memory.size() == m_memory_size);
      return m_memory.size();
    }

    [[nodiscard]] inline constexpr bool checkAddrInRange(const std::integral auto addr) const noexcept
    {
      return addr >= 0 && addr < getMemorySize();
    }

    return_type<bool> write(const slot_type data, const std::integral auto addr)
    {
      if (!checkAddrInRange(addr)) {
        return std::domain_error("invalid range addr");
      }

      static_assert(indexable<model_type, slot_type>, "Invalid memory model type");

      if constexpr (sizeof(slot_type) > 8u) { m_memory[addr] = std::move(data); }
      else { m_memory[addr] = data; }

      return true;
    }

    [[nodiscard]] constexpr return_type<slot_type> read(const std::integral auto addr) const
    {
      if (!checkAddrInRange(addr)) {
        return std::domain_error("invalid range addr");
      }

      static_assert(indexable<model_type, slot_type>, "Invalid memory model type");

      return m_memory.at(addr);
    }

    inline constexpr void clear() noexcept
    {
      if constexpr (std::is_same_v<model_type, array_model>) {
        m_memory.fill(0u);
      } else if constexpr (std::is_same_v<model_type, vector_model>) {
        m_memory.assign(m_memory_size, 0u);
        assert(getMemorySize() == m_memory_size);
      } else {
        assert(false);
      }
    }
    
  private:
    model_type m_memory{};
  };
}
