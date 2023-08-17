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


    template <typename... Poses>
    concept valid_positions = (std::same_as<std::size_t, Poses> || ...);


    constexpr auto
    setBit(
        std::unsigned_integral auto&            n,
        const std::same_as<std::size_t> auto    position,
        const valid_positions auto...           positions
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
        const valid_positions auto...           positions
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
        const valid_positions auto...           positions
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


namespace misim::page
{
    template <typename SourceType, typename DestType>
    concept valid_forwarding_reference_type = requires
    {
        std::same_as<
            std::remove_reference_t<SourceType>, 
            std::remove_reference_t<DestType>
        >
        && std::is_constructible_v<DestType, SourceType>;
    };


    template <
        std::unsigned_integral Width,
        std::size_t            Size
    >
    class Page
    {
    public:
        using page_width_type = Width;
        using size_type       = std::size_t;
        using page_model_type = std::array<page_width_type, Size>;

        const size_type m_page_size  = Size;
        const size_type m_page_width = sizeof(page_width_type) * CHAR_BIT;

    public:
        [[nodiscard]] constexpr explicit
        Page() = default;

        template <valid_forwarding_reference_type<page_model_type> Array>
        [[nodiscard]] constexpr explicit
        Page(
            Array&& page,
            const bool           in_memory,
            const size_type      start_address,
            const class Process* master_ptr
        )
            : m_page          { std::forward<Array>(page) }
            , m_in_memory     { in_memory       }
            , m_start_address { start_address   }
            , m_master_ptr    { master_ptr      }
        {
        }

    public:
        [[nodiscard]] constexpr inline auto
        checkAddressInrange(const size_type address) noexcept
            -> bool
        {
            return (
                address >= m_start_address
                && address < (m_start_address + m_page_size)
            );
        }

        constexpr auto
        write(
            const page_width_type data,
            const size_type       address
        ) noexcept
            -> std::expected<void, std::domain_error>
        {
            if (!checkAddressInrange(address)) {
                return std::unexpected(
                    std::domain_error("Address out of bound.")
                );
            }

            m_page[address - m_start_address] = std::move(data);

            return {};
        }

        [[nodiscard]] constexpr auto
        read(const size_type address) noexcept
            -> std::expected<page_width_type, std::domain_error>
        {
            if (!checkAddressInrange(address)) {
                return std::unexpected(
                    std::domain_error("Address out of bound.")
                );
            }

            return m_page.at(address - m_start_address);
        }

        constexpr inline auto
        clear() noexcept
            -> void
        {
            m_page.fill(0x0);
        }

        constexpr auto
        clear(
            const size_type begin,
            const size_type end
        ) noexcept
            -> std::expected<void, std::domain_error>
        {
            if (
                begin == m_start_address
                && end == (m_start_address + m_page_size - 1)
                )
            {
                clear();
                return {};
            }

            if (
                !checkAddressInrange(begin)
                || !checkAddressInrange(end)
                )
            {
                return std::unexpected(
                    std::domain_error("Address out of bound.")
                );
            }

            for (size_type i{ begin }; i <= end; ++i) {
                m_page[i - m_start_address] = 0x0;
            }

            return {};
        }

    public:
        [[nodiscard]] constexpr inline auto
        getPageWidth() const noexcept
            -> std::size_t
        {
            return m_page_width;
        }

        [[nodiscard]] constexpr inline auto
        getPageSize() const noexcept
            -> size_type
        {
            return m_page.max_size();
        }

        [[nodiscard]] constexpr inline auto
        getStartAddress() const noexcept
            -> size_type
        {
            return m_start_address;
        }

        constexpr inline auto
        setStartAddress(const size_type start_address) noexcept
            -> void
        {
            m_start_address = start_address;
        }

        [[nodiscard]] constexpr inline auto
        getInMemory() const noexcept
            -> bool
        {
            return m_in_memory;
        }

        constexpr inline auto
        setInMemory(bool in_memory) noexcept
            -> void
        {
            m_in_memory = in_memory;
        }

        [[nodiscard]] constexpr inline auto
        getMasterPtr() const noexcept
            -> class Process*
        {
            return m_master_ptr;
        }

        constexpr inline auto
        setMasterPtr(const class Process* master_ptr) noexcept
            -> void
        {
            m_master_ptr = master_ptr;
        }

    private:
        page_model_type m_page{};

        bool            m_in_memory{};
        size_type       m_start_address{};

        class Process*  m_master_ptr{};
    };
}
