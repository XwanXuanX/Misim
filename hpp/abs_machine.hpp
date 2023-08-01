/*
* Assembly Abstract Machine
*
* A very simple, non-pipelined conceptual model of processor,
* that runs my own instruction set.
*/


#include <bit>
#include <array>
#include <utility>
#include <vector>
#include <cstdint>
#include <numeric>
#include <concepts>
#include <optional>
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <unordered_set>
#include <unordered_map>


// define bit size of a byte
constexpr std::uint32_t byte_size = 8;


// -------------------------------------------------------
//
// Memory
// 
// -------------------------------------------------------


template <std::unsigned_integral T, std::size_t S>
class memory {
public:
	using mem_width = T;
	using size_type = std::size_t;
	const size_type mem_size = S;
	using memory_model = std::array<mem_width, S>;

	// fource compiler generate default constructor
	[[nodiscard]] constexpr explicit memory() = default;

	// mutator: modify data in one mem slot
	constexpr auto write_slot(mem_width slot_data, size_type addr) noexcept -> bool {
		if (addr < 0 || addr >= mem_size) {
			return false;
		}
		mem_[addr] = std::move(slot_data);
		return true;
	}

	// accessor: read data in one mem slot
	[[nodiscard]] constexpr auto read_slot(size_type addr) noexcept -> std::optional<mem_width> {
		if (addr < 0 || addr >= mem_size) {
			return std::nullopt;
		}
		return mem_.at(addr);
	}

	// mutator: reset (clear) mem slots to NULL
	// overload 1: clear whole range
	constexpr auto clear_mem() noexcept -> bool {
		mem_.fill(static_cast<mem_width>(0x0));
		return true;
	}
	// overload 2: clear sub-region
	constexpr auto clear_mem(size_type begin_addr, size_type end_addr) noexcept -> bool {
		// if whole range, delegate to overload 1
		if (begin_addr == 0 && end_addr == mem_size - 1) {
			return clear_mem();
		}
		auto inrange = [end = mem_size](const auto& addr) -> bool {
			return addr >= 0 && addr < end;
		};
		if (!inrange(begin_addr) || !inrange(end_addr)) {
			return false;
		}
		for (; begin_addr <= end_addr; ++begin_addr) {
			mem_[begin_addr] = 0x0;
		}
		return true;
	}
	// overload 3: reset (clear) several mem slots to NULL (1 or more)
	template <typename T, typename... Args>
		requires std::same_as<T, size_type> && (std::same_as<T, Args> || ...)
	constexpr auto clear_mem(T slot, Args... slots) noexcept -> bool {
		bool succeed = write_slot(static_cast<mem_width>(0x0), slot);
		if constexpr (sizeof...(slots) > 0) {
			succeed &= clear_mem(slots...);
		}
		return succeed;
	}

	// accessor: return memory width in # bits
	[[nodiscard]] constexpr auto get_mem_nbit() const noexcept -> std::uint32_t {
		return sizeof(mem_width) * byte_size;
	}

	// accessor: return memory size in length
	[[nodiscard]] constexpr auto get_mem_size() const noexcept -> size_type {
		return mem_.max_size();
	}

private:
	memory_model mem_{};
};


// -------------------------------------------------------
//
// Free functions
// 
// -------------------------------------------------------


// one can simply use <bit> stl.
// here implement some necessary operations that <bit> does not have

// bit in range: test if the given bit position is within the range
// NOTE: POS is the index of the bit, from 0, start from right (least significant)
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_inrange(const std::size_t pos) noexcept -> bool {
	return pos >= 0 && pos < sizeof(Int) * byte_size;
}

// bit test: test if a bit is set to true
// NOTE: POS is the index of the bit, from 0, start from right (least significant)
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_test(Int n, std::size_t pos) noexcept -> bool {
	if (!bit_inrange<Int>(pos)) {
		return false;
	}
	// 1 << pos ==> 0b0000 0001 -> 0b0010 0000
	// n & (1 << pos) ==> 0b0101 1101 & 0b0010 0000 -> 0b0000 0000 -> false
	return n & (static_cast<Int>(1) << pos);
}

