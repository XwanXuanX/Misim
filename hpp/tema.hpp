/*********************************************************************
 * 
 *  @file   tema.hpp
 *  @brief  Assembly Abstract machine (Ver.2)
 *  
 *  @author Tony (Yetong) Li
 *  @date   August 2023
 *  
 *  @detail
 * 
 *  In the first attempt, I created my own instruction set and decoder,
 *  so that I can run a single process on a single-core CPU.
 * 
 *  This is not enough... Here comes the second trial!
 * 
 *  God bless me.
 * 
 *********************************************************************/

/*********************************************************************
 * 
 *  Highly abstract overview:
 *      CPU structures:
 *      Core x1, Kernel x1, RAM x1, Disk x1, Page xN, Process xN
 * 
 *  Objectives:
 *      1. Contain all features Ver.1 has
 *	    2. Page, main memory, secondary memory, process, cores, kernel
 *	    3. Allow page swapping and multiple processes on one core.
 *	    4. C++20 std minimum!
 *	    5. As thread-safe as possible!
 * 
 *  Program layout:
 *      Page:				array<integer, size> page;
 *
 *      MainMemory:		    stack<integer> empty_slots;
 *							array<Page, size> memory;
 *
 *	    SecondaryMemory:	vector<Page> disk;
 *
 *	    Process:            vector<Page*> frame;
 *
 *	    Core:				class StateSaver;
 *							Kernel* pkernel;
 *							vector<Process*> procs;
 *							unordered_map<PID, StateSaver> cpu_states;
 *
 *      Kernel:             TBD
 * 
 *********************************************************************/


#include <bit>
#include <map>
#include <array>
#include <vector>
#include <cstdint>
#include <utility>
#include <numeric>
#include <climits>
#include <fstream>
#include <iostream>
#include <concepts>
#include <expected>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <filesystem>
#include <unordered_set>
#include <unordered_map>


namespace misim::freefuncs
{
    template <std::unsigned_integral uint>
    [[nodiscard]] constexpr auto
    checkBitInrange(const std::size_t position) noexcept
        -> bool
    {
        return (
            position >= 0
            && position < sizeof(uint) * CHAR_BIT
        );
    }


    [[nodiscard]] constexpr auto
    checkBitInrange(const std::unsigned_integral auto n, const std::size_t position) noexcept
        -> bool
    {
        return freefuncs::checkBitInrange<decltype(n)>(position);
    }


    template <std::unsigned_integral uint>
    [[nodiscard]] constexpr auto
    testBit(const uint n, const std::size_t position) noexcept
        -> std::expected<bool, std::domain_error>
    {
        if (!freefuncs::checkBitInrange(n, position)) {
            return std::unexpected(
                std::domain_error("Bit position out of bound.")
            );
        }

        return n & (static_cast<uint>(1) << position);
    }
    
    
    [[nodiscard]] constexpr auto
    testBitAll(const std::unsigned_integral auto n) noexcept
        -> std::expected<bool, std::domain_error>
    {
        const std::size_t n_size = sizeof(n) * CHAR_BIT - 1;

        if (sizeof(n) == sizeof(std::uintmax_t))
        {
            using return_t = std::expected<bool, std::domain_error>;
            if (
                const return_t result = freefuncs::testBit(n, n_size);
                result && !*result
                )
            {
                return false;
            }
            else if (!result) {
                return result;
            }
        }

        const auto mask = (static_cast<std::uintmax_t>(1) << n_size) - 1;

        return (n & mask) == mask;
    }


    [[nodiscard]] constexpr auto
    testBitAll(const std::unsigned_integral auto n, const std::size_t last_nbit) noexcept
        -> std::expected<bool, std::domain_error>
    {
        if (!freefuncs::checkBitInrange(n, last_nbit)) {
            return std::unexpected(
                std::domain_error("Last nbits out of bound.")
            );
        }

        if (!freefuncs::checkBitInrange(n, last_nbit + 1)) {
            return freefuncs::testBitAll(n);
        }

        const auto mask = (static_cast<std::uintmax_t>(1) << (last_nbit + 1)) - 1;

        return (n & mask) == mask;
    }


    [[nodiscard]] constexpr auto
    testBitAny(const std::unsigned_integral auto n) noexcept
        -> bool
    {
        return static_cast<bool>(n);
    }


    [[nodiscard]] constexpr auto
    testBitAny(const std::unsigned_integral auto n, const std::size_t last_nbit) noexcept
        -> std::expected<bool, std::domain_error>
    {
        if (!freefuncs::checkBitInrange(n, last_nbit)) {
            return std::unexpected(
                std::domain_error("Last nbits out of bound.")
            );
        }

        if (!freefuncs::checkBitInrange(n, last_nbit + 1)) {
            return freefuncs::testBitAny(n);
        }

        const auto mask = (static_cast<std::uintmax_t>(1) << (last_nbit + 1)) - 1;

        return static_cast<bool>(n & mask);
    }


