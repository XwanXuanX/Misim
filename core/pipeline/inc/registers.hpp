#pragma once


#include <array>
#include <cassert>
#include <concepts>
#include <string>
#include <stdexcept>


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
    static consteval void getAssociation(std::add_lvalue_reference_t<Arr> arr, std::size_t index = 0)
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
    [[nodiscard]] static consteval std::integral auto parseRegname(std::add_const_t<T> reg_name)
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
    [[nodiscard]] static consteval std::integral auto idx(const std::string_view reg_name)
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


}