// bit_test_all: test if all the bit is set to true
// NOTE: last_nbit is the number of bits to be tested, start from right (least significant)
// overload 1: entire range
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_test_all(Int n) noexcept -> bool {
	std::size_t last_nbit{ sizeof(Int)* byte_size - 1 };
	bool res{ true };
	// prevent from overflow
	if (sizeof(Int) == sizeof(std::uintmax_t)) {
		res &= bit_test<Int>(n, last_nbit);	// check the highest bit
		n >>= 1;		// move the bits down by 1
		--last_nbit;	// dec the check range
	}
	const Int mask{ (static_cast<std::uintmax_t>(1) << (last_nbit + 1)) - 1 };
	n &= mask;
	return n == mask && res;
}
// overload 2: user provided range
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_test_all(Int n, std::size_t last_nbit) noexcept -> bool {
	if (!bit_inrange<Int>(last_nbit)) {
		return false;
	}
	// if check whole range, delegate to overload 1
	if (!bit_inrange<Int>(last_nbit + 1)) {
		return bit_test_all<Int>(n);
	}
	// not whole range, no need to worry about overflow
	// (1 << (last_nbit + 1)) - 1 ==> 0b0001 -> 0b0000 - 1 -> 0b1111 for last_nbit = 3
	// n &= mask ==> 0b1101 & 0b1111 -> 0b1101 / 0b1111 & 0b1111 / 0b0000 & 0b1111 -> 0b0000
	const Int mask{ (static_cast<std::uintmax_t>(1) << (last_nbit + 1)) - 1 };
	n &= mask;
	return n == mask;
}

// bit_test_any: test if any bits are set to true
// NOTE: last_nbit is the number of bits to be tested, start from right (least significant)
// overload 1: entire range
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_test_any(Int n) noexcept -> bool {
	return static_cast<bool>(n);
}
// overload 2: user provided range
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_test_any(Int n, std::size_t last_nbit) noexcept -> bool {
	if (!bit_inrange<Int>(last_nbit)) {
		return false;
	}
	// if testing for the whole interval, delegate to overload 1
	if (!bit_inrange<Int>(last_nbit + 1)) {
		return bit_test_any<Int>(n);
	}
	// not whole range, not need to worry about overflow
	const Int mask{ (static_cast<std::uintmax_t>(1) << (last_nbit + 1)) - 1 };
	n &= mask;
	return static_cast<bool>(n);
}

// bit_test_none: test if none of the bits are set
// NOTE: last_nbit is the number of bits to be tested, start from right (least significant)
// overload 1: entire range
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_test_none(Int n) noexcept -> bool {
	return !bit_test_any<Int>(n);
}
// overload 2: user provided range
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_test_none(Int n, std::size_t last_nbit) noexcept -> bool {
	return !bit_test_any<Int>(n, last_nbit);
}

// bit_set: set bits to true
// NOTE: POS is the index of the bit, from 0, start from right (least significant)
// overload 1: user provided location
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_set(Int n, const std::size_t pos) noexcept -> std::optional<Int> {
	if (!bit_inrange<Int>(pos)) {
		return std::nullopt;
	}
	return n | (static_cast<Int>(1) << pos);
}
// overload 2: entire range
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_set(Int n) noexcept -> std::optional<Int> {
	return n | static_cast<Int>(-1);
}
// helper validate concept
template <typename P, typename... Ps>
concept is_valid_poses = std::same_as<P, std::size_t> && (std::same_as<P, Ps> || ...);
// overload 3: user provide multiple location
template <typename Int, typename P, typename... Ps>
	requires std::unsigned_integral<Int>&& is_valid_poses<P, Ps...>
[[nodiscard]] constexpr auto bit_set(Int n, P pos, Ps... poses) noexcept -> std::optional<Int> {
	std::optional<Int> res{bit_set<Int>(n, pos)};
	if (!res.has_value()) {
		return res;
	}
	if constexpr (sizeof...(poses) > 0) {
		res = bit_set(res.value(), poses...);
	}
	return res;
}

// bit_reset: set bits to false
// NOTE: POS is the index of the bit, from 0, start from right (least significant)
// overload 1: user provided location
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_reset(Int n, const std::size_t pos) noexcept -> std::optional<Int> {
	if (!bit_inrange<Int>(pos)) {
		return std::nullopt;
	}
	// check if the bit is already false
	if (!bit_test<Int>(n, pos)) {
		return n;
	}
	// otherwise need to use xor to toggle the bit
	return n ^ (static_cast<Int>(1) << pos);
}
// overload 2: entire range
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_reset(Int n) noexcept -> std::optional<Int> {
	return n & static_cast<Int>(0x0);
}
// overload 3: user provide multiple location
template <typename Int, typename P, typename... Ps>
	requires std::unsigned_integral<Int>&& is_valid_poses<P, Ps...>
[[nodiscard]] constexpr auto bit_reset(Int n, P pos, Ps... poses) noexcept -> std::optional<Int> {
	std::optional<Int> res{bit_reset<Int>(n, pos)};
	if (!res.has_value()) {
		return res;
	}
	if constexpr (sizeof...(poses) > 0) {
		res = bit_reset(res.value(), poses...);
	}
	return res;
}

// utility function: change all integral type to std::size_t for use in POS
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto pos(Int n) noexcept -> std::size_t {
	return static_cast<std::size_t>(n);
}

