/**
 * @file loader.hpp
 * @brief Prototype loader for prototype CPU.
 * 
 * @date Sep. 2023
 * @author Yetong (Tony) Li
*/

#pragma once


#include <coroutine>
#include <vector>
#include "core.hpp"


namespace utility::generator
{
    template <typename V>
    concept valid_generator_value_type = requires
    {
        requires
        (std::movable<V> || std::copyable<V>);
        requires
        std::default_initializable<V>;
    };

    template <valid_generator_value_type V>
    class Generator
    {
    public:
        struct promise_type;

        using value_type  = std::remove_cvref_t<V>;
        using handle_type = std::coroutine_handle<promise_type>;

    public:
        struct promise_type
        {
        public:
            auto get_return_object() noexcept
                -> Generator
            {
                return Generator(handle_type::from_promise(*this));
            }

            auto initial_suspend() noexcept
                -> std::suspend_always
            {
                return {};
            }

            auto final_suspend() noexcept
                -> std::suspend_always
            {
                return {};
            }

            auto return_void() noexcept
                -> void
            {
                return;
            }

            auto unhandled_exception() noexcept
                -> void
            {
                return;
            }

            template <std::convertible_to<value_type> From>
            auto yield_value(From&& from)
                -> std::suspend_always
            {
                m_promise_value = std::forward<From>(from);
                return {};
            }

            auto get_promise_value() const noexcept
                -> value_type
            {
                return m_promise_value;
            }

        private:
            value_type m_promise_value;
        };

    public:
        [[nodiscard]] explicit
        Generator(handle_type handle)
            : m_handle{ handle }
        {
        }

        [[nodiscard]] explicit
        Generator() = default;

        ~Generator()
        {
            if (m_handle)
            {
                m_handle.destroy();
            }
        }

    public:
        auto finished() const noexcept
            -> bool
        {
            return m_handle.done();
        }

        auto value() const noexcept
            -> value_type
        {
            return m_handle.promise().get_promise_value();
        }

        auto resume() noexcept
            -> void
        {
            if (!finished())
            {
                m_handle.resume();
            }
        }

    private:
        struct Sentinal {};

        struct Iterator
        {
        public:
            [[nodiscard]] explicit
            Iterator(handle_type handle)
                : m_iter_handle{ handle }
            {
            }

        public:
            auto operator==(Sentinal) const noexcept
                -> bool
            {
                return m_iter_handle.done();
            }

            auto operator++() noexcept
                -> Iterator&
            {
                m_iter_handle.resume();
                
                return *this;
            }

            auto operator*() const noexcept
                -> const value_type
            {
                return m_iter_handle.promise().get_promise_value();
            }

        private:
            handle_type m_iter_handle{};
        };

    public:
        auto begin() const noexcept
            -> Iterator
        {
            return Iterator(m_handle);
        }

        auto end() const noexcept
            -> Sentinal
        {
            return {};
        }

    private:
        handle_type m_handle{};
    };
}


namespace prototype::core
{
    template <std::unsigned_integral SystemBit,
              std::size_t            MemorySize,
              typename               SyscallTable>
    class Core<SystemBit, MemorySize, SyscallTable>::Loader
    {
    public:
        using data_type        = std::vector<system_bit_type>;
        using instruction_type = std::vector<system_bit_type>;

    private:
        [[nodiscard]] static
        auto binaryFileGenerator(std::ifstream file) noexcept
            -> utility::generator::Generator<std::string>
        {
            std::string line{};

            while (std::getline(file, line))
            {
                if (
                    line.length() == 0
                    || line.starts_with(';')
                    )
                {
                    continue;
                }

                co_yield line;
            }

            co_return;
        }

    public:
        Loader() noexcept = delete;

        [[nodiscard]] explicit
        Loader(const std::filesystem::path& file_path)
        {
            if (
                !std::filesystem::exists(file_path)
                || !file_path.has_extension()
                || file_path.extension() != ".bin"
                )
            {
                throw std::filesystem::filesystem_error(
                    "Invalid binary file path.",
                    std::error_code(1, std::system_category())
                );
            }

            m_file_stream = std::ifstream(file_path.c_str());
        }

        [[nodiscard]] explicit
        Loader(std::ifstream file_stream)
            : m_file_stream{ std::move(file_stream) }
        {
        }

    private:
        template <typename Callable>
        struct State
        {
            std::string m_name{};
            int         m_id  {};

            std::function<Callable> method{};
        };

        static constexpr
        auto isNumeric(const std::string& str) noexcept
            -> bool
        {
            return std::ranges::all_of(
                str,
                [](const char ch) noexcept -> bool {
                    return std::isdigit(ch) || ch == ' ';
                }
            );
        }

