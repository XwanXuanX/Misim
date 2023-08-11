/**
* Assembly Abstract machine (Ver.2)
* 
* In the first attempt, I created my own instruction set and decoder, so that I can run
* a single process on a single-core CPU.
* 
* This is not enough... Here comes the second trial!
* 
* Second attempt objectives:
* 1. Contain all features version 1 has;
* 2. Introduce memory page and processes;
* 3. Still single-core CPU, but with multiple processes.
* 
* God bless me.
*/


// import absm.hpp to include all necessary headers
#include "absm.hpp"
#include <span>



//// define bit size of a byte
//static inline constexpr std::uint32_t byte_size = 8;
//
//struct Memory {
//	std::array<int, 100> mem_;
//};
//
//struct Page {
//	std::span<int, std::dynamic_extent> page_;
//};
//
//struct Process {
//	int pid;
//	std::vector<Page> pages;
//};
//
////struct Core {
////	struct StateSaver {
////
////	};
////
////	std::vector<Process> procs;
////	std::vector<StateSaver> ss;
////};
////
////struct Kernel {
////	std::array<Core, 4> cores;
////
////	std::vector<Process> proc;
////
////	Memory m;
////};