// bit_flip: flip bits to their opposite
// NOTE: POS is the index of the bit, from 0, start from right (least significant)
// overload 1: user provided location
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_flip(Int n, const std::size_t pos) noexcept -> std::optional<Int> {
	if (!bit_inrange<Int>(pos)) {
		return std::nullopt;
	}
	return n ^ (static_cast<Int>(1) << pos);
}
// overload 2: entire ranges
template <std::unsigned_integral Int>
[[nodiscard]] constexpr auto bit_flip(Int n) noexcept -> std::optional<Int> {
	return ~n;
}
// overload 3: user provided multiple ranges
template <typename Int, typename P, typename... Ps>
	requires std::unsigned_integral<Int>&& is_valid_poses<P, Ps...>
[[nodiscard]] constexpr auto bit_flip(Int n, P pos, Ps... poses) noexcept -> std::optional<Int> {
	std::optional<Int> res{bit_flip<Int>(n, pos)};
	if (!res.has_value()) {
		return res;
	}
	if constexpr (sizeof...(poses) > 0) {
		res = bit_flip(res.value(), poses...);
	}
	return res;
}

// The solution to solve problem with multiply invokes undefined behaviour
// Explanation:
// Define A & B be unsigned integers, whose size less than signed int (normally 32bits)
// If the product of A * B overflows, A & B will be promoted to signed int and multiply.
// However, if the result overflows, signed int will invoke undefined behaviour.
// Solution:
// Use template method to let unsigned integers only promote to unsigned int.
// After the operation is done, truncated the result to original size.

template <std::unsigned_integral T>
struct make_promoted;

// unsigned integers will be promoted to "unsigned int" or to themselves,
// if their rank is already greater or equal to the rank of "unsigned int".
template <std::unsigned_integral T>
struct make_promoted {
	static constexpr auto small = sizeof(T) < sizeof(unsigned int);
	using type = std::conditional_t<small, unsigned int, T>;
};

template <class T>
using make_promoted_t = typename make_promoted<T>::type;

// helper function
template <class T>
[[nodiscard]] constexpr auto promote(T v) noexcept -> make_promoted_t<T> {
	return v;
}


// -------------------------------------------------------
//
// Register file
// 
// -------------------------------------------------------


// total number of registers are 16, including:
// R0 - R12 ==> General purpose register
// R13 / SP ==> Stack pointer register
// R14 / LR ==> Link register (return address)
// R15 / PC ==> Program counter register

// special registers
// PSR		==> Program state register (flags)

// Segment registers
// CS		==> Code segment starting address
// DS		==> Data segment starting address
// SS		==> Stack segment starting address
// ES		==> Extra segment starting address

// bit-width of these 16 registers and Segment registers can be customized to 32 bits or 64 bits
// PSR stays 8 bits wide

enum GPReg : std::uint8_t {
	// general purpose registers
	R0 = 0,	// index starting from 0
	R1,
	R2,
	R3,
	R4,
	R5,
	R6,
	R7,
	R8,
	R9,
	R10,
	R11,
	R12,
	// special purpose registers
	SP,		// Stack Pointer register
	LR,		// Link register
	PC		// Program counter
};

enum SEGReg : std::uint8_t {
	CS,		// Code Segment
	DS,		// Data Segment
	SS,		// Stack Segment
	ES		// Extra Segment
};

enum PSRReg : std::uint8_t {
	N,	// Negative or less than flag
	Z,	// Zero flag
	C,	// Carry/borrow flag
	V	// Overflow flag
};

template <std::unsigned_integral T>
class Registers {
public:
	using psr_width = std::uint8_t;
	using gp_width  = T;

	using psr_model = psr_width;
	using gp_model  = std::array<gp_width, 16>;

	// default initialize all register content to 0
	// initialize registers to a certain value is the job of CPU
	[[nodiscard]] constexpr explicit Registers() noexcept = default;

	// ---------------------------------------------------
	// General Purpose Registers
	// ---------------------------------------------------

	// overload 1: without const
	[[nodiscard]] constexpr auto GP(std::uint8_t RegName) noexcept -> gp_width& {
		return gp_[RegName];
	}
	// overload 2: const overload
	[[nodiscard]] constexpr auto GP(std::uint8_t RegName) const noexcept -> gp_width {
		return gp_.at(RegName);
	}

	// ---------------------------------------------------
	// Program Status Register
	// ---------------------------------------------------

