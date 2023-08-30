/*********************************************************************
 *
 *  @file   overture.hpp
 *  @brief  Assembly Abstract machine (Ver.1)
 *
 *  @author Tony (Yetong) Li
 *  @date   August 2023
 *
 *  @detail
 *
 *	Assembly Abstract Machine.
 * 
 *	A very simple, non-pipelined conceptual model of processor,
 *	that runs my own instruction set.
 *
 *********************************************************************/


#include <bit>
#include <map>
#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
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


namespace scsp::memory
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
        [[nodiscard]] constexpr explicit
        Memory() = default;

    public:
        [[nodiscard]] inline constexpr
        auto checkAddressInrange(const memory_size_type address) noexcept
            -> bool
        {
            return address >= 0 && address < m_memory_size;
        }

        constexpr
        auto write(const memory_width_type data, const memory_size_type address) noexcept
            -> std::expected<void, std::domain_error>
        {
            if (!checkAddressInrange(address)) {
                return std::unexpected(
                    std::domain_error("Address out of range.")
                );
            }

            m_memory[address] = std::move(data);
            return {};
        }

        [[nodiscard]] constexpr
        auto read(const memory_size_type address) noexcept
            -> std::expected<memory_width_type, std::domain_error>
        {
            if (!checkAddressInrange(address)) {
                return std::unexpected(
                    std::domain_error("Address out of range.")
                );
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
            -> std::expected<void, std::domain_error>
        {
            if (begin == 0 && end == m_memory_size - 1) {
                clear();
                return {};
            }
            if (!checkAddressInrange(begin) || !checkAddressInrange(end)) {
                return std::unexpected(
                    std::domain_error("Address out of range.")
                );
            }

            for (memory_size_type i{ begin }; i <= end; ++i) {
                m_memory[i] = 0x0;
            }
            return {};
        }

    private:
        memory_model_type m_memory{};
    };
}


