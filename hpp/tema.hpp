/**
* Assembly Abstract machine (Ver.2)
* 
* In the first attempt, I created my own instruction set and decoder, so that I can run
* a single process on a single-core CPU.
* 
* This is not enough... Here comes the second trial!
* 
* God bless me.
*/


// import absm.hpp to include all necessary headers
#include "overture.hpp"
#include <expected>
#include <cassert>


#define NODISCARD [[nodiscard]]


/**
* Highly abstract overview:
* 
* CPU structures: Core x1, Kernel x1, RAM x1, Disk x1, Page xN, Process xN
* 
* Program layout:
*	class Page:				array<integer, size> page;
* 
*	class MainMemory:		stack<integer> empty_slots;
*							array<Page, size> memory;
* 
*	class SecondaryMemory:	vector<Page> disk;
*
*	class Process:			vector<Page*> frame;
* 
*	class Core:				class StateSaver;
*							Kernel* pkernel;
*							vector<Process*> procs;
*							unordered_map<PID, StateSaver> cpu_states;
* 
*	class Kernel:			TBD
* 
* Objectives:
*	1. Contain all features Ver.1 has
* 
*	2. Introducing page, main memory, secondary memory, process, cores, and kernel
* 
*	3. Allow page swapping and multiple processes on one core.
* 
*	4. C++20 std minimum!
* 
*	5. As thread-safe as possible!
*/


// -------------------------------------------------------
// 
// Page (Not Thread-safe)
// 
// -------------------------------------------------------


template <std::unsigned_integral T, std::size_t S>
class Page {
public:
	using page_width = T;
	using size_type = std::size_t;
	const size_type page_size = S;
	using page_model = std::array<page_width, S>;

	// force compiler generate default constructor
	NODISCARD constexpr explicit Page() = default;

	// ctor that set in_memory_ flag and start_addr_
	NODISCARD constexpr explicit Page(
		const bool in_memory, const size_type start_addr, const class Process* p_master) noexcept :
		page_{}, in_memory_{ in_memory }, start_addr_{ start_addr }, pMaster_{ p_master } {}

	// ctor that takes an array of elements and populate page
	template <class Arr>
		requires (std::same_as<page_model, std::remove_reference_t<Arr>>&& std::is_constructible_v<page_model, Arr>)
	NODISCARD constexpr explicit Page(
		Arr&& arr, const bool in_memory, const size_type start_addr, const class Process* p_master)
		noexcept(std::is_nothrow_constructible_v<page_model, Arr>) :
		page_{ std::forward<Arr>(arr) },
		in_memory_{ in_memory }, start_addr_{ start_addr }, pMaster_{ p_master } {}

	// check if addr in range
	constexpr auto inrange(const size_type addr) noexcept -> bool {
		return (addr >= start_addr_ && addr < start_addr_ + page_size);
	}

	// modify data in one slot
	constexpr auto write(const page_width data, const size_type addr) noexcept
		-> std::expected<void, std::domain_error> {
		if (not inrange(addr)) {
			return std::unexpected(std::domain_error("Error: addr not in range."));
		}
		page_[addr - start_addr_] = std::move(data);
		return std::expected<void, std::domain_error>{};
	}

	// read data in one slot
	NODISCARD constexpr auto read(const size_type addr) noexcept
		-> std::expected<page_width, std::domain_error> {
		if (not inrange(addr)) {
			return std::unexpected(std::domain_error("Error: addr not in range."));
		}
		return page_.at(addr - start_addr_);
	}

	// clear entire page to NULL
	constexpr auto clear() noexcept -> void { page_.fill(static_cast<page_width>(0x0)); }

	// clear sub-region to NULL
	constexpr auto clear(const size_type begin, const size_type end) noexcept
		-> std::expected<void, std::domain_error> {
		using return_t = std::expected<void, std::domain_error>;
		// if whole range, delegate to overload 1
		if (begin == start_addr_ and end == (start_addr_ + page_size - 1)) {
			clear();
			return return_t{};
		}
		if (not inrange(begin) or not inrange(end)) {
			return std::unexpected(std::domain_error("Error: begin | end not in range."));
		}
		for (auto idx{ begin }; idx <= end; ++idx) {
			page_[idx - start_addr_] = static_cast<page_width>(0x0);
		}
		return return_t{};
	}

	// Getters & Setters

	// return page width in # bits
	NODISCARD constexpr auto get_page_width() const noexcept -> std::size_t { return sizeof(page_width) * byte_size; }

	// return page size in length
	NODISCARD constexpr auto get_page_size() const noexcept -> size_type { return page_.max_size(); }

	// get start address
	NODISCARD constexpr auto get_start_addr() const noexcept -> size_type { return start_addr_; }

	// set start address
	constexpr void set_start_addr(const size_type start_addr) noexcept { start_addr_ = start_addr; }

	// get in memory
	NODISCARD constexpr bool in_memory() const noexcept { return in_memory_; }

	// set in memory
	constexpr void set_in_memory(const bool in) noexcept { in_memory_ = in; }

	// get owning process ptr
	NODISCARD constexpr auto get_pMaster() const noexcept -> class Process* { return pMaster_; }

	// set owning process ptr
	constexpr void set_pMaster(const class Process* p_master) noexcept { pMaster_ = p_master; }

private:
	page_model		page_{};
	bool			in_memory_{};		// if page in main memory
	size_type		start_addr_{};		// start address of frame

	// Cannot call any class method in Process class due to not seen full definition of Process.
	class Process*	pMaster_{};			// pointer to the owning process
};


// -------------------------------------------------------
// 
// MainMemory (Thread-safe)
// 
// -------------------------------------------------------


/**
* Design explanation:
* 
* Main memory is consists of an array of Pages. That is, memory OWNs Pages.
* Processes needs to request page from memory and borrow it. After the process is finished,
* pages will be released and returned to the memory.
*/