	// specify which bit flag to test
	[[nodiscard]] constexpr auto PSR(std::uint8_t FlagName) const noexcept -> bool {
		return bit_test<psr_width>(psr_, FlagName);
	}
	// set the value of the given PSR bit
	constexpr auto PSR(std::uint8_t FlagName, const bool v) noexcept -> bool {
		auto t = (v) ? \
			bit_set<psr_width>(psr_, FlagName) :
			bit_reset<psr_width>(psr_, FlagName);
		if (!t.has_value()) {
			return false;
		}
		psr_ = t.value();
		return true;
	}
	// get the value of the PSR
	[[nodiscard]] constexpr auto get_psr_value() const noexcept -> psr_width {
		return psr_;
	}
	// clear the value of the PSR
	constexpr auto clear_psr() noexcept -> void {
		psr_ = static_cast<psr_model>(0x0);
	}

private:
	gp_model gp_{};
	psr_model psr_{};
};


// -------------------------------------------------------
//
// ALU
// 
// -------------------------------------------------------


/**
* ALU is a black box, it takes in a input, and spit the output;
*						   ______________
*						  |				 |
*	Operands -----------> |		ALU		 | -------------> Output
*						  |______________|
*
*/

// define all possible operations the ALU can handle
enum struct ALU_OpCode : std::uint8_t {
	// Arithmetic operations
	ADD,	// Add two operands
	UMUL,	// Multiple A & B
	UDIV,	// Divide A & B
	UMOL,	// Take the modulus (%)
	PASS,	// Pass through one operand without change

	// Bitwise logical operations
	AND,	// Bitwise And of A & B
	ORR,	// Bitwise Or of A & B
	XOR,	// Bitwise Xor of A & B
	COMP,	// One's compliment of A / B

	// Bit shift operations
	SHL,	// Logical shift left
	SHR,	// Logical shift right
	RTL,	// Rotate left
	RTR,	// Rotate right
};

// the input to the ALU
template <std::unsigned_integral S>
struct ALU_in {
	using op_size = S;

	ALU_OpCode					OpCode_;	// the Opcode the ALU will operate in
	std::pair<op_size, op_size> Ops_;		// A & B two operands
};

// the output to the ALU
template <std::unsigned_integral S>
struct ALU_out {
	using op_size = S;
	using flag_type = std::unordered_set<std::uint8_t>;

	flag_type			Flags_;		// The PSR flags the operation produces
	op_size				Res_;		// The result of the operation
};

// define special concept used later
template <class F, class T>
concept is_valid_callback = std::is_invocable_r_v<bool, F, T, T, T>;

// complete namespace class - pure static functions
class ALU {
public:
	// ---------------------------------------------
	// Factory function
	// 
	// Dynamic dispatch
	// ---------------------------------------------

	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto exec(ALU_in<T> alu_in) noexcept -> ALU_out<T> {
		using op_size = typename ALU_in<T>::op_size;
		std::pair<op_size, op_size> AB = alu_in.Ops_;
		switch (alu_in.OpCode_) {
			case ALU_OpCode::ADD:		return ALU::ADD(AB.first, AB.second);
			case ALU_OpCode::UMUL:		return ALU::UMUL(AB.first, AB.second);
			case ALU_OpCode::UDIV:		return ALU::UDIV(AB.first, AB.second);
			case ALU_OpCode::UMOL:		return ALU::UMOL(AB.first, AB.second);
			case ALU_OpCode::PASS:		return ALU::PASS(AB.first);

			case ALU_OpCode::AND:		return ALU::AND(AB.first, AB.second);
			case ALU_OpCode::ORR:		return ALU::ORR(AB.first, AB.second);
			case ALU_OpCode::XOR:		return ALU::XOR(AB.first, AB.second);
			case ALU_OpCode::COMP:		return ALU::COMP(AB.first);

			case ALU_OpCode::SHL:		return ALU::SHL(AB.first, AB.second);
			case ALU_OpCode::SHR:		return ALU::SHR(AB.first, AB.second);
			case ALU_OpCode::RTL:		return ALU::RTL(AB.first, AB.second);
			case ALU_OpCode::RTR:		return ALU::RTR(AB.first, AB.second);

			// NO DEFAULT BRANCH!
			// The compiler will warn about OpCode missuse
		}
		// dummy return value, to eliminate compiler warning.
		return ALU_out<T>{};
	}

private:
	// ---------------------------------------------
	// All underlying implemetation details
	// ALU_out is very light-weight;
	// Feel free to pass / return by value.
	// ---------------------------------------------

	// 0. PSR setter (result only | with-preset flags)
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto get_psf(T R, std::unordered_set<std::uint8_t>& flags)
		noexcept -> void {
		// only from the result, we can get information about Z and N
		// if the first bit (sign bit) is set, then set N
		if (bit_test<T>(R, sizeof(T) * byte_size - 1)) {
			flags.insert(N);
		}
		// if the result is equal to 0, then set Z
		if (bit_test_none<T>(R) && R == 0x0) {
			flags.insert(Z);
		}
	}

