#pragma once


#include <array>
#include <cassert>
#include <concepts>
#include <string>
#include <stdexcept>
#include <functional>


namespace pipeline::registers
{
  struct Association
  {
    static inline constexpr std::unsigned_integral auto max_size{ 10u };
    using string_type = std::array<char, max_size>;

    string_type str1{};
    string_type str2{};
  };

  using string_type = typename Association::string_type;

  [[nodiscard]] consteval string_type operator ""_r(const char* str_literal,
    [[maybe_unused]] const std::size_t len)
  {
    const std::unsigned_integral auto str_len{ std::string::traits_type::length(str_literal) };

    string_type string{};
    assert(string.size() >= str_len && "Container not large enough");

    for (int i{ 0 }; i < str_len; ++i) {
      string[i] = str_literal[i];
    }

    return string;
  }

  template <Association... Assoc>
  struct RegisterMapping
  {
  public:
    static constexpr std::unsigned_integral auto pack_size{ sizeof...(Assoc) };

    using array_type = std::conditional_t<(pack_size > 0), std::array<Association, pack_size>, void>;

    template <std::unsigned_integral T, typename Error = std::domain_error>
    using return_type = std::variant<T, Error>;

  private:
    template <typename Arr, Association A, Association... As>
    static constexpr void getAssociation(std::add_lvalue_reference_t<Arr> arr, std::size_t index = 0)
    {
      static_assert(!std::same_as<Arr, void> && requires { arr[0u]; });

      arr[index] = A;
    
      if constexpr (sizeof...(As) > 0) {
        getAssociation<Arr, As...>(arr, index + 1);
      }
    }

    template <typename T>
      requires(std::is_same_v<std::remove_cvref_t<T>, std::string_view> ||
        std::is_same_v<std::remove_cvref_t<T>, string_type>)
    [[nodiscard]] static constexpr std::integral auto parseRegname(std::add_const_t<T> reg_name)
    {
      static_assert(
        requires {
          { reg_name.size() } -> std::convertible_to<std::size_t>;
          { reg_name.at(static_cast<std::size_t>(0)) } -> std::convertible_to<char>;
        }
      );

      // Introspection
      auto getSize = [&reg_name]() -> std::unsigned_integral auto {
        if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string_view>) {
          return reg_name.size();
        } else {
          for (std::size_t i{ 0 }; const auto ch : reg_name) {
            if (ch == 0) {
              return i;
            }
            ++i;
          }

          return reg_name.size();
        }
      };

      const std::unsigned_integral auto ssize{ getSize() };

      if (ssize != 2 && ssize != 3) {
        return -1;
      }

      auto isdigit = [](const char ch) -> bool {
        return (ch >= 48 && ch <= 57);
      };

      if (!isdigit(reg_name.at(1)) || !isdigit(reg_name.at(ssize - 1))) {
        return -1;
      }

      const std::integral auto d1{ reg_name.at(1) - 48 };
      decltype(d1) d2{ reg_name.at(ssize - 1) - 48 };

      return (ssize == 2) ? d1 : (d1 * 10 + d2);
    }

  public:
    [[nodiscard]] static constexpr std::integral auto idx(const std::string_view reg_name)
    {
      if constexpr (pack_size == 0) {
        return parseRegname<std::string_view>(reg_name);
      } else {
        static_assert(!std::same_as<array_type, void>);

        array_type mapping{};
        getAssociation<decltype(mapping), Assoc...>(mapping);

        auto checkSame = [](const string_type& s1, const std::string_view& s2) noexcept -> bool {
          const std::unsigned_integral auto min_size = (s1.size() < s2.size()) ? s1.size() : s2.size();

          for (std::size_t i{ 0 }; i < min_size; ++i) {
            if (s1.at(i) == 0 || s1.at(i) != s2.at(i)) {
              return false;
            }
          }

          return true;
        };

        auto matchRegname = [&mapping, checkSame](const std::string_view reg_name) -> std::integral auto {
          for (int i{ 0 }; const auto& assoc : mapping) {
            if (checkSame(assoc.str1, reg_name)) {
              return i;
            }
            ++i;
          }

          return -1;
        };

        const std::integral auto index = matchRegname(reg_name);

        return (index < 0) ?
          parseRegname<std::string_view>(reg_name) :
          parseRegname<string_type>(mapping.at(index).str2);
      }
    }
  };

  enum class Status : std::uint8_t
  {
    N,  // Negative
    Z,  // Zero
    C,  // Carry
    V   // Overflow
  };

  template <typename M>
  concept valid_register_mapping = requires {
    { M::idx(std::string_view{}) } -> std::integral;
  };

  template <std::unsigned_integral GpWidth,
    std::size_t GpSize_,
    valid_register_mapping Mapping>
  class Registers
  {
  public:
    using gp_width_type = GpWidth;
    using gp_model_type = std::array<gp_width_type, GpSize_>;

    using psr_flag_type = bool;
    using psr_model_type = std::array<psr_flag_type, 4>;

    const std::size_t m_gp_size{ GpSize_ };
    const std::size_t m_psr_size{ 4u };

    using reg_mapping = Mapping;

    template <typename T, typename Error = std::domain_error>
    using return_ref_t = std::variant<std::reference_wrapper<T>, Error>;

    template <typename T, typename Error = std::domain_error>
    using return_val_t = std::variant<T, Error>;

  private:
    template <template <typename...> typename ReturnType, std::unsigned_integral ValType>
    [[nodiscard]] constexpr ReturnType<ValType> getGpHelper(const auto reg_name)
    {
      auto getIndex = [&reg_name] -> std::integral auto {
        if constexpr (std::convertible_to<std::remove_cvref_t<decltype(reg_name)>, int>) {
          return reg_name;
        } else if constexpr (std::same_as<std::remove_cvref_t<decltype(reg_name)>, std::string_view>) {
          return reg_mapping::idx(reg_name);
        } else {
          return -1;
        }
      };

      const std::integral auto index{ getIndex() };

      if (index < 0 || index >= m_gp_size) {
        return std::domain_error("invalid reg_name");
      } else {
        return m_gp.at(index);
      }
    }

  public:
    [[nodiscard]] constexpr return_ref_t<gp_width_type> getGeneralPurpose(const std::string_view reg_name)
    {
      return getGpHelper<return_ref_t, gp_width_type>(reg_name);
    }

    [[nodiscard]] constexpr return_ref_t<gp_width_type> getGeneralPurpose(const std::integral auto reg_name)
    {
      return getGpHelper<return_ref_t, gp_width_type>(reg_name);
    }

    [[nodiscard]] constexpr psr_flag_type& getProgramStatus(const Status flag_name) noexcept
    {
      return m_psr.at(static_cast<std::size_t>(flag_name));
    }
    
    [[nodiscard]] constexpr
    return_ref_t<psr_flag_type> getProgramStatus(const std::integral auto flag_name)
    {
      if (flag_name < 0 || flag_name >= m_psr_size) {
        return std::domain_error("invalid flag_name");
      }

      return m_psr.at(flag_name);
    }

    constexpr void clearProgramStatus() noexcept
    {
      m_psr.fill(false);
    }

  private:
    gp_model_type m_gp{};
    psr_model_type m_psr{};
  };
}