    [[nodiscard]] constexpr auto
    testBitNone(const std::unsigned_integral auto n) noexcept
        -> bool
    {
        return !freefuncs::testBitAny(n);
    }

        
    [[nodiscard]] constexpr auto
    testBitNone(const std::unsigned_integral auto n, const std::size_t last_nbit) noexcept
        -> std::expected<bool, std::domain_error>
    {
        using return_t = std::expected<bool, std::domain_error>;
        if (
            const return_t result = freefuncs::testBitAny(n, last_nbit);
            result
            )
        {
            return !*result;
        }
        else {
            return result;
        }
    }


    template <std::unsigned_integral uint>
    constexpr auto
    setBit(uint& n, const std::size_t position) noexcept
        -> std::expected<void, std::domain_error>
    {
        if (!freefuncs::checkBitInrange<uint>(position)) {
            return std::unexpected(
                std::domain_error("Bit position out of bound.")
            );
        }

        n |= (static_cast<uint>(1) << position);
        return {};
    }


    template <std::unsigned_integral uint>
    constexpr auto
    setBit(uint& n) noexcept
        -> std::expected<void, std::domain_error>
    {
        n |= static_cast<uint>(-1);
        return {};
    }


    template <class... Poses>
    concept ValidPoses = (std::same_as<std::size_t, Poses> || ...);


    constexpr auto
    setBit(
        std::unsigned_integral auto&            n,
        const std::same_as<std::size_t> auto    position,
        const ValidPoses auto...                positions
    ) noexcept
        -> std::expected<void, std::domain_error>
    {
        using return_t = std::expected<void, std::domain_error>;
        if (
            const return_t result = freefuncs::setBit(n, position);
            !result
            )
        {
            return result;
        }

        return_t result;

        if constexpr (sizeof...(positions) > 0) {
            result = freefuncs::setBit(n, positions...);
        }

        return result;
    }


    template <std::unsigned_integral uint>
    constexpr auto
    resetBit(uint& n, const std::size_t position) noexcept
        -> std::expected<void, std::domain_error>
    {
        if (!freefuncs::checkBitInrange<uint>(position)) {
            return std::unexpected(
                std::domain_error("Bit position out of bound.")
            );
        }

        using return_t = std::expected<bool, std::domain_error>;

        if (
            const return_t result = freefuncs::testBit<uint>(n, position);
            !result
            )
        {
            return std::unexpected(
                std::domain_error("freefunc::TestBit function failed.")
            );
        }
        else if (!*result) {
            return {};
        }

        n ^= (static_cast<uint>(1) << position);

        return {};
    }


    inline constexpr auto
    resetBit(std::unsigned_integral auto& n) noexcept
        -> std::expected<void, std::domain_error>
    {
        n &= 0x0;
        return {};
    }


    constexpr auto
    resetBit(
        std::unsigned_integral auto&            n,
        const std::same_as<std::size_t> auto    position,
        const ValidPoses auto...                positions
    ) noexcept
        -> std::expected<void, std::domain_error>
    {
        using return_t = std::expected<void, std::domain_error>;

        if (
            const return_t result = freefuncs::resetBit(n, position);
            !result
            )
        {
            return result;
        }

        return_t result;

        if constexpr (sizeof...(positions) > 0) {
            result = freefuncs::resetBit(n, positions...);
        }

        return result;
    }


    [[nodiscard]] inline constexpr auto
    pos(const std::unsigned_integral auto position) noexcept
        -> std::size_t
    {
        return static_cast<std::size_t>(position);
    }


    template <std::unsigned_integral uint>
    constexpr auto
    flipBit(uint& n, const std::size_t position) noexcept
        -> std::expected<void, std::domain_error>
    {
        if (!freefuncs::checkBitInrange<uint>(position)) {
            return std::unexpected(
                std::domain_error("Bit position out of bound.")
            );
        }

        n ^= (static_cast<uint>(1) << position);

        return {};
    }


    inline constexpr auto
    flipBit(std::unsigned_integral auto& n) noexcept
        -> std::expected<void, std::domain_error>
    {
        n = ~n;
        return {};
    }


    constexpr auto
    flipBit(
        std::unsigned_integral auto&            n,
        const std::same_as<std::size_t> auto    position,
        const ValidPoses auto...                positions
    ) noexcept
        -> std::expected<void, std::domain_error>
    {
        using return_t = std::expected<void, std::domain_error>;

        if (
            const return_t result = freefuncs::flipBit(n, position);
            !result
            )
        {
            return result;
        }

        return_t result;

        if constexpr (sizeof...(positions) > 0) {
            result = freefuncs::flipBit(n, positions...);
        }

        return result;
    }