	// 0. PSR setter (result only | non-preset flags)
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto get_psf(T R) noexcept
		-> std::unordered_set<std::uint8_t> {
		std::unordered_set<std::uint8_t> flags{};
		get_psf<T>(R, flags);
		return flags;
	}

	// 0. PSR setter (result and operands)
	template <std::unsigned_integral T, is_valid_callback<T> F>
	[[nodiscard]] static constexpr auto get_psf(
		T A, T B, T R,	// operands & results
		F CC,	// check carry flag
		F CV	// check overflow flag
	) noexcept -> std::unordered_set<std::uint8_t> {
		std::unordered_set<std::uint8_t> flags{};
		// use CC and CV to check if C or V flag should be set
		if (CC(A, B, R)) {
			flags.insert(C);
		}
		if (CV(A, B, R)) {
			flags.insert(V);
		}
		// for the rest we can delegate to the other overload
		get_psf<T>(R, flags);
		return flags;
	}

	// 0.5. Make ALU output
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto make_ALUOut(T R) noexcept -> ALU_out<T> {
		std::unordered_set<std::uint8_t> flags = ALU::get_psf<T>(R);
		return ALU_out<T> {.Flags_ = std::move(flags), .Res_ = R};
	}

	// 1. ADD - Add two operands
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto ADD(T A, T B) noexcept -> ALU_out<T> {
		T R = A + B;
		std::unordered_set<std::uint8_t> flags = ALU::get_psf<T, std::function<bool(T, T, T)>>(
			A, B, R,
			// CC - Carry
			[](T A, T B, T R) noexcept -> bool { return (R < A && R < B) ? true : false; },
			// CV - Overflow
			[](T A, T B, T R) noexcept -> bool {
				std::size_t msb{sizeof(T)* byte_size - 1};
				// if A & B's msb differ, then we are safe
				if (bit_test<T>(A, msb) ^ bit_test<T>(B, msb)) {
					return false;
				}
				// check if A | B's msb differ from R's msb
				return (bit_test<T>(A, msb) ^ bit_test<T>(R, msb)) ? true : false;
			}
		);
		// construct ALU_out with flags and R
		return ALU_out<T> {.Flags_ = std::move(flags), .Res_ = R};
	}

	// 2. UMUL - Unsigned Multiple A & B
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto UMUL(T A, T B) noexcept ->  ALU_out<T> {
		// EXPERIMENTAL! NO CV FLAGS WILL BE DETECTED!
		// take care of the issue with integer promotion
		return ALU::make_ALUOut<T>(static_cast<T>(promote<T>(A) * promote<T>(B)));
	}

	// 3. UDIV - Unsigned Divide A & B
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto UDIV(T A, T B) noexcept ->  ALU_out<T> {
		// take care of B is 0x00, undefined behaviour
		if (bit_test_none<T>(B) && B == 0x0) {
			return ALU_out<T>{};
		}
		return ALU::make_ALUOut<T>(A / B);
	}

	// 4. UMOL - Take the modulus (%) of Unsigned integers
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto UMOL(T A, T B) noexcept ->  ALU_out<T> {
		// take care of B is 0x00, undefined behaviour
		if (bit_test_none<T>(B) && B == 0x0) {
			return ALU_out<T>{};
		}
		return ALU::make_ALUOut<T>(A % B);
	}

	// 5. PASS - Pass through one operand without change
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto PASS(T A) noexcept ->  ALU_out<T> { return ALU::make_ALUOut<T>(A); }

	// 6. AND - Bitwise And of A & B
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto AND(T A, T B) noexcept ->  ALU_out<T> { return ALU::make_ALUOut<T>(A & B); }

	// 7. ORR - Bitwise Or of A & B
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto ORR(T A, T B) noexcept ->  ALU_out<T> { return ALU::make_ALUOut<T>(A | B); }

	// 8. XOR - Bitwise Xor of A & B
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto XOR(T A, T B) noexcept ->  ALU_out<T> { return ALU::make_ALUOut<T>(A ^ B); }

	// 9. COMP - One's compliment of A
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto COMP(T A) noexcept ->  ALU_out<T> {
		std::optional<T> R = bit_flip<T>(A);
		return (R.has_value()) ? ALU::make_ALUOut<T>(R.value()) : ALU_out<T>{};
	}

	// 10. SHL - Logical shift left
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto SHL(T A, std::size_t s) noexcept ->  ALU_out<T> {
		return ALU::make_ALUOut<T>(A << s);
	}

	// 11. SHR - Logical shift right
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto SHR(T A, std::size_t s) noexcept ->  ALU_out<T> {
		return ALU::make_ALUOut<T>(A >> s);
	}