namespace scsp::freefuncs
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
        return freefuncs::checkBitInrange<decltype(n)>(position);
    }

    template <std::unsigned_integral uint> [[nodiscard]] static constexpr
    auto testBit(const uint n, const std::size_t position) noexcept
        -> std::expected<bool, std::domain_error>
    {
        if (!freefuncs::checkBitInrange(n, position))
        {
            return std::unexpected(
                std::domain_error("Bit position out of bound.")
            );
        }

        return n & (static_cast<uint>(1) << position);
    }

    [[nodiscard]] static constexpr
    auto testBitAll(const std::unsigned_integral auto n) noexcept
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
            else if (!result)
            {
                return result;
            }
        }
        const auto mask = (static_cast<std::uintmax_t>(1) << n_size) - 1;
        return (n & mask) == mask;
    }

    [[nodiscard]] static constexpr
    auto testBitAll(const std::unsigned_integral auto n, const std::size_t last_nbit) noexcept
        -> std::expected<bool, std::domain_error>
    {
        if (!freefuncs::checkBitInrange(n, last_nbit))
        {
            return std::unexpected(
                std::domain_error("Last nbits out of bound.")
            );
        }
        if (!freefuncs::checkBitInrange(n, last_nbit + 1))
        {
            return freefuncs::testBitAll(n);
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
        -> std::expected<bool, std::domain_error>
    {
        if (!freefuncs::checkBitInrange(n, last_nbit))
        {
            return std::unexpected(
                std::domain_error("Last nbits out of bound.")
            );
        }
        if (!freefuncs::checkBitInrange(n, last_nbit + 1))
        {
            return freefuncs::testBitAny(n);
        }
        const auto mask = (static_cast<std::uintmax_t>(1) << (last_nbit + 1)) - 1;
        return static_cast<bool>(n & mask);
    }

    [[nodiscard]] static inline constexpr
    auto testBitNone(const std::unsigned_integral auto n) noexcept
        -> bool
    {
        return !freefuncs::testBitAny(n);
    }

    [[nodiscard]] static constexpr
    auto testBitNone(const std::unsigned_integral auto n, const std::size_t last_nbit) noexcept
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

    template <std::unsigned_integral uint> static constexpr
    auto setBit(uint& n, const std::size_t position) noexcept
        -> std::expected<void, std::domain_error>
    {
        if (!freefuncs::checkBitInrange<uint>(position))
        {
            return std::unexpected(
                std::domain_error("Bit position out of bound.")
            );
        }

        n |= (static_cast<uint>(1) << position);
        return {};
    }

    template <std::unsigned_integral uint> static inline constexpr
    auto setBit(uint& n) noexcept
        -> std::expected<void, std::domain_error>
    {
        n |= static_cast<uint>(-1);
        return {};
    }

    template <typename... Poses>
    concept valid_positions = requires
    {
        requires
        (std::same_as<std::size_t, Poses> || ...);
    };

    static constexpr
    auto setBit(std::unsigned_integral auto&         n       ,
                const std::same_as<std::size_t> auto position,
                const valid_positions auto...        positions) noexcept
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
        if constexpr (sizeof...(positions) > 0)
        {
            result = freefuncs::setBit(n, positions...);
        }
        return result;
    }

    template <std::unsigned_integral uint> static constexpr
    auto resetBit(uint& n, const std::size_t position) noexcept
        -> std::expected<void, std::domain_error>
    {
        if (!freefuncs::checkBitInrange<uint>(position)) 
        {
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
        else if (!*result) 
        {
            return {};
        }

        n ^= (static_cast<uint>(1) << position);
        return {};
    }

    static inline constexpr
    auto resetBit(std::unsigned_integral auto& n) noexcept
        -> std::expected<void, std::domain_error>
    {
        n &= 0x0;
        return {};
    }

    static constexpr
    auto resetBit(std::unsigned_integral auto&         n       ,
                  const std::same_as<std::size_t> auto position,
                  const valid_positions auto...        positions) noexcept
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
        if constexpr (sizeof...(positions) > 0)
        {
            result = freefuncs::resetBit(n, positions...);
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
        -> std::expected<void, std::domain_error>
    {
        if (!freefuncs::checkBitInrange<uint>(position))
        {
            return std::unexpected(
                std::domain_error("Bit position out of bound.")
            );
        }

        n ^= (static_cast<uint>(1) << position);
        return {};
    }

    static inline constexpr
    auto flipBit(std::unsigned_integral auto& n) noexcept
        -> std::expected<void, std::domain_error>
    {
        n = ~n;
        return {};
    }

    static constexpr
    auto flipBit(std::unsigned_integral auto&         n       ,
                 const std::same_as<std::size_t> auto position,
                 const valid_positions auto...        positions) noexcept
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
        if constexpr (sizeof...(positions) > 0)
        {
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

    template <typename T> [[nodiscard]] static inline constexpr
    auto promote(const T n) noexcept
        -> make_promoted_t<T>
    {
        return n;
    }
}


namespace scsp::register_file
{
    enum GPReg : std::uint8_t
    {
        R0 = 0, //  \ 
        R1,     //   |
        R2,     //   |
        R3,     //   |
        R4,     //   |
        R5,     //   |  General
        R6,     //   |  Purpose
        R7,     //   |  Registers
        R8,     //   |
        R9,     //   |
        R10,    //   |
        R11,    //   |
        R12,    //  /

        SP,     //      Stack Pointer
        LR,     //      Link Register
        PC      //      Program Counter
    };

    enum SEGReg : std::uint8_t
    {
        CS,		//      Code Segment
        DS,		//      Data Segment
        SS,		//      Stack Segment
        ES		//      Extra Segment
    };

    enum PSRReg : std::uint8_t
    {
        N,	    //      Negative or less than flag
        Z,	    //      Zero flag
        C,	    //      Carry/borrow flag
        V	    //      Overflow flag
    };

    template <std::unsigned_integral uint>
    class Registers
    {
    public:
        using PSR_width_type = std::uint8_t;
        using GP_width_type  = uint;
        using PSR_model_type = PSR_width_type;
        using GP_model_type  = std::array<GP_width_type, 16>;

    public:
        [[nodiscard]] constexpr explicit
        Registers() noexcept = default;

    public:
        [[nodiscard]] inline constexpr
        auto getGeneralPurpose(const std::uint8_t register_name) noexcept
            -> GP_width_type&
        {
            return m_gp[register_name];
        }

        [[nodiscard]] inline constexpr
        auto getGeneralPurpose(const std::uint8_t register_name) const noexcept
            -> GP_width_type
        {
            return m_gp.at(register_name);
        }

        [[nodiscard]] inline constexpr
        auto getProgramStatus(const std::uint8_t flag_name) const noexcept
            -> bool
        {
            namespace ff = ::scsp::freefuncs;
            return ff::testBit<PSR_width_type>(m_psr, flag_name).value();
        }

        constexpr
        auto setProgramStatus(const std::uint8_t flag_name, const bool value) noexcept
            -> std::expected<bool, std::runtime_error>
        {
            namespace ff = ::scsp::freefuncs;

            std::expected<void, std::domain_error> result = (value) ? ff::setBit  <PSR_width_type>(m_psr, flag_name) :
                                                                      ff::resetBit<PSR_width_type>(m_psr, flag_name) ;
            if (!result)
            {
                return std::unexpected(
                    std::runtime_error("setBit / resetBit failed")
                );
            }
            return {};
        }

        [[nodiscard]] inline constexpr
        auto getPsrValue() const noexcept
            -> PSR_width_type
        {
            return m_psr;
        }

        inline constexpr
        auto clearPsrValue() noexcept
            -> void
        {
            m_psr = 0x0;
        }

    private:
        GP_model_type  m_gp{};
        PSR_model_type m_psr{};
    };
}


namespace scsp::ALU
{
    /**
    * ALU is a black box, it takes in a input, and spit the output;
    *						   ______________
    *						  |				 |
    *	Operands -----------> |		ALU		 | -------------> Output
    *						  |______________|
    *
    */

    enum struct ALUOpCode : std::uint8_t 
    {
                //      Arithmetic operations
        ADD,	//      Add two operands
        UMUL,	//      Multiple A & B
        UDIV,	//      Divide A & B
        UMOL,	//      Take the modulus (%)
        PASS,	//      Pass through one operand without change

                //      Bitwise logical operations
        AND,	//      Bitwise And of A & B
        ORR,	//      Bitwise Or of A & B
        XOR,	//      Bitwise Xor of A & B
        COMP,	//      One's compliment of A / B

                //      Bit shift operations
        SHL,	//      Logical shift left
        SHR,	//      Logical shift right
        RTL,	//      Rotate left
        RTR,	//      Rotate right
    };

    template <std::unsigned_integral Width>
    struct ALUInput
    {
        using operand_width_type = Width;

        ALUOpCode               m_opcode;       // the Opcode the ALU will operate in
        std::pair<Width, Width> m_operands;     // A & B two operands
    };

    template <std::unsigned_integral Width>
    struct ALUOutput
    {
        using operand_width_type = Width;
        using updated_flags_type = std::unordered_set<std::uint8_t>;

        updated_flags_type m_flags;             // The PSR flags the operation produces
        operand_width_type m_result;            // The result of the operation
    };

    template <typename Function, typename Param>
    concept valid_callback_function = requires
    {
        requires
        std::is_invocable_r_v<bool, Function, Param, Param, Param>;
    };

    class ALU
    {
    public:
        template <std::unsigned_integral Width> [[nodiscard]] static constexpr
        auto execute(ALUInput<Width> alu_input) noexcept
            -> ALUOutput<Width>
        {
            using enum ::scsp::ALU::ALUOpCode;
            using operand_width_type = typename ALUInput<Width>::operand_width_type;

            std::pair<operand_width_type, operand_width_type> AB = alu_input.m_operands;

            switch (alu_input.m_opcode)
            {
                case ADD :  { return add (AB.first, AB.second); }
                case UMUL:  { return umul(AB.first, AB.second); }
                case UDIV:  { return udiv(AB.first, AB.second); }
                case UMOL:  { return umol(AB.first, AB.second); }
                case PASS:  { return pass(AB.first           ); }

                case AND :  { return and_(AB.first, AB.second); }
                case ORR :  { return orr (AB.first, AB.second); }
                case XOR :  { return xor_(AB.first, AB.second); }
                case COMP:  { return comp(AB.first           ); }

                case SHL :  { return shl (AB.first, AB.second); }
                case SHR :  { return shr (AB.first, AB.second); }
                case RTL :  { return rtl (AB.first, AB.second); }
                case RTR :  { return rtr (AB.first, AB.second); }
            }

            return ALUOutput<Width>{};
        }

    private:
        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto getProgramStatusFlags(const T R, std::unordered_set<std::uint8_t>& flags) noexcept
            -> void
        {
            // get program status flags from result only
            // if the first bit (sign bit) is set, then set Neg flag;
            namespace ff = ::scsp::freefuncs;
            using enum ::scsp::register_file::PSRReg;

            if (ff::testBit<T>(R, sizeof(T) * CHAR_BIT - 1).value())
            {
                flags.insert(N);
            }
            // if the result is equal to 0, then set Zero flag;
            if (ff::testBitNone(R) && R == 0x0) 
            {
                flags.insert(Z);
            }
        }

        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto getProgramStatusFlags(const T R) noexcept
            -> std::unordered_set<std::uint8_t>
        {
            std::unordered_set<std::uint8_t> flags{};
            getProgramStatusFlags<T>(R, flags);
            return flags;
        }

        template <std::unsigned_integral     T      ,
                  valid_callback_function<T> Function> [[nodiscard]] static constexpr
        auto getProgramStatusFlags(const T A, const T B, const T R,
                                   Function checkCarry, Function checkOverflow) noexcept
            -> std::unordered_set<std::uint8_t>
        {
            using enum ::scsp::register_file::PSRReg;

            std::unordered_set<std::uint8_t> flags{};
            if (checkCarry(A, B, R)) 
            {
                flags.insert(C);
            }
            if (checkOverflow(A, B, R)) 
            {
                flags.insert(V);
            }

            // Neg and Zero delegate to other overload
            getProgramStatusFlags<T>(R, flags);
            return flags;
        }

        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto makeOutput(const T R) noexcept
            -> ALUOutput<T>
        {
            std::unordered_set<std::uint8_t> flags = getProgramStatusFlags<T>(R);
            return ALUOutput<T>{
                .m_flags = std::move(flags),
                .m_result = R
            };
        }

        // 1. ADD
        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto add(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            const T R = A + B;

            auto checkCarry = [](const T A, const T B, const T R) noexcept
                -> bool
            {
                return (R < A && R < B) ? true : false;
            };
            auto checkOverflow = [](const T A, const T B, const T R) noexcept
                -> bool
            {
                std::size_t msb = sizeof(T) * CHAR_BIT - 1;
                namespace ff = ::scsp::freefuncs;
                // if A & B's msb differ, no overflow
                if (ff::testBit<T>(A, msb).value() ^ ff::testBit<T>(B, msb).value()) {
                    return false;
                }
                // check if A | B's msb differ from R's msb
                return (ff::testBit<T>(A, msb).value() ^ ff::testBit<T>(R, msb).value()) ? true : false;
            };

            using flag_type = std::unordered_set<std::uint8_t>;
            flag_type flags = getProgramStatusFlags<T, std::function<bool(T, T, T)>>(
                A, B, R,
                checkCarry, checkOverflow
            );

            return ALUOutput<T> {
                .m_flags = std::move(flags),
                .m_result = R
            };
        }

        // 2. UMUL
        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto umul(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            /// EXPERIMENTAL! NO CV FLAGS WILL BE DETECTED! ///
            
            using ::scsp::freefuncs::promote;
            return makeOutput<T>(static_cast<T>(promote<T>(A) * promote<T>(B)));
        }

        // 3. UDIV
        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto udiv(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            using ::scsp::freefuncs::testBitNone;

            if (testBitNone(B) && B == 0x0) {
                return ALUOutput<T>{};
            }
            return makeOutput<T>(A / B);
        }

        // 4. UMOL
        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto umol(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            using ::scsp::freefuncs::testBitNone;

            if (testBitNone(B) && B == 0x0) {
                return ALUOutput<T>{};
            }
            return makeOutput<T>(A % B);
        }

        // 5. PASS
        template <std::unsigned_integral T> [[nodiscard]] static inline constexpr
        auto pass(const T A) noexcept
            -> ALUOutput<T>
        {
            return makeOutput<T>(A);
        }

        // 6. AND
        template <std::unsigned_integral T> [[nodiscard]] static inline constexpr
        auto and_(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            return makeOutput<T>(A & B);
        }

        // 7. ORR
        template <std::unsigned_integral T> [[nodiscard]] static inline constexpr
        auto orr(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            return makeOutput<T>(A | B);
        }

        // 8. XOR
        template <std::unsigned_integral T> [[nodiscard]] static inline constexpr
        auto xor_(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            return makeOutput<T>(A ^ B);
        }

        // 9. COMP
        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto comp(T A) noexcept
            -> ALUOutput<T>
        {
            using ::scsp::freefuncs::flipBit;
            std::expected<void, std::domain_error> result = flipBit(A);

            return (result) ? makeOutput<T>(A) :
                              ALUOutput<T>{}   ;
        }

        // 10. SHL
        template <std::unsigned_integral T> [[nodiscard]] static inline constexpr
        auto shl(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            return makeOutput<T>(A << B);
        }

        // 11. SHR
        template <std::unsigned_integral T> [[nodiscard]] static inline constexpr
        auto shr(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            return makeOutput<T>(A >> B);
        }

        // 12. RTL
        template <std::unsigned_integral T> [[nodiscard]] static inline constexpr
        auto rtl(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            return makeOutput<T>(std::rotl<T>(A, static_cast<int>(B)));
        }

        // 13. RTR
        template <std::unsigned_integral T> [[nodiscard]] static inline constexpr
        auto rtr(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            return makeOutput<T>(std::rotr<T>(A, static_cast<int>(B)));
        }
    };
}


namespace scsp::decode
{
    enum OpType : std::uint8_t
    {
        Rt,     //  R_type: 2 source registers, 1 dest registers, no imm; Binary operations
                //  ADD Rd, Rs, Rm ; MUL Rd, Rs, Rm

        It,     //  I_type: 1 source register, 1 dest register, 1 imm; Binary operations, with imm
                //  ADD Rd, Rs, imm

        Ut,     //  U_type: 1 source register, 1 dest register, 0 imm; Unary operations
                //  NEG Rd, Rs ; NOT Rd, Rs ; LDR Rd, Rs ; STR Rd, Rs

        St,     //  S_type: 0 source register, 1 dest register, 0 imm (only for stack operations)
                //  PUSH Rd ; POP Rd

        Jt      //  J_type: 0 source register, 0 dest register 1 imm (only for branching)
                //  JMP imm ; JEQ imm
    };

    enum OpCode : std::uint8_t
    {
        // Opcode	//  Explanation			    //		Example					//		Semantics
        ADD,		//  add two operands		//		ADD  R1, R2, R3/imm		//		R1 <- R2 + R3/imm
        UMUL,		//  multiple two operands	//		UMUL R1, R2, R3/imm		//		R1 <- R2 * R3/imm
        UDIV,		//  divide two operands		//		UDIV R1, R2, R3/imm		//		R1 <- R2 / R3/imm
        UMOL,		//		op1 % op2			//		UMOL R1, R2, R3/imm		//		R1 <- R2 % R3/imm

        AND,		//  bitwise And of A & B	//		AND  R1, R2, R3/imm		//		R1 <- R2 & R3/imm
        ORR,		//  bitwise Or of A & B		//		ORR  R1, R2, R3/imm		//		R1 <- R2 | R3/imm
        XOR,		//  bitwise Xor of A & B	//		XOR  R1, R2, R3/imm		//		R1 <- R2 ^ R3/imm
        SHL,		//  logical shift left		//		SHL  R1, R2, R3/imm		//		R1 <- R2 << R3/imm
        SHR,		//  logical shift right		//		SHR  R1, R2, R3/imm		//		R1 <- R2 >> R3/imm
        RTL,		//  logical rotate left		//		RTL  R1, R2, R3/imm		//		R1 <- R2 <~ R3/imm
        RTR,		//  logical rotate right	//		RTR  R1, R2, R3/imm		//		R1 <- R2 ~> R3/imm

        NOT,		//  comp all the bits		//		NOT  R1, R2				//		R1 <- ~R2

        LDR,		//  load reg from mem		//		LDR  R1, R2				//		R1 <- [R2]
        STR,		//  store reg in mem		//		STR  R1, R2				//		[R1] <- R2

        PUSH,		//  push reg onto stack		//		PUSH R1					//		[SP - 4] <- R1;
        POP,		//  pop top ele into reg	//		POP	 R1					//		R1 <- [SP] + 4

        JMP,		//	unconditional jump		//		JMP label				//		N/A
        JZ,			//  jump if Z flag is set	//		JZ	label				//		N/A
        JN,			//  jump if N flag is set	//		JN	label				//		N/A
        JC,			//  jump if C flag is set	//		JC	label				//		N/A
        JV,			//  jump if V flag is set	//		JV	label				//		N/A
        JZN,		//  jump if Z or N is set	//		JZN label				//		N/A

        SYSCALL		//  invokes system calls	//		SYSCALL 1				//		N/A
    };

    // Basic layout:
    // 1 0 9 8   7 6 5 4   3 2 1 0   9 8 7 6   5 4 3 2   1 0 9 8   7 6 5 4   3 2 1 0
    // 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0
    //					   \____/    \____/    \_____/   \_______________/   \_____/
    // \_____________________|_/	   |		  |				  |				|
    //			imm			 Rn		   Rm		  Rd			 OpCode		  OpType

    struct DefaultEncoding final
    {
        using min_width_type = std::uint32_t;
        using value_type     = std::uint8_t;

        template <std::unsigned_integral T>
        struct Field
        {
            T m_start_idx;
            T m_length;
        };

        using field_type = Field<value_type>;

        static constexpr field_type m_op_type { .m_start_idx = 0,  .m_length = 4  };
        static constexpr field_type m_op_code { .m_start_idx = 4,  .m_length = 8  };
        static constexpr field_type m_Rd      { .m_start_idx = 12, .m_length = 4  };
        static constexpr field_type m_Rm      { .m_start_idx = 16, .m_length = 4  };
        static constexpr field_type m_Rn      { .m_start_idx = 20, .m_length = 4  };
        static constexpr field_type m_imm     { .m_start_idx = 20, .m_length = 12 };
    };

    template <typename EncodingRule>
    concept valid_encoding_rule = requires
    {
        typename EncodingRule::min_width_type;
        typename EncodingRule::value_type;
        typename EncodingRule::field_type;

        requires
        std::convertible_to<typename EncodingRule::value_type, std::uint32_t>;

        EncodingRule::m_op_type;
        EncodingRule::m_op_code;
        EncodingRule::m_Rd;
        EncodingRule::m_Rm;
        EncodingRule::m_Rn;
        EncodingRule::m_imm;
    };

    template <valid_encoding_rule EncodingRule>
    struct Instruction
    {
    private:
        using u8  = std::uint8_t;
        using u16 = std::uint16_t;
        using u32 = std::uint32_t;

        template <typename EncodingRule::value_type Size>
        using field_size_type = std::conditional_t<(Size <= sizeof(u8) * CHAR_BIT), u8,
                                std::conditional_t<(Size <= sizeof(u16) * CHAR_BIT), u16, u32>>;

        static_assert(
            EncodingRule::m_Rd.m_length == EncodingRule::m_Rm.m_length &&
            EncodingRule::m_Rm.m_length == EncodingRule::m_Rn.m_length
        );

    public:
        using optype_type    = field_size_type<EncodingRule::m_op_type.m_length>;
        using opcode_type    = field_size_type<EncodingRule::m_op_code.m_length>;
        using register_type  = field_size_type<EncodingRule::m_Rd.m_length>;
        using immediate_type = field_size_type<EncodingRule::m_imm.m_length>;

        optype_type    m_type;
        opcode_type    m_code;
        register_type  m_Rd, m_Rn, m_Rm;
        immediate_type m_imm;
    };

    template <valid_encoding_rule EncodingRule>
    class Decoder
    {
    public:
        using encoding_type    = EncodingRule;
        using field_type       = typename encoding_type::field_type;
        using instruction_type = Instruction<encoding_type>;

        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto decode(T instruction) noexcept
            -> instruction_type
        {
            static_assert(sizeof(T) >= sizeof(typename encoding_type::min_width_type),
                          "Instruction length smaller than minimum encoding length.");

            return instruction_type{
                .m_type = extract<T, typename instruction_type::optype_type>   (instruction, encoding_type::m_op_type),
                .m_code = extract<T, typename instruction_type::opcode_type>   (instruction, encoding_type::m_op_code),

                .m_Rd   = extract<T, typename instruction_type::register_type> (instruction, encoding_type::m_Rd     ),
                .m_Rn   = extract<T, typename instruction_type::register_type> (instruction, encoding_type::m_Rn     ),
                .m_Rm   = extract<T, typename instruction_type::register_type> (instruction, encoding_type::m_Rm     ),

                .m_imm  = extract<T, typename instruction_type::immediate_type>(instruction, encoding_type::m_imm    )
            };
        }

    private:
        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto makeMask(const std::size_t mask_length) noexcept
            -> T
        {
            using ::scsp::freefuncs::setBit;

            T mask = 0x0;
            setBit<T>(mask);

            if (mask_length > sizeof(T) * CHAR_BIT)
            {
                return mask;
            }
            return mask >> (sizeof(T) * CHAR_BIT - mask_length);
        }

        template <std::unsigned_integral T        ,
                  std::unsigned_integral Conversion> [[nodiscard]] static constexpr
        auto extract(T instruction, field_type field) noexcept
            -> Conversion
        {
            instruction >>= field.m_start_idx;
            return static_cast<Conversion>(instruction & makeMask<T>(field.m_length));
        }
    };
}


namespace scsp::syscall
{
    template <typename Object>
    using lref_type = std::add_lvalue_reference_t<Object>;

    struct SyscallTable
    {
        template <typename Memory, typename Register>
        inline static const std::unordered_map<std::uint32_t,
                                               std::function<void(lref_type<Memory>, lref_type<Register>)>>
        m_syscall_table = {
            {0, SyscallTable::template welcome   <Memory, Register>},
            {1, SyscallTable::template consoleOut<Memory, Register>},
            {2, SyscallTable::template consoleIn <Memory, Register>}
        };

    private:
        template <typename Memory, typename Register> static constexpr
        auto welcome(lref_type<Memory>, lref_type<Register>) noexcept
            -> void
        {
            printf(
                "Welcome stranger!\n\n"
                "This is the CPU speaking - I'm glad that you found this eastern egg left by my creator.\n"
                "If you see this message, it means that you must be browsing through the code or experimenting with me.\n"
                "I hope you have the same enthusiasm with C++ as my creator does - because enthusiasm is the "
                "most important thing in the world.\n\n"
                "Well, wish you a good day. Bye, adios!\n"
            );
        }

        /*
         * Doc-string:
         * The console_out() function allow user to print text on terminal.
         * @inputs:
         *	- R0: the starting addr of a contigious memory (string) that will be displayed;
         *	- R1: the length of contigious memory;
         * @outputs:
         *	- None
         */
        template <typename Memory, typename Register> static
        auto consoleOut(lref_type<Memory> memory, lref_type<Register> registers)
            -> void
        {
            using enum ::scsp::register_file::GPReg;

            std::string temp{};
            temp.reserve(registers.getGeneralPurpose(R1));

            for (
                 auto i = registers.getGeneralPurpose(R0);
                 i < (registers.getGeneralPurpose(R0) + registers.getGeneralPurpose(R1));
                 ++i
                )
            {
                if (
                    auto result = memory.read(i);
                    result
                    )
                {
                    temp.push_back(*result);
                }
                else {
                    throw std::out_of_range("Memory access out of range.");
                }
            }

            std::cout << temp;
        }

        /*
         * Doc-string:
         * The console_in() function allow user to input string with keyboard.
         * @inputs:
         *	- R0: the starting addr of a contigious memory that will be filled;
         *	- R1: the length of contigious memory;
         * @outputs:
         *	- User input string will be placed in the contigious memory. All spaces will be rewritten.
         *	  Thus please make sure to save necessary contents on the stack before doing the operation.
         */
        template <typename Memory, typename Register> static
        auto consoleIn(lref_type<Memory> memory, lref_type<Register> registers)
            -> void
        {
            using enum ::scsp::register_file::GPReg;

            std::string temp;
            std::getline(std::cin, temp);

            if (temp.length() > registers.getGeneralPurpose(R1))
            {
                throw std::length_error("User-input string exceeds maximum space length.");
            }

            std::ranges::for_each(
                temp,
                [i = registers.getGeneralPurpose(R0), &memory](auto character) mutable noexcept {
                    memory.write(character, i);
                    ++i;
                }
            );
        }
    };
}


namespace scsp::tracer
{
    template <typename Exception>
    concept valid_exception = requires
    {
        requires
        std::derived_from<Exception, std::exception>;
    };

    template <typename Object>
    using const_lref_type = const std::add_lvalue_reference_t<Object>;

    class Tracer
    {
    public:
        enum struct CriticalLvls : std::int8_t
        {
            INFO,
            WARNING,
            ERROR
        };

        struct Labels
        {
            using enum ::scsp::register_file::GPReg;
            using enum ::scsp::register_file::SEGReg;
            using enum ::scsp::register_file::PSRReg;
            using enum ::scsp::decode::OpCode;
            using enum ::scsp::decode::OpType;

            using gp_translation_type  = const std::map<::scsp::register_file::GPReg,            std::string_view>;
            using psr_translation_type = const std::unordered_map<::scsp::register_file::PSRReg, char            >;
            using seg_translation_type = const std::unordered_map<::scsp::register_file::SEGReg, std::string_view>;
            using opc_translation_type = const std::unordered_map<::scsp::decode::OpCode,        std::string_view>;
            using opt_translation_type = const std::unordered_map<::scsp::decode::OpType,        std::string_view>;

            static inline gp_translation_type gp_translation = {
                { R0, "R0"   },   { R1, "R1" },   { R2, "R2"   },   { R3, "R3"   },
                { R4, "R4"   },   { R5, "R5" },   { R6, "R6"   },   { R7, "R7"   },
                { R8, "R8"   },   { R9, "R9" },   { R10, "R10" },   { R11, "R11" },
                { R12, "R12" },   { SP, "SP" },   { LR, "LR"   },   { PC, "PC"   }
            };

            static inline psr_translation_type psr_translation = {
                { N, 'N' },
                { Z, 'Z' },
                { C, 'C' },
                { V, 'V' }
            };

            static inline seg_translation_type seg_translation = {
                { CS, "Code Segment"  },
                { DS, "Data Segment"  },
                { SS, "Stack Segment" },
                { ES, "Extra Segment" }
            };

            static inline opt_translation_type opt_translation = {
                { Rt, "R type" },
                { It, "I type" },
                { Ut, "U type" },
                { St, "S type" },
                { Jt, "J type" }
            };

            static inline opc_translation_type opc_translation = {
                { ADD, "ADD" }, { UMUL, "UMUL" }, { UDIV, "UDIV" }, { UMOL, "UMOL" },

                { AND, "AND" }, { ORR, "ORR" }, { XOR, "XOR" }, { SHL, "SHL" },
                { SHR, "SHR" }, { RTL, "RTL" }, { RTR, "RTR" }, { NOT, "NOT" },

                { LDR, "LDR" }, { STR, "STR" }, { PUSH, "PUSH" }, { POP, "POP" },

                { JMP, "JMP" }, { JZ, "JZ" }, { JN, "JN" }, { JC, "JC" }, { JV, "JV" }, { JZN, "JZN" },

                { SYSCALL, "SYSCALL" }
            };
        };

    public:
        [[nodiscard]] explicit
        Tracer(const std::filesystem::path& log_path)
            : m_log_file{ std::filesystem::absolute(log_path).c_str() }
            , m_instruction_count{ 0 }
        {
            if (!m_log_file.is_open())
            {
                throw std::filesystem::filesystem_error(
                    "Failed to create the log file.",
                    std::error_code(1, std::system_category())
                );
            }
        }

    public:
        template <Tracer::CriticalLvls Level, valid_exception Exception>
        auto log(const std::string_view& message)
            -> void
        {
            auto getPrefix = [](Tracer::CriticalLvls level)
                -> std::string_view
            {
                using enum ::scsp::tracer::Tracer::CriticalLvls;
                switch (level)
                {
                    case INFO:      return "INFO: "   ;
                    case WARNING:   return "WARNING: ";
                    case ERROR:     return "ERROR: "  ;
                    default:        throw std::runtime_error("Unknown critical level.");
                }
            };

            m_log_file << getPrefix(Level) << message << '\n';

            if (Level == Tracer::CriticalLvls::ERROR)
            {
                m_log_file.close();
                throw Exception(message.data());
            }
        }

        template <std::unsigned_integral T             ,
                  typename Instruction, typename Memory,
                  typename Register   , typename Segment>
        auto generateTrace(const T binary,
                           const_lref_type<Instruction> instruction, const_lref_type<Memory>  memory,
                           const_lref_type<Register>    registers  , const_lref_type<Segment> segments) noexcept
            -> void
        {
            m_log_file << getHeading(binary)
                       << getInstruction<Instruction>(instruction)
                       << getRegister<Register>(registers)
                       << getMemory<Memory, Segment>(memory, segments)
                       << '\n';

            ++m_instruction_count;
        }

    private:
        template <typename Register>
        auto getRegister(const_lref_type<Register> registers) noexcept
            -> std::string
        {
            std::stringstream ss_label{};
            std::stringstream ss_value{};

            std::ranges::for_each(
                Labels::gp_translation,
                [&](const auto p) {
                    ss_label << p.second << ',';
                    ss_value << registers.getGeneralPurpose(p.first) << ',';
                }
            );
            formatter(ss_label, ss_value);

            std::ranges::for_each(
                Labels::psr_translation,
                [&](const auto p) {
                    ss_label << p.second << ',';
                    ss_value << registers.getProgramStatus(p.first) << ',';
                }
            );
            formatter(ss_label, ss_value);

            return ss_label.str();
        }

        template <typename Memory, typename Segment>
        auto getMemory(const_lref_type<Memory> memory, const_lref_type<Segment> segments)
            -> std::string
        {
            std::stringstream ss_segments{};
            std::stringstream ss_values{};

            for (const auto& p : segments)
            {
                try
                {
                    ss_segments << Labels::seg_translation.at(static_cast<::scsp::register_file::SEGReg>(p.first));
                }
                catch (const std::out_of_range&)
                {
                    log<CriticalLvls::ERROR, std::out_of_range>("No corresponding segment translation.");
                }

                for (auto i = p.second.m_start; i <= p.second.m_end; ++i) {
                    ss_values << static_cast<std::uint32_t>(memory.read(i).value()) << ',';
                }

                formatter(ss_segments, ss_values);
            }

            return ss_segments.str();
        }

        template <typename Instruction>
        auto getInstruction(const_lref_type<Instruction> instruction)
            -> std::string
        {
            std::stringstream ss_label{};
            std::stringstream ss_value{};

            constexpr std::array<std::string_view, 6> labels{
                "OpType", "OpCode", "Rd", "Rm", "Rn", "Imm"
            };
            std::ranges::for_each(
                labels,
                [&ss_label](const auto v) { ss_label << v << ','; }
            );
            ss_label << '\n';

            const std::array<typename Instruction::register_type, 3> registers{
                instruction.m_Rd, instruction.m_Rm, instruction.m_Rn
            };
            try
            {
                ss_value << Labels::opt_translation.at(static_cast<::scsp::decode::OpType>(instruction.m_type)) << ',';
                ss_value << Labels::opc_translation.at(static_cast<::scsp::decode::OpCode>(instruction.m_code)) << ',';
                std::ranges::for_each(
                    registers,
                    [&ss_value](const auto v) {
                        ss_value << Labels::gp_translation.at(static_cast<::scsp::register_file::GPReg>(v)) << ',';
                    }
                );
            }
            catch (const std::out_of_range&)
            {
                log<CriticalLvls::ERROR, std::out_of_range>("No corresponding instruction translation.");
            }

            ss_value << static_cast<std::uint32_t>(instruction.m_imm) << '\n';
            ss_label << ss_value.str();

            return ss_label.str();
        }

        template <std::unsigned_integral T>
        auto getHeading(const T binary) const noexcept
            -> std::string
        {
            std::stringstream ss;
            ss << "Instruction #" << m_instruction_count
               << ", 0x"
               << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex
               << binary
               << '\n';
            
            return ss.str();
        }

        static
        auto formatter(std::stringstream& label, std::stringstream& value) noexcept
            -> void
        {
            label << '\n';
            value << '\n';
            label << value.str();
            value.str(std::string{});
        }

    private:
        std::ofstream m_log_file;
        std::uint32_t m_instruction_count;
    };
}


namespace scsp::core
{
    /*
     * Memory Layout
     * 
     *  _________________  <----------- Top
     * |_________________|
     * |				 | \
     * |  Extra Segment  | | 64KB (magic fields :)
     * |_________________| /<----------- ES reg
     * |_________________|
     * |				 | \
     * |			     | |
     * |  Stack Segment  | | 64KB
     * |				 | |
     * |				 | |
     * |_________________| / <----------- SS reg
     * |_________________|
     * |				 | \
     * |   Data Segment  | | 64KB
     * |				 | |
     * |_________________| / <----------- DS reg
     * |_________________|
     * |				 | \
     * |   Code Segment  | | 64KB
     * |_________________| / <---------- CS reg
     * |_________________| <----------- Bottom (vector table or nullptr)
     */

    template <typename SystemBit, typename Data, typename... Datas>
    concept valid_data = requires
    {
        requires
        std::convertible_to<Data, SystemBit> && (std::same_as<Data, Datas> || ...);
    };

    template <typename SourceType, typename DestType>
    concept valid_forwarding_reference_type = requires
    {
        requires
        std::same_as<std::remove_reference_t<SourceType>,
                     std::remove_reference_t<DestType>   >;
        requires
        std::is_constructible_v<DestType, SourceType>;
    };

    namespace mm = ::scsp::memory;
    namespace rf = ::scsp::register_file;
    namespace dc = ::scsp::decode;
    namespace au = ::scsp::ALU;
    namespace tr = ::scsp::tracer;

    template <std::unsigned_integral SystemBit,
              std::size_t MemorySize,
              typename SyscallTable>
    class Core
    {
    public:
        using system_bit_type         = SystemBit;
        const std::size_t memory_size = MemorySize;

        using memory_type      = mm::Memory<system_bit_type, MemorySize>;
        using register_type    = rf::Registers<system_bit_type>;
        using decoder_type     = dc::Decoder<dc::DefaultEncoding>;
        using alu_type         = au::ALU;

        using instruction_type = typename decoder_type::instruction_type;
        using alu_in_type      = au::ALUInput<system_bit_type>;
        using alu_out_type     = au::ALUOutput<system_bit_type>;

        struct SegmentRange
        {
            using value_type = std::uint32_t;

            value_type m_start;
            value_type m_end;
        };

        using segment_type = std::unordered_map<rf::SEGReg, Core::SegmentRange>;
        using syscall_type = SyscallTable;

    public:
        template <valid_forwarding_reference_type<segment_type> Segment> [[nodiscard]] constexpr explicit
        Core(Segment&& segment_config)
            : m_tracer{ nullptr }
        {
            if (!initialize(std::forward<Segment>(segment_config)))
            {
                traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>("Failed to initialize segment.");
            }
        }

        template <valid_forwarding_reference_type<segment_type> Segment> [[nodiscard]] constexpr explicit
        Core(Segment&& segment_config, tr::Tracer* p_tracer)
            : m_tracer{ p_tracer }
        {
            if (!initialize(std::forward<Segment>(segment_config)))
            {
                traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>("Failed to initialize segment.");
            }
        }

    public:
        template <typename Instruction, typename... Instructions> 
            requires (valid_data<system_bit_type, Instruction, Instructions...>) constexpr
        auto instructionLoader(Instruction in, Instructions... ins) noexcept
            -> void
        {
            using enum rf::SEGReg;
            recursiveLoaderHelper(m_segment.at(CS).m_start, m_segment.at(CS).m_end, in, ins...);
        }

        template <typename Data, typename... Datas>
            requires (valid_data<system_bit_type, Data, Datas...>) constexpr
        auto dataLoader(Data da, Datas... das) noexcept
            -> void
        {
            using enum rf::SEGReg;
            recursiveLoaderHelper(m_segment.at(DS).m_start, m_segment.at(DS).m_end, da, das...);
        }

        constexpr
        auto run()
            -> void
        {
            using ::scsp::freefuncs::testBitAll;

            for (;;)
            {
                auto binary = fetch();

                if (testBitAll(binary).value())
                {
                    break;
                }

                instruction_type instruction = decoder_type::decode(binary);

                if (checkJump(instruction))
                {
                    generateTrace(binary, instruction);
                    continue;
                }

                auto result = execute(instruction);

                memoryAccess(instruction, result);

                generateTrace(binary, instruction);
            }
        }

    private:
        template <valid_forwarding_reference_type<segment_type> Segment> constexpr
        auto initialize(Segment&& segment_config) noexcept
            -> bool
        {
            using enum rf::SEGReg;
            using enum rf::GPReg;
            using enum rf::SEGReg;

            constexpr std::array<rf::SEGReg, 4> segment_registers{ CS, DS, SS, ES };
            if (
                !std::ranges::all_of(segment_registers, 
                                     [&segment_config](rf::SEGReg sr) { return segment_config.contains(sr); })
                )
            {
                return false;
            }
            
            std::vector<Core::SegmentRange> ranges{};
            for (auto&& r : segment_config)
            {
                ranges.emplace_back(r.second);
            }

            if (
                !std::ranges::all_of(ranges,
                                     [this](const Core::SegmentRange rng) {
                                        return rng.m_end >= rng.m_start && rng.m_start >= 0 && rng.m_end < this->memory_size;
                                     })
                )
            {
                return false;
            }

            std::ranges::sort(
                ranges,
                std::ranges::less{},
                [](const Core::SegmentRange rng) { return rng.m_start; }
            );

            auto checkOverlap = [](const Core::SegmentRange rng1, const Core::SegmentRange rng2)
                -> bool
            {
                return rng1.m_start <= rng2.m_end && rng2.m_start <= rng1.m_end;
            };
            for (auto iter = ranges.cbegin() + 1; iter != ranges.cend(); ++iter)
            {
                if (checkOverlap(*(iter - 1), *iter)) {
                    return false;
                }
            }

            auto acc = [](system_bit_type&& i, const Core::SegmentRange rng)
                -> system_bit_type
            {
                return i + (rng.m_end - rng.m_start) + 1;
            };
            if (
                system_bit_type result = std::accumulate(ranges.begin(), ranges.end(), 0, acc);
                result > memory_size
                )
            {
                return false;
            }

            m_segment = std::forward<Segment>(segment_config);

            m_register.getGeneralPurpose(SP) = m_segment.at(SS).m_end + 1;
            m_register.getGeneralPurpose(PC) = m_segment.at(CS).m_start;

            return true;
        }

        template <tr::Tracer::CriticalLvls Level, tr::valid_exception Exception> constexpr
        auto traceLog(const std::string_view& message) const
            -> void
        {
            if (m_tracer != nullptr)
            {
                m_tracer->log<Level, Exception>(message);
                return;
            }
            if (Level == tr::Tracer::CriticalLvls::ERROR)
            {
                throw Exception(message.data());
            }
        }

        constexpr
        auto generateTrace(system_bit_type binary, instruction_type& instruction) noexcept
            -> void
        {
            if (m_tracer != nullptr)
            {
                m_tracer->generateTrace<system_bit_type, instruction_type, memory_type, register_type, segment_type>(
                    binary, instruction, m_memory, m_register, m_segment
                );
            }
        }

        template <std::unsigned_integral T> [[nodiscard]] inline static constexpr
        auto checkAddressInrange(const T address, const Core::SegmentRange segment) noexcept
            -> bool
        {
            return address >= segment.m_start && address <= segment.m_end;
        }

        constexpr
        auto updatePsr(const typename alu_out_type::updated_flags_type& updated_flags) noexcept
            -> void
        {
            m_register.clearPsrValue();

            for (auto&& flag : updated_flags)
            {
                m_register.setProgramStatus(flag, true);
            }
        }

        template <typename T, typename... Ts> constexpr
        auto recursiveLoaderHelper(std::size_t idx, std::size_t limit, T x, Ts... xs) noexcept
            -> void
        {
            if (idx > limit) {
                return;
            }

            m_memory.write(static_cast<system_bit_type>(x), idx);

            if constexpr (sizeof...(Ts) > 0)
            {
                recursiveLoaderHelper(idx + 1, limit, xs...);
            }
        }

        template <std::convertible_to<typename alu_in_type::operand_width_type> T> inline static constexpr
        auto makeALUInput(au::ALUOpCode alu_opcode, const T operand1, const T operand2) noexcept
            -> alu_in_type
        {
            return alu_in_type{
                alu_opcode,
                {operand1, operand2}
            };
        }

        [[nodiscard]] constexpr
        auto fetch()
            -> system_bit_type
        {
            using enum rf::GPReg;
            using enum rf::SEGReg;

            if (
                !Core::checkAddressInrange<system_bit_type>(m_register.getGeneralPurpose(PC), 
                                                            m_segment.at(CS))
                )
            {
                traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                    "Error: PC exceeds CS boundary!"
                );
            }

            if (
                auto instruction = m_memory.read(m_register.getGeneralPurpose(PC));
                instruction.has_value()
                )
            {
                ++m_register.getGeneralPurpose(PC);
                return *instruction;
            }

            traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                "Error: Failed to read instruction from memory!"
            );

            return 0;
        }

        constexpr
        auto generateALUInput(const instruction_type& instruction) const
            -> alu_in_type
        {
            using enum dc::OpType;
            using enum dc::OpCode;

            if (instruction.m_type == Jt)
            {
                traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                    "Jump-type instruction fall through!"
                );
            }

            auto makeALUInputForRI = [this](const instruction_type& instruction, au::ALUOpCode opcode)
                -> alu_in_type
            {
                using enum dc::OpType;

                switch (instruction.m_type)
                {
                    case Rt:
                    {
                        return makeALUInput<system_bit_type>(
                            opcode, 
                            m_register.getGeneralPurpose(instruction.m_Rm), 
                            m_register.getGeneralPurpose(instruction.m_Rn)
                        );
                    }
                    case It:
                    {
                        return makeALUInput<system_bit_type>(
                            opcode, 
                            m_register.getGeneralPurpose(instruction.m_Rm), 
                            instruction.m_imm
                        );
                    }
                    default:
                    {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                            "Unknown instruction type detected."
                        );
                    }    
                }

                return alu_in_type{};
            };

            using au::ALUOpCode;
            using enum rf::GPReg;

            switch (instruction.m_code)
            {
                case ADD : { return makeALUInputForRI(instruction, ALUOpCode::ADD);  }
                case UMUL: { return makeALUInputForRI(instruction, ALUOpCode::UMUL); }
                case UDIV: { return makeALUInputForRI(instruction, ALUOpCode::UDIV); }
                case UMOL: { return makeALUInputForRI(instruction, ALUOpCode::UMOL); }
                case AND : { return makeALUInputForRI(instruction, ALUOpCode::AND);  }
                case ORR : { return makeALUInputForRI(instruction, ALUOpCode::ORR);  }
                case XOR : { return makeALUInputForRI(instruction, ALUOpCode::XOR);  }
                case SHL : { return makeALUInputForRI(instruction, ALUOpCode::SHL);  }
                case SHR : { return makeALUInputForRI(instruction, ALUOpCode::SHR);  }
                case RTL : { return makeALUInputForRI(instruction, ALUOpCode::RTL);  }
                case RTR : { return makeALUInputForRI(instruction, ALUOpCode::RTR);  } 
                case NOT : {
                    return makeALUInput<system_bit_type>(
                        ALUOpCode::COMP, m_register.getGeneralPurpose(instruction.m_Rm), 0x0
                    );
                }
                case LDR : {
                    return makeALUInput<system_bit_type>(
                        ALUOpCode::PASS, m_register.getGeneralPurpose(instruction.m_Rm), 0x0
                    );
                }
                case STR : {
                    return makeALUInput<system_bit_type>(
                        ALUOpCode::PASS, m_register.getGeneralPurpose(instruction.m_Rm), 0x0
                    );
                }
                case PUSH: {
                    return makeALUInput<system_bit_type>(
                        ALUOpCode::ADD, m_register.getGeneralPurpose(SP), -1
                    );
                }
                case POP : {
                    return makeALUInput<system_bit_type>(
                        ALUOpCode::ADD, m_register.getGeneralPurpose(SP), 1
                    );
                }
                default:   {
                    traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>("Unknown OpCode detected.");
                }
            }

            return alu_in_type{};
        }

        [[nodiscard]] constexpr
        auto execute(const instruction_type& instruction)
            -> typename alu_out_type::operand_width_type
        {
            const alu_out_type result = alu_type::execute<system_bit_type>(generateALUInput(instruction));
            updatePsr(result.m_flags);
            return result.m_result;
        }

        constexpr
        auto checkJump(const instruction_type& instruction)
            -> bool
        {
            using enum dc::OpType;
            using enum dc::OpCode;
            using enum rf::GPReg;
            using enum rf::PSRReg;

            if (instruction.m_type != Jt) {
                return false;
            }

            auto performJump = [this](bool condition, typename register_type::GP_width_type dst) noexcept
                -> void
            {
                if (condition) {
                    m_register.getGeneralPurpose(PC) = dst;
                }
            };

            switch (instruction.m_code)
            {
                case JMP:
                {
                    performJump(true, instruction.m_imm);
                    break;
                }
                case JZ: 
                {
                    performJump(m_register.getProgramStatus(Z), instruction.m_imm);
                    break; 
                }
                case JN: 
                {
                    performJump(m_register.getProgramStatus(N), instruction.m_imm);
                    break;
                }
                case JC: 
                {
                    performJump(m_register.getProgramStatus(C), instruction.m_imm);
                    break;
                }
                case JV: 
                {
                    performJump(m_register.getProgramStatus(V), instruction.m_imm);
                    break;
                }
                case JZN:
                {
                    performJump(
                        m_register.getProgramStatus(Z) || m_register.getProgramStatus(N),
                        instruction.m_imm
                    );
                    break;
                }
                case SYSCALL:
                {
                    try
                    {
                        syscall_type::template m_syscall_table<Core::memory_type, Core::register_type>.at(
                            static_cast<std::uint32_t>(instruction.m_imm)
                        ) (this->m_memory, this->m_register);
                    }
                    catch (const std::out_of_range&)
                    {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::out_of_range>("Unknown syscall number.");
                    }
                    break;
                }
                default:
                {
                    traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>("Unknown OpCode detected.");
                    break;
                }
            }

            return true;
        }

        constexpr
        auto memoryAccess(const instruction_type& instruction, system_bit_type addr_val)
            -> void
        {
            using enum dc::OpCode;
            using enum rf::SEGReg;
            using enum rf::GPReg;

            switch (instruction.m_code)
            {
                case LDR:
                {
                    if (
                        auto result = m_memory.read(addr_val);
                        result.has_value()
                        )
                    {
                        m_register.getGeneralPurpose(instruction.m_Rd) = *result;
                    }
                    else {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>("Invalid memory access.");
                    }
                    break;
                }
                case STR:
                {
                    if (
                        auto result = m_memory.write(m_register.getGeneralPurpose(instruction.m_Rd), addr_val);
                        !result
                        )
                    {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>("Invalid memory access.");
                    }
                    break;
                }
                case PUSH:
                {
                    if (!Core::checkAddressInrange(addr_val, m_segment.at(SS)))
                    {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>("Stack-overflow :)");
                    }
                    m_memory.write(m_register.getGeneralPurpose(instruction.m_Rd), addr_val);
                    m_register.getGeneralPurpose(SP) = addr_val;
                    break;
                }
                case POP:
                {
                    if (!Core::checkAddressInrange(addr_val - 1, m_segment.at(SS))) {
                        return;
                    }
                    if (
                        auto result = m_memory.read(m_register.getGeneralPurpose(SP));
                        result.has_value()
                        )
                    {
                        m_register.getGeneralPurpose(instruction.m_Rd) = *result;
                        m_register.getGeneralPurpose(SP) = addr_val;
                    }
                    else {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>("Invalid memory access.");
                    }
                    break;
                }
                default: 
                {
                    m_register.getGeneralPurpose(instruction.m_Rd) = addr_val;
                    break;
                }
            }
        }

    private:
        memory_type   m_memory{};
        register_type m_register{};
        segment_type  m_segment{};

        tr::Tracer*   m_tracer;
    };
}