    /**
     * The solution to solve problem with multiply invokes undefined behaviour
     * 
     * Explanation:
     * 
     *  Define A & B be unsigned integers, whose size less than signed int (normally 32bits)
     *  If the product of A * B overflows, A & B will be promoted to signed int and multiply.
     *  However, if the result overflows, signed int will invoke undefined behaviour.
     * 
     * Solution:
     * 
     *  Use template method to let unsigned integers only promote to unsigned int.
     *  After the operation is done, truncated the result to original size.
     */


    template <std::unsigned_integral uint>
    struct MakePromoted
    {
        static constexpr bool small = sizeof(uint) < sizeof(std::uint32_t);

        using type = std::conditional_t<small, std::uint32_t, uint>;
    };


    template <typename T>
    using make_promoted_t = typename MakePromoted<T>::type;


    template <typename T>
    [[nodiscard]] inline constexpr auto
    promote(const T n) noexcept
        -> make_promoted_t<T>
    {
        return n;
    }
}






//namespace misim::page {
//  template <std::unsigned_integral width, std::size_t size>
//  class Page {
//  public:
//    using page_width = width;
//    using size_type = std::size_t;
//    const std::size_t page_size = size;
//    using page_model = std::array<page_width, size>;
//
//    _NODISCARD constexpr explicit Page() = default;
//
//    template <class Array>
//      requires (std::same_as<page_model, std::remove_reference_t<Array>> and std::is_constructible_v<page_model, Array>)
//    _NODISCARD constexpr explicit Page(Array&& array, const bool in_memory, const size_type start_addr, const class Process* pMaster)
//      noexcept (std::is_nothrow_constructible_v<page_model, Array>) :
//      page_{ std::forward<Array>(array) }, in_memory_{ in_memory }, start_addr_{ start_addr }, pMaster_{ pMaster } {}
//
//    _NODISCARD constexpr auto CheckAddrInrange(const size_type addr) noexcept -> bool {
//      return addr >= start_addr_ and addr < (start_addr_ + page_size);
//    }
//
//    // modify
//    constexpr auto Write(const page_width data, const size_type addr) noexcept -> std::expected<void, std::domain_error> {
//      if (not CheckAddrInrange(addr))
//        return std::unexpected(std::domain_error("Address out of bound."));
//      page_[addr - start_addr_] = std::move(data);
//      return std::expected<void, std::domain_error>{};
//    }
//
//    // access
//    _NODISCARD constexpr auto Read(const size_type addr) noexcept -> std::expected<page_width, std::domain_error> {
//      if (not CheckAddrInrange(addr))
//        return std::unexpected(std::domain_error("Address out of bound."));
//      return page_.at(addr - start_addr_);
//    }
//
//    // clear
//    constexpr auto Clear() noexcept -> void { page_.fill(static_cast<page_width>(0x0)); }
//
//    // clear
//    constexpr auto Clear(const size_type begin, decltype(begin) end) noexcept -> std::expected<void, std::domain_error> {
//      using return_t = std::expected<void, std::domain_error>;
//      if (begin == start_addr_ and end == (start_addr_ + page_size - 1)) {
//        Clear();
//        return return_t{};
//      }
//      if (not CheckAddrInrange(begin) or not CheckAddrInrange(end))
//        return std::unexpected(std::domain_error("Address out of bound."));
//      for (size_type i = begin; i <= end; ++i)
//        page_[i - start_addr_] = static_cast<page_width>(0x0);
//      return return_t{};
//    }
//
//
//
//
//
//
//  private:
//    page_model		page_{};
//    bool			in_memory_{};
//    size_type		start_addr_{};
//
//    class Process*	pMaster_{};
//  };
//}
//
//



  //// return page width in # bits
  //NODISCARD constexpr auto get_page_width() const noexcept -> std::size_t { return sizeof(page_width) * CHAR_BIT; }

  //// return page size in length
  //NODISCARD constexpr auto get_page_size() const noexcept -> size_type { return page_.max_size(); }

  //// get start address
  //NODISCARD constexpr auto get_start_addr() const noexcept -> size_type { return start_addr_; }

  //// set start address
  //constexpr void set_start_addr(const size_type start_addr) noexcept { start_addr_ = start_addr; }

  //// get in memory
  //NODISCARD constexpr bool in_memory() const noexcept { return in_memory_; }

  //// set in memory
  //constexpr void set_in_memory(const bool in) noexcept { in_memory_ = in; }

  //// get owning process ptr
  //NODISCARD constexpr auto get_pMaster() const noexcept -> class Process* { return pMaster_; }

  //// set owning process ptr
  //constexpr void set_pMaster(const class Process* p_master) noexcept { pMaster_ = p_master; }


// -------------------------------------------------------
// 
// MainMemory (Thread-safe)
// 
// -------------------------------------------------------


/**
* Design explanation:
* 
* Main memory is consists of an array of Pages. That is, memory OWNs Pages.
* Processes needs to request page from memory and borrow it. After the process is finished,
* pages will be released and returned to the memory.
*/