	// 12. RTL - Rotate left
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto RTL(T A, std::size_t s) noexcept ->  ALU_out<T> {
		return ALU::make_ALUOut<T>(std::rotl<T>(A, static_cast<int>(s)));
	}

	// 13. RTR - Rotate right
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto RTR(T A, std::size_t s) noexcept ->  ALU_out<T> {
		return ALU::make_ALUOut<T>(std::rotr<T>(A, static_cast<int>(s)));
	}
};


// -------------------------------------------------------
// 
// Decoder
// 
// -------------------------------------------------------


// 5 types of instructions:
enum OpType : std::uint8_t {
	R_t,		// R_type ==> 2 source registers, 1 dest registers, no imm; Binary operations
				//			  ADD Rd, Rs, Rm | MUL Rd, Rs, Rm
	I_t,		// I_type ==> 1 source register, 1 dest register, 1 imm; Binary operations, with imm
				//			  ADD Rd, Rs, imm
	U_t,		// U_type ==> 1 source register, 1 dest register, 0 imm; Unary operations
				//			  NEG Rd, Rs | NOT Rd, Rs | LDR Rd, Rs | STR Rd, Rs
	S_t,		// S_type ==> 0 source register, 1 dest register, 0 imm (only for stack operations)
				//			  PUSH Rd | POP  Rd
	J_t			// J_type ==> 0 source register, 0 dest register 1 imm (only for branching)
				//			  JMP imm | JEQ	imm
};

enum OpCode : std::uint8_t {
	// Opcode	//		Explanation			//		Example					//		Semantics
	ADD,		// add two operands			//		ADD  R1, R2, R3/imm		//		R1 <- R2 + R3/imm
	UMUL,		// multiple two operands	//		UMUL R1, R2, R3/imm		//		R1 <- R2 * R3/imm
	UDIV,		// divide two operands		//		UDIV R1, R2, R3/imm		//		R1 <- R2 / R3/imm
	UMOL,		//		op1 % op2			//		UMOL R1, R2, R3/imm		//		R1 <- R2 % R3/imm

	AND,		// bitwise And of A & B		//		AND  R1, R2, R3/imm		//		R1 <- R2 & R3/imm
	ORR,		// bitwise Or of A & B		//		ORR  R1, R2, R3/imm		//		R1 <- R2 | R3/imm
	XOR,		// bitwise Xor of A & B		//		XOR  R1, R2, R3/imm		//		R1 <- R2 ^ R3/imm
	SHL,		// logical shift left		//		SHL  R1, R2, R3/imm		//		R1 <- R2 << R3/imm
	SHR,		// logical shift right		//		SHR  R1, R2, R3/imm		//		R1 <- R2 >> R3/imm
	RTL,		// logical rotate left		//		RTL  R1, R2, R3/imm		//		R1 <- R2 <~ R3/imm
	RTR,		// logical rotate right		//		RTR  R1, R2, R3/imm		//		R1 <- R2 ~> R3/imm

	NOT,		// comp all the bits		//		NOT  R1, R2				//		R1 <- ~R2

	LDR,		// load reg from mem		//		LDR  R1, R2				//		R1 <- [R2]
	STR,		// store reg in mem			//		STR  R1, R2				//		[R1] <- R2

	PUSH,		// push reg onto stack		//		PUSH R1					//		[SP - 4] <- R1;
	POP,		// pop top ele into reg		//		POP	 R1					//		R1 <- [SP] + 4

	JMP,		//	unconditional jump		//		JMP label				//		N/A
	JZ,			// jump if Z flag is set	//		JZ	label				//		N/A
	JN,			// jump if N flag is set	//		JN	label				//		N/A
	JC,			// jump if C flag is set	//		JC	label				//		N/A
	JV,			// jump if V flag is set	//		JV	label				//		N/A
	JZN			// jump if Z or N is set	//		JZN label				//		N/A
};

// Encoding rules
// It's the encoding designer's obligation to make sure all info can be fit in min_width
struct default_encoding final {
	using min_width = std::uint32_t;				// minimum instruction length
	using idx_len_t = std::uint8_t;					// type used to encode idx and len
	using field = std::pair<idx_len_t, idx_len_t>;	// field type

	// Basic layout:
	// 1 0 9 8   7 6 5 4   3 2 1 0   9 8 7 6   5 4 3 2   1 0 9 8   7 6 5 4   3 2 1 0
	// 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0 , 0 0 0 0
	//					   \____/    \____/    \_____/   \_______________/   \_____/
	// \_____________________|_/	   |		  |				  |				|
	//			imm			 Rn		   Rm		  Rd			 OpCode		  OpType

