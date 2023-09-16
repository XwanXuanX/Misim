/**
 * @file tracer.hpp
 * @brief Tracer to log detailed CPU behaviour
 * 
 * @date Sep. 2023
 * @author Yetong (Tony) Li
*/

#pragma once


#include <map>
#include <cassert>
#include <concepts>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <string>
#include "register.hpp"
#include "decoder.hpp"


namespace prototype::tracer
{
    template <typename Exception>
    concept valid_exception = requires
    {
        requires
        std::derived_from<Exception, std::exception>;
    };

    template <typename Object>
    using const_lref_type = const std::add_lvalue_reference_t<Object>;

    class Tracer
    {
    public:
        enum struct CriticalLvls : std::int8_t
        {
            INFO,
            WARNING,
            ERROR
        };

    private:
        struct Labels
        {
            using enum ::prototype::register_file::GPReg;
            using enum ::prototype::register_file::SEGReg;
            using enum ::prototype::register_file::PSRReg;
            using enum ::prototype::decode::OpCode;
            using enum ::prototype::decode::OpType;

            using gp_translation_type  = const std::map          <::prototype::register_file::GPReg , std::string_view>;
            using psr_translation_type = const std::unordered_map<::prototype::register_file::PSRReg, char            >;
            using seg_translation_type = const std::unordered_map<::prototype::register_file::SEGReg, std::string_view>;
            using opc_translation_type = const std::unordered_map<::prototype::decode::OpCode       , std::string_view>;
            using opt_translation_type = const std::unordered_map<::prototype::decode::OpType       , std::string_view>;

            static inline gp_translation_type gp_translation =
            {
                { R0, "R0"   },   { R1, "R1" },   { R2, "R2"   },   { R3, "R3"   },
                { R4, "R4"   },   { R5, "R5" },   { R6, "R6"   },   { R7, "R7"   },
                { R8, "R8"   },   { R9, "R9" },   { R10, "R10" },   { R11, "R11" },
                { R12, "R12" },   { SP, "SP" },   { LR, "LR"   },   { PC, "PC"   }
            };

            static inline psr_translation_type psr_translation =
            {
                { N, 'N' },
                { Z, 'Z' },
                { C, 'C' },
                { V, 'V' }
            };

            static inline seg_translation_type seg_translation =
            {
                { CS, "Code Segment"  },
                { DS, "Data Segment"  },
                { SS, "Stack Segment" },
                { ES, "Extra Segment" }
            };

            static inline opt_translation_type opt_translation =
            {
                { Rt, "R type" },
                { It, "I type" },
                { Ut, "U type" },
                { St, "S type" },
                { Jt, "J type" }
            };

            static inline opc_translation_type opc_translation =
            {
                { ADD, "ADD" }, { UMUL, "UMUL" }, { UDIV, "UDIV" }, { UMOL, "UMOL" },

                { AND, "AND" }, { ORR, "ORR"   }, { XOR, "XOR"   }, { SHL, "SHL"   },
                { SHR, "SHR" }, { RTL, "RTL"   }, { RTR, "RTR"   }, { NOT, "NOT"   },

                { LDR, "LDR" }, { STR, "STR"   }, { PUSH, "PUSH" }, { POP, "POP"   },

                { JMP, "JMP" }, { JZ, "JZ" }, { JN, "JN" }, { JC, "JC" }, { JV, "JV" }, { JZN, "JZN" },

                { SYSCALL, "SYSCALL" }
            };
        };

    public:
        [[nodiscard]] explicit
        Tracer(const std::filesystem::path& log_path)
            : m_log_file{ std::filesystem::absolute(log_path).c_str() }
            , m_instruction_count{ 0 }
        {
            if (!m_log_file.is_open())
            {
                throw std::filesystem::filesystem_error(
                    "Failed to create the log file.",
                    std::error_code(1, std::system_category())
                );
            }
        }

    public:
        template <Tracer::CriticalLvls Level, valid_exception Exception>
        auto log(const std::string_view& message)
            -> void
        {
            auto getPrefix = [](Tracer::CriticalLvls level)
                -> std::string_view
            {
                using enum ::prototype::tracer::Tracer::CriticalLvls;

                switch (level)
                {
                    case INFO:      { return "INFO: "   ; }
                    case WARNING:   { return "WARNING: "; }
                    case ERROR:     { return "ERROR: "  ; }
                    default:        { throw std::runtime_error("Unknown critical level."); }
                }
            };

            m_log_file << getPrefix(Level) << message << '\n';

            if (Level == Tracer::CriticalLvls::ERROR)
            {
                m_log_file.close();
                throw Exception(message.data());
            }
        }

