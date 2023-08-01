/*
* Utilities
* 
* For logging and testing
*/


#include <iostream>
#include <fstream>
#include <concepts>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <functional>


// -------------------------------------------------------
//
// Logging System
// 
// -------------------------------------------------------

// critical levels
enum struct LogCriticalLvls : std::int8_t {
	DEBUG,
	INFO,
	WARNING,
	ERROR
};

// preset formatter class for logging class
class PresetFormatter {
public:
	[[nodiscard]] std::string_view GetFormat(const LogCriticalLvls lvl) const noexcept(false) {
		// will potentially throw out_of_range error.
		return m_map_.at(lvl);
	}

private:
	const std::unordered_map<LogCriticalLvls, std::string_view> m_map_{
		{LogCriticalLvls::DEBUG,   "\033[1;37mDEBUG\033[0m"},   // White
		{LogCriticalLvls::INFO,    "\033[1;32mINFO\033[0m" },   // Green
		{LogCriticalLvls::WARNING, "\033[1;33mWARNING\033[0m"}, // Yellows
		{LogCriticalLvls::ERROR,   "\033[1;31mERROR\033[0m"}    // Red
	};
};

// bind with concepts
template <typename T>
concept ValidFormatter = requires(T formatter) {
	{ formatter.GetFormat(LogCriticalLvls::DEBUG) } -> std::convertible_to<std::string_view>;
}&& std::default_initializable<T>;

template <typename E>
concept ValidException = std::derived_from<E, std::exception>;

// logging class - log msg to terminal and file
template <typename FMT = PresetFormatter>
	requires ValidFormatter<FMT>
class Logging {
public:
	using formatter = FMT;

	// Ignore on the DEBUG message
	[[nodiscard]] explicit constexpr Logging() : m_criticality_(LogCriticalLvls::INFO), m_log_file_() {}

	// Log all types of messages
	[[nodiscard]] explicit constexpr Logging(const std::filesystem::path& log_path) :
		m_criticality_(LogCriticalLvls::DEBUG), m_log_file_(std::filesystem::absolute(log_path).c_str()) {
		if (!m_log_file_.is_open()) {
			throw std::filesystem::filesystem_error("Error: Failed to create / open the log file!",
				std::error_code(1, std::system_category()));
		}
	}

	// Main logging method
	template <LogCriticalLvls LVL, typename E = std::exception>
		requires ValidException<E>
	void Log(const std::string_view& msg) noexcept(false) {
		// check universal logging lock
		if constexpr (!m_enabled_) {
			return;
		}
		if (LVL < m_criticality_) {
			return;
		}
		if (m_log_file_.is_open()) {
			m_log_file_ << m_fmt_.GetFormat(LVL) << " - " << msg << '\n';
		}
		std::cout << m_fmt_.GetFormat(LVL) << " - " << msg << '\n';
		// throw exception if the level is ERROR
		if (LVL == LogCriticalLvls::ERROR) {
			m_log_file_.close();
			throw E(msg.data());
		}
	}

private:
	const formatter m_fmt_{};
	std::ofstream m_log_file_;
	LogCriticalLvls m_criticality_;

	// -------------------------------------------------
	// universal logging lock, can only be set manually
	// -------------------------------------------------
	static constexpr bool m_enabled_ = true;
};


// -------------------------------------------------------
//
// Logging System
// 
// -------------------------------------------------------


// concepts to constrain function type
template <typename Func>
concept ValidCallback = requires(Func fnc) {
	{ fnc((long long)0) } -> std::same_as<void>;
};

// scoped timer class
template <typename Func = std::function<void(long long)>>
	requires ValidCallback<Func>
class ScopedTimer {
public:
	using callback = Func;

	// ScopedTimer is a profiling tool, should not be transferable
	ScopedTimer(const ScopedTimer&) = delete;
	ScopedTimer(ScopedTimer&&) noexcept = delete;
	ScopedTimer& operator=(const ScopedTimer&) = delete;
	ScopedTimer& operator=(ScopedTimer&&) noexcept = delete;

	// RAII
	[[nodiscard]] explicit ScopedTimer(callback function) :
		m_t0_(std::chrono::high_resolution_clock::now()), m_function_(function) {}

	~ScopedTimer() noexcept {
		auto t1 = std::chrono::high_resolution_clock::now();
		m_function_(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - m_t0_).count());
	}

private:
	std::chrono::high_resolution_clock::time_point m_t0_;
	callback m_function_;
};