	// The encoding format is as follows: pair<start idx, nbit_len>
	// Encoding as follows
	static constexpr field op_type_{ 0 , 4 };
	static constexpr field op_code_{ 4 , 8 };
	static constexpr field		Rd_{ 12, 4 };
	static constexpr field		Rm_{ 16, 4 };
	static constexpr field		Rn_{ 20, 4 };	// Note: Rn and imm share the same 4 bits
	static constexpr field	   imm_{ 20, 12};	// Allowed since Rn is optional
};

// -------------------------------------------------------
// ^^^ Encoding Rules			Implementations vvv
// -------------------------------------------------------

template <class E>
concept is_valid_encoding = requires {
	typename E::min_width;
	typename E::idx_len_t;
	std::convertible_to<typename E::idx_len_t, std::uint32_t>;
	typename E::field;
	std::same_as<typename E::field,
		std::pair<typename E::idx_len_t, typename E::idx_len_t>>;
}&& requires {
	E::op_type_; E::op_code_; E::Rd_; E::Rm_; E::Rn_; E::imm_;
};

template <is_valid_encoding E>
struct Instr {
private:
	using u8 = std::uint8_t;
	using u16 = std::uint16_t;
	using u32 = std::uint32_t;

public:
	// template metaprogramming technique here:
	// automatically select the best-fitting uint types for each field
	// OpType_type
	using Tt = std::conditional_t<(E::op_type_.second <= sizeof(u8) * byte_size), u8,
		std::conditional_t<(E::op_type_.second <= sizeof(u16) * byte_size), u16, u32>>;
	// OpCode_type
	using Ct = std::conditional_t<(E::op_code_.second <= sizeof(u8) * byte_size), u8,
		std::conditional_t<(E::op_code_.second <= sizeof(u16) * byte_size), u16, u32>>;
	// Register_type
	static_assert(E::Rd_.second == E::Rm_.second && E::Rm_.second == E::Rn_.second);
	using Rt = std::conditional_t<(E::Rd_.second <= sizeof(u8) * byte_size), u8,
		std::conditional_t<(E::Rd_.second <= sizeof(u16) * byte_size), u16, u32>>;
	// Immediate_type
	using It = std::conditional_t<(E::imm_.second <= sizeof(u8) * byte_size), u8,
		std::conditional_t<(E::imm_.second <= sizeof(u16) * byte_size), u16, u32>>;

	Tt			type_;		// opcode type (R I U S J)
	Ct			code_;		// opcode
	Rt  Rd_, Rn_, Rm_;		// dest & source registers
	It			 imm_;		// immediate value
};

// The main decoder class
template <is_valid_encoding E = default_encoding>
class Decoder {
public:
	using encoding = E;
	using instr_type = Instr<encoding>;

	template <std::unsigned_integral T>
		requires(sizeof(T) >= sizeof(typename encoding::min_width))
	[[nodiscard]] static constexpr auto decode(T instr) noexcept -> instr_type {
		using Rt = typename instr_type::Rt;
		return instr_type{
			.type_ = Decoder::extract<T, typename instr_type::Tt>(instr, encoding::op_type_),
			.code_ = Decoder::extract<T, typename instr_type::Ct>(instr, encoding::op_code_),
			.Rd_ = Decoder::extract<T, Rt>(instr, encoding::Rd_),
			.Rn_ = Decoder::extract<T, Rt>(instr, encoding::Rn_),
			.Rm_ = Decoder::extract<T, Rt>(instr, encoding::Rm_),
			.imm_ = Decoder::extract<T, typename instr_type::It>(instr, encoding::imm_)
		};
	}

private:
	template <std::unsigned_integral T>
	[[nodiscard]] static constexpr auto make_mask(std::size_t s) noexcept -> T {
		// first turn all the field on; then shift right to obtain the mask
		// this strategy will eliminates out of range operation
		T m = bit_set<T>(static_cast<T>(0x0)).value();
		if (s > sizeof(T) * byte_size) {
			return m;
		}
		return m >> (sizeof(T) * byte_size - s);
	}

	template <std::unsigned_integral T, std::unsigned_integral Conv>
	[[nodiscard]] static constexpr auto extract(T instr, typename encoding::field fld) noexcept -> Conv {
		instr >>= fld.first;
		return static_cast<Conv>(instr & Decoder::make_mask<T>(fld.second));
	}
};


// -------------------------------------------------------
// 
// CPU Core
// 
// -------------------------------------------------------


/*
* In this abstract machine, Core is consists of: memory + registerfile + ALU + decoder + fetch
* 1. Config & Init: Configure memory size, define segment registers, initialize GP registerfile;
* 2. Load Data & Ins: Load static-init data and instructions in corresponding segments;
* ->	3. Fetch Ins: fetch the instruction from .code seg, inc PC by 1;
* |		4. Decode Ins: decode binary instruction, output Instr-struct;
* |		5. Execute: generate ALU_in -> feed ALU -> get ALU_out -> update PSR flags;
* |		6. Mem Access: access memory if needed;
* |		7. Write Back: write the result back to register file.
* `<-	8. Retire the instruction.
*/