        static
        auto parseHeading(const ::prototype::register_file::SEGReg symbol,
                          const std::string& line,
                          segment_type&      segm                         )
            -> void
        {
            if (!Loader::isNumeric(line)) {
                throw std::invalid_argument("Line not numeric.");
            }

            std::istringstream iss{ line };
            system_bit_type start_addr{}, end_addr{};

            iss >> start_addr >> end_addr;

            if (start_addr > end_addr)
            {
                throw std::logic_error(
                    "Starting address higher than ending address."
                );
            }

            segm.insert_or_assign(
                symbol,
                SegmentRange{ start_addr, end_addr }
            );
        }

        static
        auto parseDataSize(const std::string& line, data_type&, instruction_type&,
                           segment_type&      segm                                )
            -> void
        {
            using enum ::prototype::register_file::SEGReg;
            Loader::parseHeading(DS, line, segm);
        }

        static
        auto parseExtraSize(const std::string& line, data_type&, instruction_type&,
                            segment_type&      segm                                )
            -> void
        {
            using enum ::prototype::register_file::SEGReg;
            Loader::parseHeading(ES, line, segm);
        }

        static
        auto parseTextSize(const std::string& line, data_type&, instruction_type&,
                           segment_type&      segm                                )
            -> void
        {
            using enum ::prototype::register_file::SEGReg;
            Loader::parseHeading(CS, line, segm);
        }

        template <typename ContainerType> static
        auto parseBody(const std::string& line,
                       std::add_lvalue_reference_t<ContainerType> container)
            -> void
        {
            if (!Loader::isNumeric(line)) {
                throw std::invalid_argument("Line not numeric.");
            }

            std::istringstream iss{ line };
            system_bit_type data{};

            iss >> data;

            container.push_back(data);
        }

        static
        auto parseDataData(const std::string& line,
                           data_type&         data, instruction_type&, segment_type&)
            -> void
        {
            Loader::parseBody<data_type>(line, data);
        }

        static
        auto parseTextData(const std::string& line, data_type&,
                           instruction_type&  inst, segment_type&)
            -> void
        {
            Loader::parseBody<instruction_type>(line, inst);
        }

        enum class States : std::uint8_t
        {
            ds, es, ts, dd, td
        };

        struct StateSelector
        {
        public:
            using state_type = Loader::State<void(const std::string&,
                                                  data_type&, instruction_type&, segment_type&)>;

        public:
            auto update(const States update_to) noexcept
                -> void
            {
                auto pickUpdateState = [](const States update_to) noexcept
                    -> state_type
                {
                    switch (update_to)
                    {
                        case States::ds: { return { "ds", 0, Loader::parseDataSize  }; }
                        case States::es: { return { "es", 1, Loader::parseExtraSize }; }
                        case States::ts: { return { "ts", 2, Loader::parseTextSize  }; }
                        case States::dd: { return { "dd", 3, Loader::parseDataData  }; }
                        case States::td: { return { "td", 4, Loader::parseTextData  }; }
                    }

                    return {};
                };

                m_state = pickUpdateState(update_to);
            }

            auto run(const std::string& line, data_type& data, instruction_type& inst, segment_type& segm) const
                -> void
            {
                if (!m_state.m_name.length() && m_state.m_id == 0)
                {
                    std::cout << "Error: m_state used without being initialized.\n";

                    throw std::runtime_error(
                        "Attempt to run with empty state."
                    );
                }

                m_state.method(line, data, inst, segm);
            }

        private:
            state_type m_state{};
        };

    public:
        auto parseBinaryFile()
            -> void
        {
            using namespace utility::generator;

            StateSelector state_selector{};
            std::unordered_map<std::string, States> lut
            {
                { "ds", States::ds },
                { "es", States::es },
                { "ts", States::ts },
                { "dd", States::dd },
                { "td", States::td }
            };

            for (const std::string& line : Loader::binaryFileGenerator(std::move(m_file_stream)))
            {
                if (!line.length()) {
                    continue;
                }

                if (lut.contains(line)) {
                    state_selector.update(lut.at(line));
                    continue;
                }

                state_selector.run(line, m_data, m_instructions, m_segments);
            }

            appendStackSize();
        }

    private:
        auto appendStackSize() noexcept
            -> void
        {
            using element_type = std::pair<::prototype::register_file::SEGReg, SegmentRange>;
            using enum ::prototype::register_file::SEGReg;

            auto maximum = std::ranges::max_element(
                m_segments,
                [](const element_type& lhs, const element_type& rhs) noexcept -> bool {
                    return lhs.second.m_end < rhs.second.m_end;
                }
            );

            m_segments.insert_or_assign(
                SS,
                SegmentRange {
                    .m_start = (maximum->second.m_end) + 1,
                    .m_end   =  MemorySize - 1
                }
            );

            return;
        }

    public:
        auto getData() const noexcept
            -> data_type
        {
            return m_data;
        }

        auto getInstruction() const noexcept
            -> instruction_type
        {
            return m_instructions;
        }

        auto getSegments() const noexcept
            -> segment_type
        {
            return m_segments;
        }

    private:
        std::ifstream    m_file_stream {};

        data_type        m_data        {};
        instruction_type m_instructions{};
        segment_type     m_segments    {};
    };
}
