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

    public:
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
        if (!freefuncs::checkBitInrange(n, position)) {
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

        if (sizeof(n) == sizeof(std::uintmax_t)) {
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

    [[nodiscard]] static constexpr
    auto testBitAll(const std::unsigned_integral auto n, const std::size_t last_nbit) noexcept
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
        if (!freefuncs::checkBitInrange<uint>(position)) {
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
        if constexpr (sizeof...(positions) > 0) {
            result = freefuncs::setBit(n, positions...);
        }
        return result;
    }

    template <std::unsigned_integral uint> static constexpr
    auto resetBit(uint& n, const std::size_t position) noexcept
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
        if constexpr (sizeof...(positions) > 0) {
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
        if (!freefuncs::checkBitInrange<uint>(position)) {
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

    template <typename T> [[nodiscard]] static inline constexpr
    auto promote(const T n) noexcept
        -> make_promoted_t<T>
    {
        return n;
    }
}


namespace scsp::register_file
{
    enum GPReg : std::uint8_t {
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

    enum SEGReg : std::uint8_t {
        CS,		//      Code Segment
        DS,		//      Data Segment
        SS,		//      Stack Segment
        ES		//      Extra Segment
    };

    enum PSRReg : std::uint8_t {
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
            return ff::testBit<PSR_width_type>(m_psr, flag_name);
        }

        constexpr
        auto setProgramStatus(const std::uint8_t flag_name, const bool value) noexcept
            -> std::expected<bool, std::runtime_error>
        {
            namespace ff = ::scsp::freefuncs;
            std::expected<void, std::domain_error> result =
                (value) ? ff::setBit  <PSR_width_type>(m_psr, flag_name) :
                          ff::resetBit<PSR_width_type>(m_psr, flag_name);
            if (!result) {
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
            using operand_width_type = typename ALUInput<Width>::operand_width_type;
            std::pair<operand_width_type, operand_width_type> AB = alu_input.m_operands;
            using enum ::scsp::ALU::ALUOpCode;

            switch (alu_input.m_opcode)
            {
                case ADD :  return add (AB.first, AB.second);
                case UMUL:  return umul(AB.first, AB.second);
                case UDIV:  return udiv(AB.first, AB.second);
                case UMOL:  return umol(AB.first, AB.second);
                case PASS:  return pass(AB.first           );

                case AND :  return and_(AB.first, AB.second);
                case ORR :  return orr (AB.first, AB.second);
                case XOR :  return xor_(AB.first, AB.second);
                case COMP:  return comp(AB.first           );

                case SHL :  return shl (AB.first, AB.second);
                case SHR :  return shr (AB.first, AB.second);
                case RTL :  return rtl (AB.first, AB.second);
                case RTR :  return rtr (AB.first, AB.second);
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
            if (ff::testBit<T>(R, sizeof(T) * CHAR_BIT - 1)) {
                flags.insert(N);
            }
            // if the result is equal to 0, then set Zero flag;
            if (ff::testBitNone(R) && R == 0x0) {
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
            if (checkCarry(A, B, R)) {
                flags.insert(C);
            }
            if (checkOverflow(A, B, R)) {
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
                if (ff::testBit<T>(A, msb) ^ ff::testBit<T>(B, msb)) {
                    return false;
                }
                // check if A | B's msb differ from R's msb
                return (ff::testBit<T>(A, msb) ^ ff::testBit<T>(R, msb)) ? true : false;
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
            return (result) ?
                    makeOutput<T>(A) :
                    ALUOutput<T>{};
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

            if (mask_length > sizeof(T) * CHAR_BIT) {
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

            for (auto i = registers.getGeneralPurpose(R0);
                 i < (registers.getGeneralPurpose(R0) + registers.getGeneralPurpose(R1));
                 ++i
                )
            {
                if (
                    auto result = memory.read(i);
                    result
                ) {
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
                [i = registers.getGeneralPurpose(R0), &memory](auto character) mutable noexcept
                    -> void
                {
                    memory.write(character, i);
                    ++i;
                }
            );
        }
    };
}








template <typename Sy, typename T, typename... Ts>
concept is_valid_data = std::convertible_to<T, Sy> && (std::same_as<T, Ts> || ...);

template <std::unsigned_integral B, std::size_t S, class SysT>
class Core {
public:
    using sysb = B;						// system bit
    const std::size_t mem_size = S;		// memory size

    // components
    using memory = memory<sysb, S>;
    using registers = Registers<sysb>;
    using decoder = Decoder<default_encoding>;
    using alu = ALU;

    // sub-components
    using instruct = typename decoder::instr_type;
    using alu_in = ALU_in<sysb>;
    using alu_out = ALU_out<sysb>;

    // helper definition
    using segment = std::unordered_map<SEGReg, std::pair<sysb, sysb>>;
    using syscall = SysT;

    // memory layout
    /*
    *  _________________  <----------- Top
    * |_________________|
    * |					| \
    * |	 Extra Segment  | | 64KB (magic fields :)
    * |_________________| /<----------- ES reg
    * |_________________|
    * |				    | \
    * |					| |
    * |	 Stack Segment  | | 64KB
    * |					| |
    * |					| |
    * |_________________| / <----------- SS reg
    * |_________________|
    * |					| \
    * |	  Data Segment  | | 64KB
    * |					| |
    * |_________________| / <----------- DS reg
    * |_________________|
    * |					| \
    * |	  Code Segment  | | 64KB
    * |_________________| / <---------- CS reg
    * |_________________| <----------- Bottom (vector table or nullptr)
    */

    // Initialize segment registers and define size for each segment
    // Initialize values for GP registers
    template <typename SG>
        requires std::same_as<segment, std::remove_reference_t<SG>>
    constexpr auto init(SG&& seg_config) noexcept -> bool {
        // check if the config contains all needed field
        constexpr std::array<SEGReg, 4> seg_regs{ CS, DS, SS, ES };
        if (!std::ranges::all_of(seg_regs, [&seg_config](SEGReg sr) -> bool {return seg_config.contains(sr); })) {
            return false;
        }
        // segment overlap is not allowed!
        auto overlap = []<typename T>(const std::pair<T, T>&r1, decltype(r1) r2) -> bool {
            return r1.first <= r2.second && r2.first <= r1.second;
        };
        using value_type = typename segment::mapped_type;
        std::vector<value_type> rngs{};
        rngs.reserve(4);
        for (auto&& r : seg_config) {
            rngs.emplace_back(r.second);
        }
        // check if the start and end addr for each segment is in the range
        if (!std::ranges::all_of(rngs, [this](const value_type& p) -> bool {
            return p.second >= p.first && (p.second >= 0 && p.second < Core::mem_size); })
            ) {
            return false;
        }
        // sort by starting index
        std::ranges::sort(rngs, std::ranges::less{}, [](const value_type& r) {return r.first; });
        for (auto it = rngs.cbegin() + 1; it != rngs.cend(); ++it) {
            if (overlap(*(it - 1), *it)) {
                return false;
            }
        }
        // total segments size should be <= mem_size
        if (sysb res = std::accumulate(rngs.begin(), rngs.end(), 0,
            [](sysb&& i, const value_type& p) {return i + (p.second - p.first) + 1; });
            res > mem_size) {
            return false;
        }
        segments_ = std::forward<SG>(seg_config);
        // configure SP register to the highest addr in the seg
        registers_.GP(SP) = segments_.at(SS).second + 1;
        // configure PC register to the lowest addr in the seg
        registers_.GP(PC) = segments_.at(CS).first;
        return true;
    }

    // This ctor overload is used when no tracer is provided
    template <typename SG> requires std::same_as<segment, std::remove_reference_t<SG>>
    [[nodiscard]] constexpr explicit Core(SG&& seg_config)
        : tracer_{ nullptr } {
        if (!this->init(std::forward<SG>(seg_config))) {
            trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>(
                "Error: Failed to initialize segments!");
        }
    }

    // This ctor overload is used when tracer is provided
    template <typename SG> requires std::same_as<segment, std::remove_reference_t<SG>>
    [[nodiscard]] constexpr explicit Core(
        SG&& seg_config,
        Tracer* tracer
    ) : tracer_{ tracer } {
        if (!this->init(std::forward<SG>(seg_config))) {
            trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>(
                "Error: Failed to initialize segments!");
        }
    }

    // Instruction loader
    template <typename T, typename... Ts> requires is_valid_data<sysb, T, Ts...>
    constexpr auto load_instr(T in, Ts... ins) noexcept -> void {
        r_load(segments_.at(CS).first, segments_.at(CS).second, in, ins...);
    }
    // Data loader
    template <typename T, typename... Ts> requires is_valid_data<sysb, T, Ts...>
    constexpr auto load_data(T in, Ts... ins) noexcept -> void {
        r_load(segments_.at(DS).first, segments_.at(DS).second, in, ins...);
    }

    // Core function
    constexpr auto run() -> void {
        for (;;) {
            auto bin = fetch();
            // terminate condition
            if (bit_test_all(bin)) {
                break;
            }
            instruct inst = decoder::decode(bin);
            // treat jumps
            if (check_jump(inst)) {
                generate_trace(bin, inst);
                continue;
            }
            auto result = execute(inst);
            mem_access(inst, result);
            generate_trace(bin, inst);
        }
    }

private:
    // log message and error reporting
    template <Tracer::LogCriticalLvls lvl, std::derived_from<std::exception> Except>
    constexpr auto trace_log(const std::string_view& msg) const -> void {
        if (tracer_ != nullptr) {
            tracer_->log<lvl, Except>(msg);
            return;
        }
        if (lvl == Tracer::LogCriticalLvls::ERROR) {
            throw Except(msg.data());
        }
    }

    // create trace log
    constexpr auto generate_trace(sysb bin, instruct& inst) noexcept -> void {
        if (tracer_ != nullptr) {
            tracer_->generate_trace<
                sysb, instruct, memory, registers, segment
            >(bin, inst, memory_, registers_, segments_);
        }
    }

    // fetch unit
    [[nodiscard]] constexpr auto fetch() -> sysb {
        if (!Core::in_range<sysb>(registers_.GP(PC), segments_.at(CS))) {
            trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>(
                "Error: PC exceeds CS boundary!");
        }
        if (auto instr = memory_.read_slot(registers_.GP(PC)); instr.has_value()) {
            ++registers_.GP(PC);
            return instr.value();
        }
        trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>(
            "Error: Failed to read instruction from memory!");
        // dummy return value to silent compiler warning
        return 0;
    }

    // execute unit
    [[nodiscard]] constexpr auto execute(const instruct& instr) -> typename alu_out::op_size {
        alu_out out = alu::exec<typename alu_in::op_size>(gen_ALUIn(instr));
        update_psr(out.Flags_);
        return out.Res_;
    }

    // memory access and write back
    constexpr auto mem_access(const instruct& instr, sysb v) -> void {
        switch (instr.code_) {
            // only LDR, STR, PUSH, and POP will access memory;
        case LDR: {
            if (auto m = memory_.read_slot(v); m.has_value()) {
                registers_.GP(instr.Rd_) = m.value();
            }
            else {
                trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>(
                    "Error: Invalid memory access!");
            }
            break;
        }
        case STR: {
            if (!memory_.write_slot(registers_.GP(instr.Rd_), v)) {
                trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>(
                    "Error: Invalid memory access!");
            }
            break;
        }
        case PUSH: {
            if (!Core::in_range(v, segments_.at(SS))) {
                trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>("Error: Stackoverflow!");
            }
            memory_.write_slot(registers_.GP(instr.Rd_), v);
            registers_.GP(SP) = v;
            break;
        }
        case POP: {
            if (!Core::in_range(v - 1, segments_.at(SS))) {
                return;
            }
            if (auto m = memory_.read_slot(registers_.GP(SP)); m.has_value()) {
                registers_.GP(instr.Rd_) = m.value();
                registers_.GP(SP) = v;
            }
            else {
                trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>("Error: Invalid memory access!");
            }
            break;
        }
                // for other instructions, "v" is the result that needs to be write back to Rd
        default: {
            registers_.GP(instr.Rd_) = v;
            break;
        }
        }
    }

    // check jump condition before ALU
    // if is jump instruction, then return true, else return false
    constexpr auto check_jump(const instruct& ins) -> bool {
        if (ins.type_ != J_t) {
            return false;
        }
        // lambda function to perform jump
        auto perform_jump = [this](bool cond, typename registers::gp_width dst)
            noexcept -> void {
            if (cond) {
                registers_.GP(PC) = dst;
            }
            };
        switch (ins.code_) {
        case JMP:	perform_jump(true, ins.imm_);		break;
        case JZ:	perform_jump(registers_.PSR(Z), ins.imm_);		break;
        case JN:	perform_jump(registers_.PSR(N), ins.imm_);		break;
        case JC:	perform_jump(registers_.PSR(C), ins.imm_);		break;
        case JV:	perform_jump(registers_.PSR(V), ins.imm_);		break;
        case JZN:	perform_jump(registers_.PSR(Z) ||
            registers_.PSR(N), ins.imm_);		break;
        case SYSCALL: {
            try {
                syscall::template syscall_table_<Core::memory, Core::registers>.at(
                    static_cast<std::uint32_t>(ins.imm_))
                    (this->memory_, this->registers_);
            }
            catch (const std::out_of_range&) {
                trace_log<Tracer::LogCriticalLvls::ERROR, std::out_of_range>(
                    "Error: Unrecoganized SYSCALL number!");
            }
            break;
        }
        default:
            trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>(
                "Error: Unrecoganized instruction type detected!");
            break;
        }
        return true;
    }

    // generate alu_in based on the instruction
    constexpr auto gen_ALUIn(const instruct& ins) const -> alu_in {
        // J-type instruction should not go through
        if (ins.type_ == J_t) {
            trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>("Error: Jump-type instruction fall through!");
        }
        // lambda function to help create ALU input for R & I type instruction
        auto make_ALUIn_RI = [this](const instruct& ins, ALU_OpCode op) -> alu_in {
            switch (ins.type_) {
            case R_t:	return make_ALUIn<sysb>(op, registers_.GP(ins.Rm_), registers_.GP(ins.Rn_));
            case I_t:	return make_ALUIn<sysb>(op, registers_.GP(ins.Rm_), ins.imm_);
            default:	trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>(
                "Error: Unrecoganized instruction type detected!");
            }
            // dummy return value to silent compiler warning
            return alu_in{};
            };
        switch (ins.code_) {
            // Arithmetic Ops
        case ADD:		return make_ALUIn_RI(ins, ALU_OpCode::ADD);
        case UMUL:		return make_ALUIn_RI(ins, ALU_OpCode::UMUL);
        case UDIV:		return make_ALUIn_RI(ins, ALU_OpCode::UDIV);
        case UMOL:		return make_ALUIn_RI(ins, ALU_OpCode::UMOL);
            // Binary Logical Ops
        case AND:		return make_ALUIn_RI(ins, ALU_OpCode::AND);
        case ORR:		return make_ALUIn_RI(ins, ALU_OpCode::ORR);
        case XOR:		return make_ALUIn_RI(ins, ALU_OpCode::XOR);
        case SHL:		return make_ALUIn_RI(ins, ALU_OpCode::SHL);
        case SHR:		return make_ALUIn_RI(ins, ALU_OpCode::SHR);
        case RTL:		return make_ALUIn_RI(ins, ALU_OpCode::RTL);
        case RTR:		return make_ALUIn_RI(ins, ALU_OpCode::RTR);
            // Uniary Logical Ops
            // for uniary operations, only the first operand is used, second is discarded (0x0)
        case NOT:		return make_ALUIn<sysb>(ALU_OpCode::COMP, registers_.GP(ins.Rm_), 0x0);
            // Load and Store Ops
        case LDR:		return make_ALUIn<sysb>(ALU_OpCode::PASS, registers_.GP(ins.Rm_), 0x0);
        case STR:		return make_ALUIn<sysb>(ALU_OpCode::PASS, registers_.GP(ins.Rm_), 0x0);
            // Stack Ops
        case PUSH:		return make_ALUIn<sysb>(ALU_OpCode::ADD, registers_.GP(SP), -1);
        case POP:		return make_ALUIn<sysb>(ALU_OpCode::ADD, registers_.GP(SP), 1);
        default:		trace_log<Tracer::LogCriticalLvls::ERROR, std::runtime_error>(
            "Error: Unrecoganized instruction type detected!");
        }
        // dummy return value to silent compiler warning
        return alu_in{};
    }

    // helper function to create ALU_in
    template <std::convertible_to<typename alu_in::op_size> T>
    inline static constexpr auto make_ALUIn(ALU_OpCode alu_op, T op1, T op2) noexcept
        -> alu_in {
        return alu_in{
            alu_op,
            {op1, op2}
        };
    }

    // set psr flags
    constexpr auto update_psr(const typename alu_out::flag_type& flags) noexcept -> void {
        // Note: alu_out::flag_type == unordered_set<std::uint8_t>;
        registers_.clear_psr();
        for (auto&& F : flags) {
            registers_.PSR(F, true);
        }
    }

    // I-loader & D-loader helper
    template <typename T, typename... Ts>
    constexpr auto r_load(std::size_t idx, std::size_t lim, T x, Ts... xs) noexcept -> void {
        if (idx > lim) {
            return;
        }
        memory_.write_slot(static_cast<sysb>(x), idx);
        if constexpr (sizeof...(Ts) > 0) {
            r_load(idx + 1, lim, xs...);
        }
    }

    // validate addr with segment size
    template <std::unsigned_integral T>
    [[nodiscard]] inline static constexpr auto in_range(T p, const std::pair<T, T>& seg) noexcept -> bool {
        return p >= seg.first && p <= seg.second;
    }

private:
    memory		memory_{};
    registers	registers_{};

    // The memory or registers are not aware of memory segmentation
    segment		segments_{};

    // The tracer pointer is used to record CPU state information
    Tracer* tracer_;
};






//namespace scsp::tracer
//{
//    template <typename Exception>
//    concept valid_exception = requires
//    {
//        requires
//        std::derived_from<Exception, std::exception>;
//    };
//
//    template <typename Object>
//    using const_lref_type = const std::add_lvalue_reference_t<Object>;
//
//    class Tracer
//    {
//    public:
//        enum struct CriticalLvls : std::int8_t
//        {
//            INFO,
//            WARNING,
//            ERROR
//        };
//
//        struct Labels
//        {
//            using enum ::scsp::register_file::GPReg;
//            using enum ::scsp::register_file::SEGReg;
//            using enum ::scsp::register_file::PSRReg;
//            using enum ::scsp::decode::OpCode;
//            using enum ::scsp::decode::OpType;
//
//            using gp_translation_type  = const std::map<::scsp::register_file::GPReg,            std::string_view>;
//            using psr_translation_type = const std::unordered_map<::scsp::register_file::PSRReg, char            >;
//            using seg_translation_type = const std::unordered_map<::scsp::register_file::SEGReg, std::string_view>;
//            using opc_translation_type = const std::unordered_map<::scsp::decode::OpCode,        std::string_view>;
//            using opt_translation_type = const std::unordered_map<::scsp::decode::OpType,        std::string_view>;
//
//            static inline gp_translation_type gp_translation = {
//                {R0, "R0"  },   {R1, "R1"},   {R2, "R2"  },   {R3, "R3"  },
//                {R4, "R4"  },   {R5, "R5"},   {R6, "R6"  },   {R7, "R7"  },
//                {R8, "R8"  },   {R9, "R9"},   {R10, "R10"},   {R11, "R11"},
//                {R12, "R12"},   {SP, "SP"},   {LR, "LR"  },   {PC, "PC"  }
//            };
//
//            static inline psr_translation_type psr_translation = {
//                {N, 'N'},
//                {Z, 'Z'},
//                {C, 'C'},
//                {V, 'V'}
//            };
//
//            static inline seg_translation_type seg_translation = {
//                {CS, "Code Segment"},
//                {DS, "Data Segment"},
//                {SS, "Stack Segment"},
//                {ES, "Extra Segment"}
//            };
//
//            static inline opt_translation_type opt_translation = {
//                {Rt, "R type"},
//                {It, "I type"},
//                {Ut, "U type"},
//                {St, "S type"},
//                {Jt, "J type"}
//            };
//
//            static inline opc_translation_type opc_translation = {
//                {ADD, "ADD"}, {UMUL, "UMUL"}, {UDIV, "UDIV"}, {UMOL, "UMOL"},
//
//                {AND, "AND"}, {ORR, "ORR"}, {XOR, "XOR"}, {SHL, "SHL"},
//                {SHR, "SHR"}, {RTL, "RTL"}, {RTR, "RTR"}, {NOT, "NOT"},
//
//                {LDR, "LDR"}, {STR, "STR"}, {PUSH, "PUSH"}, {POP, "POP"},
//
//                {JMP, "JMP"}, {JZ, "JZ"}, {JN, "JN"}, {JC, "JC"}, {JV, "JV"}, {JZN, "JZN"},
//
//                {SYSCALL, "SYSCALL"}
//            };
//        };
//
//    public:
//        [[nodiscard]] explicit
//        Tracer(const std::filesystem::path& log_path)
//            : m_log_file{ std::filesystem::absolute(log_path).c_str() }
//            , m_instruction_count{ 0 }
//        {
//            if (!m_log_file.is_open())
//            {
//                throw std::filesystem::filesystem_error(
//                    "Failed to create the log file.",
//                    std::error_code(1, std::system_category())
//                );
//            }
//        }
//
//    public:
//        template <Tracer::CriticalLvls Level    ,
//                  valid_exception      Exception>
//        auto log(const std::string_view& message)
//            -> void
//        {
//            m_log_file <<
//                [](Tracer::CriticalLvls level)
//                    -> std::string_view
//                {
//                    using enum ::scsp::tracer::Tracer::CriticalLvls;
//                    switch (level)
//                    {
//                        case INFO:      return "INFO: "   ;
//                        case WARNING:   return "WARNING: ";
//                        case ERROR:     return"ERROR: "   ;
//                        default:        throw std::runtime_error("Unknown critical level.");
//                    }
//                }(Level);
//
//            m_log_file << message << '\n';
//            
//            if (Level == Tracer::CriticalLvls::ERROR)
//            {
//                m_log_file.close();
//                throw Exception(message.data());
//            }
//        }
//
//        template <std::unsigned_integral T             ,
//                  typename Instruction, typename Memory,
//                  typename Register,    typename Segment>
//        auto generateTrace(const T binary,
//                           const_lref_type<Instruction> instruction, const_lref_type<Memory>  memory,
//                           const_lref_type<Register>    registers,   const_lref_type<Segment> segments) noexcept
//            -> void
//        {
//
//        }
//
//    private:
//        template <typename Memory, typename Segment>
//        auto getMemory(const_lref_type<Memory> memory, const_lref_type<Segment> segments)
//            -> std::string
//        {
//            std::stringstream ss_segments{};
//            std::stringstream ss_values{};
//
//            for (const auto& p : segments)
//            {
//                try
//                {
//                    ss_segments << Labels::seg_translation.at(static_cast<::scsp::register_file::SEGReg>(p.first));
//                }
//                catch (const std::out_of_range&)
//                {
//                    Tracer::log<CriticalLvls::ERROR, std::out_of_range>(
//                        "No corresponding segment translation."
//                    );
//                }
//
//                for (auto i = p.second.first; i <= p.)
//            }
//        }
//
//    private:
//        std::ofstream m_log_file;
//        std::uint32_t m_instruction_count;
//    };
//}
//
//
//
//
//
//            
//
//
//        for (auto i{ p.second.first }; i <= p.second.second; ++i) {
//            ss_value << static_cast<std::uint32_t>(mem.read_slot(i).value()) << ',';
//        }
//        formatter(ss_seg, ss_value);
//
//    return ss_seg.str();
//
//
//
//
//
//        //trace_file_ << get_heading(binary);
//        //trace_file_ << get_instr<Ins>(instr);
//        //trace_file_ << get_reg<Reg>(registers);
//        //trace_file_ << get_mem<Mem, Seg>(memory, segments);
//        //trace_file_ << '\n';
//        //++inst_count;
//
//
//
//
//    template <class Reg>
//    auto get_reg(const std::add_lvalue_reference_t<Reg> regs) const noexcept -> std::string {
//        std::stringstream ss_label{}, ss_value{};
//        std::ranges::for_each(labels_.gp_trans_, [&](const auto p)->void {
//            ss_label << p.second << ','; ss_value << regs.GP(p.first) << ','; });
//        formatter(ss_label, ss_value);
//        std::ranges::for_each(labels_.psr_trans_, [&](const auto p)->void {
//            ss_label << p.second << ','; ss_value << regs.PSR(p.first) << ','; });
//        formatter(ss_label, ss_value);
//        return ss_label.str();
//    }
//
//    static auto formatter(std::stringstream& l, std::stringstream& v) noexcept -> void {
//        l << '\n';
//        v << '\n';
//        l << v.str();
//        v.str(std::string{});
//    }
//
//    template <class Ins>
//    auto get_instr(const std::add_lvalue_reference_t<Ins> inst) -> std::string {
//        std::stringstream ss_label{}, ss_value{};
//        const std::array<std::string_view, 6> l{ "OpType", "OpCode", "Rd", "Rm", "Rn", "Imm" };
//        const std::array<typename Ins::Rt, 3> regs{ inst.Rd_, inst.Rm_, inst.Rn_ };
//        std::ranges::for_each(l, [&ss_label](const auto v)->void {ss_label << v << ','; });
//        ss_label << '\n';
//        try {
//            ss_value << labels_.type_trans_.at(static_cast<OpType>(inst.type_)) << ',';
//            ss_value << labels_.code_trans_.at(static_cast<OpCode>(inst.code_)) << ',';
//            std::ranges::for_each(regs, [this, &ss_value](const auto v)->void {
//                ss_value << labels_.gp_trans_.at(static_cast<GPReg>(v)) << ','; });
//        }
//        catch (const std::out_of_range&) {
//            Tracer::log<Tracer::LogCriticalLvls::ERROR, std::out_of_range>(
//                "Error: Tracer::type_trans_ / Tracer::code_trans_ / Tracer::gp_trans_ "
//                "- No corresponding translation!"
//            );
//        }
//        // immediate value
//        ss_value << static_cast<std::uint32_t>(inst.imm_) << '\n';
//        ss_label << ss_value.str();
//        return ss_label.str();
//    }
//
//    template <std::unsigned_integral T>
//    static auto get_heading(T binary) noexcept -> std::string {
//        std::stringstream ss;
//        ss << "Instruction #, 0x" << std::setfill('0') << std::setw(sizeof(T) * 2)
//            << std::hex << binary << '\n';
//        return ss.str();
//    }
//
//};
//
//
//
//