template <typename Sy, typename T, typename... Ts>
concept is_valid_data = std::convertible_to<T, Sy> && (std::same_as<T, Ts> || ...);

template <std::unsigned_integral B, std::size_t S>
class Core {
public:
	using sysb = B;						// system bit
	const std::size_t mem_size = S;		// memory size

	// components
	using memory	= memory<sysb, S>;
	using registers = Registers<sysb>;
	using decoder	= Decoder<default_encoding>;
	using alu		= ALU;

	// sub-components
	using instruct  = typename decoder::instr_type;
	using alu_in	= ALU_in<sysb>;
	using alu_out	= ALU_out<sysb>;

	// helper definition
	using segment = std::unordered_map<SEGReg, std::pair<sysb, sysb>>;

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
		if (!std::ranges::all_of(seg_regs, [&seg_config](SEGReg sr) -> bool {return seg_config.contains(sr);})) {
			return false;
		}
		// segment overlap is not allowed!
		auto overlap = []<typename T>(const std::pair<T, T>& r1, decltype(r1) r2) -> bool {
			return r1.first <= r2.second && r2.first <= r1.second;
		};
		using value_type = typename segment::mapped_type;
		std::vector<value_type> rngs{};
		rngs.reserve(4);
		for (auto&& r : seg_config) {
			rngs.emplace_back(r.second);
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
			[](sysb&& i, const value_type& p) {return i + (p.second - p.first) + 1;});
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

	template <typename SG> requires std::same_as<segment, std::remove_reference_t<SG>>
	[[nodiscard]] constexpr explicit Core(SG&& seg_config) {
		if (!this->init(std::forward<SG>(seg_config))) {
			throw std::runtime_error("Error: Failed to initialize segments!");
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
				continue;
			}
			auto result = execute(inst);
			mem_access(inst, result);
		}
	}

private:
	// fetch unit
	[[nodiscard]] constexpr auto fetch() -> sysb {
		if (!Core::in_range<sysb>(registers_.GP(PC), segments_.at(CS))) {
			throw std::runtime_error("Error: PC exceeds CS boundary!");
		}
		if (auto instr = memory_.read_slot(registers_.GP(PC)); instr.has_value()) {
			++registers_.GP(PC);
			return instr.value();
		}
		throw std::runtime_error("Error: Failed to read instruction from memory!");
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
			} else {
				throw std::runtime_error("Error: Invalid memory access!");
			}
			break;
		} 
		case STR: {
			if (!memory_.write_slot(registers_.GP(instr.Rd_), v)) {
				throw std::runtime_error("Error: Invalid memory access!");
			}
			break;
		}
		case PUSH: {
			if (!Core::in_range(v, segments_.at(SS))) {
				throw std::runtime_error("Error: Stackoverflow!");
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
			} else {
				throw std::runtime_error("Error: Invalid memory access!");
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
			case JMP:	perform_jump(true,				ins.imm_);		break;
			case JZ:	perform_jump(registers_.PSR(Z), ins.imm_);		break;
			case JN:	perform_jump(registers_.PSR(N), ins.imm_);		break;
			case JC:	perform_jump(registers_.PSR(C), ins.imm_);		break;
			case JV:	perform_jump(registers_.PSR(V), ins.imm_);		break;
			case JZN:	perform_jump(registers_.PSR(Z) || 
									 registers_.PSR(N), ins.imm_);		break;
			default:	throw std::runtime_error("Error: Unrecoganized instruction type detected!");
		}
		return true;
	}

	// generate alu_in based on the instruction
	constexpr auto gen_ALUIn(const instruct& ins) const -> alu_in {
		// J-type instruction should not go through
		if (ins.type_ == J_t) {
			throw std::runtime_error("Error: Jump-type instruction fall through!");
		}
		// lambda function to help create ALU input for R & I type instruction
		auto make_ALUIn_RI = [this](const instruct & ins, ALU_OpCode op) -> alu_in {
			switch (ins.type_) {
				case R_t:	return make_ALUIn<sysb>(op, registers_.GP(ins.Rm_), registers_.GP(ins.Rn_));
				case I_t:	return make_ALUIn<sysb>(op, registers_.GP(ins.Rm_), ins.imm_);
				default:	throw std::runtime_error("Error: Unrecoganized instruction type detected!");
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
			case POP:		return make_ALUIn<sysb>(ALU_OpCode::ADD, registers_.GP(SP),  1);
			default:		throw std::runtime_error("Error: Unrecoganized instruction type detected!");
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
};
