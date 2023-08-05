#include "absm.hpp"
#include <iostream>

template <class ins>
void print_ints(ins Instr) {
	std::cout << std::hex;
	std::cout << (std::uint16_t)Instr.type_ << '\n';
	std::cout << (std::uint16_t)Instr.code_ << '\n';
	std::cout << (std::uint16_t)Instr.Rd_ << ' ' << (std::uint16_t)Instr.Rn_ << ' ' << (std::uint16_t)Instr.Rm_ << '\n';
	std::cout << (std::uint16_t)Instr.imm_;
}

int main() {
	//auto R = ALU::exec<std::uint8_t>(ALU_in<std::uint8_t>{
	//	.OpCode_ = ALU_OpCode::RTR, .Carryin_ = true, .Ops_ = { 0x1, 2 }});
	std::cout << std::hex;
	//std::cout << (std::uint16_t)R.Res_ << '\n';
	//for (auto&& p : R.Flags_) {
	//	std::cout << (std::uint16_t)p << ' ';
	//}
	//auto j = Decoder<>::decode<std::uint32_t>(0x32cae7cd);
	//print_ints(j);

	auto v = std::make_pair(1, 2);

	const std::pair<int, int>& a = v;
	
	using core = Core<std::uint32_t, 50, SyscallTable>;
	core::segment seg = {
		{DS, std::make_pair(31, 47)},
		{ES, std::make_pair(48, 48)},
		{SS, std::make_pair(25, 30)},
		{CS, std::make_pair(0, 24)}
	};
	Tracer t("C:\\Users\\lzlu1\\Desktop\\tracer.csv");
	core c{ std::move(seg), &t};
	c.load_data(
		0x123,
		0x234,
		0x345
	);
	c.load_instr(
		0x00111060,
		0x01f11001,
		0x00000060,
		0x000100c2,
		0x00111001,
		0x0ff00001,
		0x000100d2,
		0x000000e3,
		0x000010e3,
		0x000000f3,
		0x000010f3,
		0x00000164,
		0xffffffff
	);
	c.run();
}

//R_t,		// R_type ==> 2 source registers, 1 dest registers, no imm; Binary operations
////			  ADD Rd, Rs, Rm | MUL Rd, Rs, Rm
//I_t,		// I_type ==> 1 source register, 1 dest register, 1 imm; Binary operations, with imm
////			  ADD Rd, Rs, imm
//U_t,		// U_type ==> 1 source register, 1 dest register, 0 imm; Unary operations
////			  NEG Rd, Rs | NOT Rd, Rs | LDR Rd, Rs | STR Rd, Rs
//S_t,		// S_type ==> 0 source register, 1 dest register, 0 imm (only for stack operations)
////			  PUSH Rd | POP  Rd
//J_t			// J_type ==> 0 source register, 0 dest register 1 imm (only for branching)
//			//			  JMP imm | JEQ	imm


//// Opcode	//		Explanation			//		Example					//		Semantics
//ADD 0,		// add two operands			//		ADD  R1, R2, R3/imm		//		R1 <- R2 + R3/imm
//UMUL 1,		// multiple two operands	//		UMUL R1, R2, R3/imm		//		R1 <- R2 * R3/imm
//UDIV 2,		// divide two operands		//		UDIV R1, R2, R3/imm		//		R1 <- R2 / R3/imm
//UMOL 3,		//		op1 % op2			//		UMOL R1, R2, R3/imm		//		R1 <- R2 % R3/imm
//
//AND 4,		// bitwise And of A & B		//		AND  R1, R2, R3/imm		//		R1 <- R2 & R3/imm
//ORR 5,		// bitwise Or of A & B		//		ORR  R1, R2, R3/imm		//		R1 <- R2 | R3/imm
//XOR 6,		// bitwise Xor of A & B		//		XOR  R1, R2, R3/imm		//		R1 <- R2 ^ R3/imm
//SHL,		// logical shift left		//		SHL  R1, R2, R3/imm		//		R1 <- R2 << R3/imm
//SHR,		// logical shift right		//		SHR  R1, R2, R3/imm		//		R1 <- R2 >> R3/imm
//RTL,		// logical rotate left		//		RTL  R1, R2, R3/imm		//		R1 <- R2 <~ R3/imm
//RTR a,		// logical rotate right		//		RTR  R1, R2, R3/imm		//		R1 <- R2 ~> R3/imm
//
//NOT b,		// comp all the bits		//		NOT  R1, R2				//		R1 <- ~R2
//
//LDR c,		// load reg from mem		//		LDR  R1, R2				//		R1 <- [R2]
//STR d,		// store reg in mem			//		STR  R1, R2				//		[R1] <- R2
//
//PUSH e,		// push reg onto stack		//		PUSH R1					//		[SP - 4] <- R1;
//POP f,		// pop top ele into reg		//		POP	 R1					//		R1 <- [SP] + 4
//
//JMP 10,		//	unconditional jump		//		JMP label				//		N/A
//JZ 11,			// jump if Z flag is set	//		JZ	label				//		N/A
//JN 12,			// jump if N flag is set	//		JN	label				//		N/A
//JC 13,			// jump if C flag is set	//		JC	label				//		N/A
//JV 14,			// jump if V flag is set	//		JV	label				//		N/A
//JZN 15			// jump if Z or N is set	//		JZN label				//		N/A


// Basic layout:
// 1 0 9 8   7 6 5 4   3 2 1 0   9 8 7 6   5 4 3 2   1 0 9 8   7 6 5 4   3 2 1 0
// 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0
//					   \____/    \____/    \_____/   \_______________/   \_____/
// \_____________________|_/	   |		  |				  |				|
//			imm			 Rn		   Rm		  Rd			 OpCode		  OpType
