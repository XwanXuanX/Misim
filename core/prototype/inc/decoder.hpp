/**
 * @file decoder.hpp
 * @brief The decoder component of prototype CPU.
 * 
 * @date Sep, 2023
 * @author Yetong (Tony) Li
*/

#pragma once


#include <concepts>
#include <cstdint>
#include "freefunc.hpp"


namespace prototype::decode
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
        using register_type  = field_size_type<EncodingRule::m_Rd.m_length     >;
        using immediate_type = field_size_type<EncodingRule::m_imm.m_length    >;

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

            return instruction_type
            {
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
            using ::prototype::freefunc::setBit;

            T mask = 0x0;
            setBit<T>(mask);

            if (mask_length > sizeof(T) * CHAR_BIT) {
                return mask;
            }

            return mask >> (sizeof(T) * CHAR_BIT - mask_length);
        }

        template <std::unsigned_integral T,
                  std::unsigned_integral Conversion> [[nodiscard]] static constexpr
        auto extract(T instruction, field_type field) noexcept
            -> Conversion
        {
            instruction >>= field.m_start_idx;
            return static_cast<Conversion>(instruction & makeMask<T>(field.m_length));
        }
    };
}