        template <std::unsigned_integral T,
                  typename Instruction, typename Memory,
                  typename Register   , typename Segment>
        auto generateTrace(const T binary,
                           const_lref_type<Instruction> instruction, const_lref_type<Memory>  memory,
                           const_lref_type<Register>    registers  , const_lref_type<Segment> segments) noexcept
            -> void
        {
            m_log_file << getHeading(binary)
                       << getInstruction<Instruction>(instruction)
                       << getRegister<Register>(registers)
                       << getMemory<Memory, Segment>(memory, segments)
                       << '\n';

            ++m_instruction_count;
        }

    private:
        template <typename Register>
        auto getRegister(const_lref_type<Register> registers) noexcept
            -> std::string
        {
            std::stringstream ss_label{};
            std::stringstream ss_value{};

            std::ranges::for_each(
                Labels::gp_translation,
                [&](const auto p) {
                    ss_label << p.second << ',';
                    ss_value << registers.getGeneralPurpose(p.first) << ',';
                }
            );

            formatter(ss_label, ss_value);

            std::ranges::for_each(
                Labels::psr_translation,
                [&](const auto p) {
                    ss_label << p.second << ',';
                    ss_value << registers.getProgramStatus(p.first) << ',';
                }
            );

            formatter(ss_label, ss_value);

            return ss_label.str();
        }

        template <typename Memory, typename Segment>
        auto getMemory(const_lref_type<Memory> memory, const_lref_type<Segment> segments)
            -> std::string
        {
            std::stringstream ss_segments{};
            std::stringstream ss_values{};

            for (const auto& p : segments)
            {
                try
                {
                    ss_segments << Labels::seg_translation.at(
                        static_cast<::prototype::register_file::SEGReg>(p.first)
                    );
                }
                catch (const std::out_of_range&)
                {
                    log<CriticalLvls::ERROR, std::out_of_range>(
                        "No corresponding segment translation."
                    );
                }

                for (auto i = p.second.m_start; i <= p.second.m_end; ++i)
                {
                    auto result = memory.read(i);
                    
                    if (result.index() != 0) {
                        log<CriticalLvls::ERROR, std::out_of_range>(
                            "Memory access out of range."
                        );
                    }

                    assert(result.index() == 0);
                    ss_values << static_cast<std::uint32_t>(std::get<0>(result)) << ',';
                }

                formatter(ss_segments, ss_values);
            }

            return ss_segments.str();
        }

        template <typename Instruction>
        auto getInstruction(const_lref_type<Instruction> instruction)
            -> std::string
        {
            std::stringstream ss_label{};
            std::stringstream ss_value{};

            constexpr std::array<std::string_view, 6> labels
            {
                "OpType", "OpCode", "Rd", "Rm", "Rn", "Imm"
            };

            std::ranges::for_each(
                labels,
                [&ss_label](const auto v) { ss_label << v << ','; }
            );
            ss_label << '\n';

            const std::array<typename Instruction::register_type, 3> registers
            {
                instruction.m_Rd, instruction.m_Rm, instruction.m_Rn
            };

            try
            {
                ss_value << Labels::opt_translation.at(
                    static_cast<::prototype::decode::OpType>(instruction.m_type)) << ',';

                ss_value << Labels::opc_translation.at(
                    static_cast<::prototype::decode::OpCode>(instruction.m_code)) << ',';

                std::ranges::for_each(
                    registers,
                    [&ss_value](const auto v) {
                        ss_value << Labels::gp_translation.at(
                            static_cast<::prototype::register_file::GPReg>(v)) << ',';
                    }
                );
            }
            catch (const std::out_of_range&)
            {
                log<CriticalLvls::ERROR, std::out_of_range>(
                    "No corresponding instruction translation."
                );
            }

            ss_value << static_cast<std::uint32_t>(instruction.m_imm) << '\n';
            ss_label << ss_value.str();
            
            return ss_label.str();
        }

        template <std::unsigned_integral T>
        auto getHeading(const T binary) const noexcept
            -> std::string
        {
            std::stringstream ss;

            ss << "Instruction #" << m_instruction_count
               << ", 0x"
               << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex
               << binary
               << '\n';
            
            return ss.str();
        }

        static
        auto formatter(std::stringstream& label, std::stringstream& value) noexcept
            -> void
        {
            label << '\n';
            value << '\n';
            label << value.str();
            value.str(std::string{});
        }

    private:
        std::ofstream m_log_file;
        std::uint32_t m_instruction_count;
    };
}
