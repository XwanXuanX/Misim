#include "abs_machine.hpp"
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
	
	using core = Core<std::uint32_t, 10>;
	core::segment seg = {
		{DS, std::make_pair(0, 2)},
		{ES, std::make_pair(3, 4)},
		{SS, std::make_pair(5, 6)},
		{CS, std::make_pair(7, 9)}
	};
	core c{ std::move(seg) };
	c.load_instr(0x6167efca, 0xdddeeec, 0xeeecaaa, 0x4, 0x6);
	c.load_data(0x9, 0xaefc8, 0x2, 0x3);
	auto mem = c.func();
	for (auto i : mem) {
		std::cout << i << ' ';
	}
	std::cout << '\n';
	std::cout << c.fetch() << ' ' << c.fetch() << '\n';
}

// Basic layout:
// 1 0 9 8   7 6 5 4   3 2 1 0   9 8 7 6   5 4 3 2   1 0 9 8   7 6 5 4   3 2 1 0
// 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0
//					   \____/    \____/    \_____/   \_______________/   \_____/
// \_____________________|_/	   |		  |				  |				|
//			imm			 Rn		   Rm		  Rd			 OpCode		  OpType
