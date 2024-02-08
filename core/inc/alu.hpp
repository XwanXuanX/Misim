/**
 * @file alu.hpp
 * @brief The ALU component of the prototype CPU.
 * 
 * @author Yetong (Tony) Li
 * @date Sep, 2023
*/

#pragma once


#include <bit>
#include <concepts>
#include <cstdint>
#include <unordered_set>
#include "freefunc.hpp"
#include "register.hpp"


namespace prototype::alu
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
            using enum ::prototype::alu::ALUOpCode;
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
        template <std::unsigned_integral T> static constexpr
        auto getProgramStatusFlags(const T R, std::unordered_set<std::uint8_t>& flags) noexcept
            -> void
        {
            // get program status flags from result only
            // if the first bit (sign bit) is set, then set Neg flag;

            namespace ff = ::prototype::freefunc;
            using enum ::prototype::register_file::PSRReg;
            using return_t = std::variant<bool, std::domain_error>;

            const return_t result = ff::testBit<T>(R, sizeof(T) * CHAR_BIT - 1);
            assert(result.index() == 0);

            if (std::get<0>(result)) {
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

        template <std::unsigned_integral     T,
                  valid_callback_function<T> Function> [[nodiscard]] static constexpr
        auto getProgramStatusFlags(const T A, const T B, const T R,
                                   Function checkCarry, Function checkOverflow) noexcept
            -> std::unordered_set<std::uint8_t>
        {
            using enum ::prototype::register_file::PSRReg;

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

            return ALUOutput<T> {
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
                namespace ff = ::prototype::freefunc;
                using return_t = std::variant<bool, std::domain_error>;

                // if A & B's msb differ, no overflow
                const return_t ra = ff::testBit<T>(A, msb);
                const return_t rb = ff::testBit<T>(B, msb);
                assert(ra.index() == 0);
                assert(rb.index() == 0);

                if (std::get<0>(ra) ^ std::get<0>(rb)) {
                    return false;
                }

                // check if A | B's msb differ from R's msb
                const return_t rr = ff::testBit<T>(R, msb);
                assert(rr.index() == 0);
                return (std::get<0>(ra) ^ std::get<0>(rr)) ? true : false;
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

            using ::prototype::freefunc::promote;
            return makeOutput<T>(static_cast<T>(promote<T>(A) * promote<T>(B)));
        }

        // 3. UDIV
        template <std::unsigned_integral T> [[nodiscard]] static constexpr
        auto udiv(const T A, const T B) noexcept
            -> ALUOutput<T>
        {
            using ::prototype::freefunc::testBitNone;

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
            using ::prototype::freefunc::testBitNone;

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
            using ::prototype::freefunc::flipBit;
            using return_t = std::variant<bool, std::domain_error>;

            const return_t result = flipBit(A);

            return (result.index() == 0) ? makeOutput<T>(A) :
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
