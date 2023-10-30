/*********************************************************************
 *
 *  @file   prototype.hpp
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

#pragma once


#include "core.hpp"
#include "loader.hpp"
#include <optional>


using parameter_type = std::optional<std::string>;

void wain(parameter_type&& binary, parameter_type&& log)
{
	namespace pc = ::prototype::core;
	namespace ps = ::prototype::syscall;
	namespace tr = ::prototype::tracer;

	constexpr std::uint32_t mem_size = 300;
	using system_bit = std::uint32_t;
	using syscall = ps::SyscallTable;

	using core = pc::Core<system_bit, mem_size, syscall>;

	if (!binary.has_value())
	{
		throw std::runtime_error("Invalid parameter.");
	}

	core::Loader loader(std::filesystem::path(std::move(binary.value())));
	loader.parseBinaryFile();

	if (log.has_value())
	{
		tr::Tracer trace(std::move(log.value()));

		core my_core{ loader.getSegments(), &trace };

		my_core.dataLoader<core::Loader::data_type>(loader.getData());
		my_core.instructionLoader<core::Loader::instruction_type>(loader.getInstruction());

		my_core.run();
	}
	else
	{	
		core my_core{ loader.getSegments(), nullptr };

		my_core.dataLoader<core::Loader::data_type>(loader.getData());
		my_core.instructionLoader<core::Loader::instruction_type>(loader.getInstruction());

		my_core.run();
	}

	return;
}


// R_t,		// R_type ==> 2 source registers, 1 dest registers, no imm; Binary operations
// 
// I_t,		// I_type ==> 1 source register, 1 dest register, 1 imm; Binary operations, with imm
// 
// U_t,		// U_type ==> 1 source register, 1 dest register, 0 imm; Unary operations
// 
// S_t,		// S_type ==> 0 source register, 1 dest register, 0 imm (only for stack operations)
// 
// J_t		// J_type ==> 0 source register, 0 dest register 1 imm (only for branching)


// Opcode	//		Explanation			//		Example					//		Semantics
// ADD 0,	// add two operands			//		ADD  R1, R2, R3/imm		//		R1 <- R2 + R3/imm
// UMUL 1,	// multiple two operands	//		UMUL R1, R2, R3/imm		//		R1 <- R2 * R3/imm
// UDIV 2,	// divide two operands		//		UDIV R1, R2, R3/imm		//		R1 <- R2 / R3/imm
// UMOL 3,	//		op1 % op2			//		UMOL R1, R2, R3/imm		//		R1 <- R2 % R3/imm
//
// AND 4,	// bitwise And of A & B		//		AND  R1, R2, R3/imm		//		R1 <- R2 & R3/imm
// ORR 5,	// bitwise Or of A & B		//		ORR  R1, R2, R3/imm		//		R1 <- R2 | R3/imm
// XOR 6,	// bitwise Xor of A & B		//		XOR  R1, R2, R3/imm		//		R1 <- R2 ^ R3/imm
// SHL,		// logical shift left		//		SHL  R1, R2, R3/imm		//		R1 <- R2 << R3/imm
// SHR,		// logical shift right		//		SHR  R1, R2, R3/imm		//		R1 <- R2 >> R3/imm
// RTL,		// logical rotate left		//		RTL  R1, R2, R3/imm		//		R1 <- R2 <~ R3/imm
// RTR a,	// logical rotate right		//		RTR  R1, R2, R3/imm		//		R1 <- R2 ~> R3/imm
//
// NOT b,	// comp all the bits		//		NOT  R1, R2				//		R1 <- ~R2
//
// LDR c,	// load reg from mem		//		LDR  R1, R2				//		R1 <- [R2]
// STR d,	// store reg in mem			//		STR  R1, R2				//		[R1] <- R2
//
// PUSH e,	// push reg onto stack		//		PUSH R1					//		[SP - 4] <- R1;
// POP f,	// pop top ele into reg		//		POP	 R1					//		R1 <- [SP] + 4
//
// JMP 10,	//	unconditional jump		//		JMP label				//		N/A
// JZ 11,	// jump if Z flag is set	//		JZ	label				//		N/A
// JN 12,	// jump if N flag is set	//		JN	label				//		N/A
// JC 13,	// jump if C flag is set	//		JC	label				//		N/A
// JV 14,	// jump if V flag is set	//		JV	label				//		N/A
// JZN 15	// jump if Z or N is set	//		JZN label				//		N/A


// Basic layout:
// 1 0 9 8   7 6 5 4   3 2 1 0   9 8 7 6   5 4 3 2   1 0 9 8   7 6 5 4   3 2 1 0
// 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0
//					   \____/    \____/    \_____/   \_______________/   \_____/
// \_____________________|_/	   |		  |				  |				|
//			imm			 Rn		   Rm		  Rd			 OpCode		  OpType
