/**
 * @file core.hpp
 * @brief The core component of prototype processor
 * 
 * @date Sep. 2023
 * @author Yetong (Tony) Li
*/

#pragma once


#include <concepts>
#include <cstdint>
#include <array>
#include <numeric>
#include <algorithm>
#include <unordered_map>
#include "memory.hpp"
#include "register.hpp"
#include "alu.hpp"
#include "decoder.hpp"
#include "syscall.hpp"
#include "tracer.hpp"
#include "freefunc.hpp"


namespace prototype::core
{
    /*
     * Memory Layout
     * 
     *  _________________  <------------ Top
     * |_________________|
     * |				 | \
     * |  Extra Segment  | | 64KB (magic fields :)
     * |_________________| /<----------- ES reg
     * |_________________|
     * |				 | \
     * |			     | |
     * |  Stack Segment  | | 64KB
     * |				 | |
     * |				 | |
     * |_________________| / <----------- SS reg
     * |_________________|
     * |				 | \
     * |   Data Segment  | | 64KB
     * |				 | |
     * |_________________| / <----------- DS reg
     * |_________________|
     * |				 | \
     * |   Code Segment  | | 64KB
     * |_________________| / <----------- CS reg
     * |_________________| <------------- Bottom (vector table or nullptr)
     */

    template <typename SystemBit, typename Data, typename... Datas>
    concept valid_data = requires
    {
        requires
        std::convertible_to<Data, SystemBit> && (std::same_as<Data, Datas> || ...);
    };

    template <typename SourceType, typename DestType>
    concept valid_forwarding_reference_type = requires
    {
        requires
        std::same_as<std::remove_reference_t<SourceType>,
                     std::remove_reference_t<DestType>   >;
        requires
        std::is_constructible_v<DestType, SourceType>;
    };

    namespace mm = ::prototype::memory;
    namespace rf = ::prototype::register_file;
    namespace dc = ::prototype::decode;
    namespace au = ::prototype::alu;
    namespace tr = ::prototype::tracer;
    namespace sy = ::prototype::syscall;

    template <std::unsigned_integral SystemBit,
              std::size_t            MemorySize,
              typename               SyscallTable>
    class Core
    {
    public:
        using system_bit_type         = SystemBit;
        const std::size_t memory_size = MemorySize;

        using memory_type      = mm::Memory<system_bit_type, MemorySize>;
        using register_type    = rf::Registers<system_bit_type>;
        using decoder_type     = dc::Decoder<dc::DefaultEncoding>;
        using alu_type         = au::ALU;

        using instruction_type = typename decoder_type::instruction_type;
        using alu_in_type      = au::ALUInput<system_bit_type>;
        using alu_out_type     = au::ALUOutput<system_bit_type>;

        struct SegmentRange
        {
            using value_type = std::uint32_t;

            value_type m_start;
            value_type m_end;
        };

        using segment_type = std::unordered_map<rf::SEGReg, Core::SegmentRange>;
        using syscall_type = SyscallTable;

        class Loader;

    public:
        template <valid_forwarding_reference_type<segment_type> Segment> [[nodiscard]] constexpr explicit
        Core(Segment&& segment_config)
            : m_tracer{ nullptr }
        {
            if (!initialize(std::forward<Segment>(segment_config)))
            {
                traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>("Failed to initialize segment.");
            }
        }

        template <valid_forwarding_reference_type<segment_type> Segment> [[nodiscard]] constexpr explicit
        Core(Segment&& segment_config, tr::Tracer* p_tracer)
            : m_tracer{ p_tracer }
        {
            if (!initialize(std::forward<Segment>(segment_config)))
            {
                traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>("Failed to initialize segment.");
            }
        }

    public:
        template <typename Instruction, typename... Instructions> 
            requires (valid_data<system_bit_type, Instruction, Instructions...>) constexpr
        auto instructionLoader(Instruction in, Instructions... ins) noexcept
            -> void
        {
            using enum rf::SEGReg;

            recursiveLoaderHelper(m_segment.at(CS).m_start, m_segment.at(CS).m_end, in, ins...);
        }

        template <typename Data, typename... Datas>
            requires (valid_data<system_bit_type, Data, Datas...>) constexpr
        auto dataLoader(Data da, Datas... das) noexcept
            -> void
        {
            using enum rf::SEGReg;

            recursiveLoaderHelper(m_segment.at(DS).m_start, m_segment.at(DS).m_end, da, das...);
        }

        template <typename Container> constexpr
        auto instructionLoader(std::add_rvalue_reference_t<Container> instructions) noexcept
            -> void
        {
            using enum rf::SEGReg;

            iterativeLoaderHelper<std::remove_reference_t<Container>>(
                m_segment.at(CS).m_start,
                m_segment.at(CS).m_end,
                std::forward<Container>(instructions)
            );
        }

        template <typename Container> constexpr
        auto dataLoader(std::add_rvalue_reference_t<Container> data) noexcept
            -> void
        {
            using enum rf::SEGReg;

            iterativeLoaderHelper<std::remove_reference_t<Container>>(
                m_segment.at(DS).m_start,
                m_segment.at(DS).m_end,
                std::forward<Container>(data)
            );
        }

        auto run()
            -> void
        {
            using ::prototype::freefunc::testBitAll;

            for (;;)
            {
                auto binary = fetch();

                if (
                    std::variant<bool, std::domain_error> result = testBitAll(binary);
                    result.index() == 0
                    )
                {
                    if (std::get<0>(result)) {
                        break;
                    }
                }
                else
                {
                    throw std::runtime_error("Error when testing all bits.");
                }

                instruction_type instruction = decoder_type::decode(binary);

                if (checkJump(instruction))
                {
                    generateTrace(binary, instruction);
                    continue;
                }

                auto result = execute(instruction);
                memoryAccess(instruction, result);
                
                generateTrace(binary, instruction);
            }

            return;
        }

    private:
        template <valid_forwarding_reference_type<segment_type> Segment> constexpr
        auto initialize(Segment&& segment_config) noexcept
            -> bool
        {
            using enum rf::SEGReg;
            using enum rf::GPReg;

            constexpr std::array<rf::SEGReg, 4> segment_registers{ CS, DS, SS, ES };
            if (
                !std::ranges::all_of(segment_registers, 
                                     [&segment_config](rf::SEGReg sr) { return segment_config.contains(sr); })
                )
            {
                return false;
            }

            std::vector<Core::SegmentRange> ranges{};
            for (auto&& r : segment_config) {
                ranges.emplace_back(r.second);
            }

            if (
                !std::ranges::all_of(ranges,
                                     [this](const Core::SegmentRange rng) {
                                        return rng.m_end >= rng.m_start && rng.m_start >= 0 && rng.m_end < this->memory_size;
                                     })
                )
            {
                return false;
            }

            std::ranges::sort(
                ranges,
                std::ranges::less{},
                [](const Core::SegmentRange rng) { return rng.m_start; }
            );

            auto checkOverlap = [](const Core::SegmentRange rng1, const Core::SegmentRange rng2)
                -> bool
            {
                return rng1.m_start <= rng2.m_end && rng2.m_start <= rng1.m_end;
            };

            for (auto iter = ranges.cbegin() + 1; iter != ranges.cend(); ++iter)
            {
                if (checkOverlap(*(iter - 1), *iter)) {
                    return false;
                }
            }

            auto acc = [](system_bit_type&& i, const Core::SegmentRange rng)
                -> system_bit_type
            {
                return i + (rng.m_end - rng.m_start) + 1;
            };

            if (
                system_bit_type result = std::accumulate(ranges.begin(), ranges.end(), 0, acc);
                result > memory_size
                )
            {
                return false;
            }

            m_segment = std::forward<Segment>(segment_config);

            m_register.getGeneralPurpose(SP) = m_segment.at(SS).m_end + 1;
            m_register.getGeneralPurpose(PC) = m_segment.at(CS).m_start;

            return true;
        }

        template <tr::Tracer::CriticalLvls Level, tr::valid_exception Exception> constexpr
        auto traceLog(const std::string_view& message) const
            -> void
        {
            if (m_tracer != nullptr)
            {
                m_tracer->log<Level, Exception>(message);
                return;
            }
            if (Level == tr::Tracer::CriticalLvls::ERROR)
            {
                throw Exception(message.data());
            }
        }

        constexpr
        auto generateTrace(system_bit_type binary, instruction_type& instruction) noexcept
            -> void
        {
            if (m_tracer != nullptr)
            {
                m_tracer->generateTrace<system_bit_type, instruction_type, memory_type, register_type, segment_type>(
                    binary, instruction, m_memory, m_register, m_segment
                );
            }
        }

        template <std::unsigned_integral T> [[nodiscard]] inline static constexpr
        auto checkAddressInrange(const T address, const Core::SegmentRange segment) noexcept
            -> bool
        {
            return address >= segment.m_start && address <= segment.m_end;
        }

        constexpr
        auto updatePsr(const typename alu_out_type::updated_flags_type& updated_flags) noexcept
            -> void
        {
            m_register.clearPsrValue();

            for (auto&& flag : updated_flags)
            {
                m_register.setProgramStatus(flag, true);
            }
        }

        template <typename T, typename... Ts> constexpr
        auto recursiveLoaderHelper(std::size_t idx, std::size_t limit, T x, Ts... xs) noexcept
            -> void
        {
            if (idx > limit) {
                return;
            }

            m_memory.write(static_cast<system_bit_type>(x), idx);

            if constexpr (sizeof...(Ts) > 0)
            {
                recursiveLoaderHelper(idx + 1, limit, xs...);
            }
        }

        template <typename Container> constexpr
        auto iterativeLoaderHelper(const std::size_t idx, const std::size_t limit,
                                   std::add_rvalue_reference_t<Container> container) noexcept
            -> void
        {
            for (auto i{ idx }; const system_bit_type data : container)
            {
                if (i > limit) {
                    return;
                }

                m_memory.write(static_cast<system_bit_type>(data), i);
                ++i;
            }

            return;
        }

        template <std::convertible_to<typename alu_in_type::operand_width_type> T> inline static constexpr
        auto makeALUInput(au::ALUOpCode alu_opcode, const T operand1, const T operand2) noexcept
            -> alu_in_type
        {
            return alu_in_type{
                alu_opcode,
                {operand1, operand2}
            };
        }

        [[nodiscard]] constexpr
        auto fetch()
            -> system_bit_type
        {
            using enum rf::GPReg;
            using enum rf::SEGReg;

            if (
                !Core::checkAddressInrange<system_bit_type>(m_register.getGeneralPurpose(PC), 
                                                            m_segment.at(CS))
                )
            {
                traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                    "Error: PC exceeds CS boundary!"
                );
            }

            if (
                auto instruction = m_memory.read(m_register.getGeneralPurpose(PC));
                instruction.index() == 0
                )
            {
                ++m_register.getGeneralPurpose(PC);
                return std::get<0>(instruction);
            }

            traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                "Error: Failed to read instruction from memory!"
            );

            return 0;
        }

        constexpr
        auto generateALUInput(const instruction_type& instruction) const
            -> alu_in_type
        {
            using enum dc::OpType;
            using enum dc::OpCode;

            if (instruction.m_type == Jt)
            {
                traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                    "Jump-type instruction fall through!"
                );
            }

            auto makeALUInputForRI = [this](const instruction_type& instruction, au::ALUOpCode opcode)
                -> alu_in_type
            {
                using enum dc::OpType;

                switch (instruction.m_type)
                {
                    case Rt:
                    {
                        return makeALUInput<system_bit_type>(
                            opcode, 
                            m_register.getGeneralPurpose(instruction.m_Rm), 
                            m_register.getGeneralPurpose(instruction.m_Rn)
                        );
                    }
                    case It:
                    {
                        return makeALUInput<system_bit_type>(
                            opcode, 
                            m_register.getGeneralPurpose(instruction.m_Rm), 
                            instruction.m_imm
                        );
                    }
                    default:
                    {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                            "Unknown instruction type detected."
                        );
                    }    
                }

                return alu_in_type{};
            };

            using au::ALUOpCode;
            using enum rf::GPReg;

            switch (instruction.m_code)
            {
                case ADD : { return makeALUInputForRI(instruction, ALUOpCode::ADD);  }
                case UMUL: { return makeALUInputForRI(instruction, ALUOpCode::UMUL); }
                case UDIV: { return makeALUInputForRI(instruction, ALUOpCode::UDIV); }
                case UMOL: { return makeALUInputForRI(instruction, ALUOpCode::UMOL); }
                case AND : { return makeALUInputForRI(instruction, ALUOpCode::AND);  }
                case ORR : { return makeALUInputForRI(instruction, ALUOpCode::ORR);  }
                case XOR : { return makeALUInputForRI(instruction, ALUOpCode::XOR);  }
                case SHL : { return makeALUInputForRI(instruction, ALUOpCode::SHL);  }
                case SHR : { return makeALUInputForRI(instruction, ALUOpCode::SHR);  }
                case RTL : { return makeALUInputForRI(instruction, ALUOpCode::RTL);  }
                case RTR : { return makeALUInputForRI(instruction, ALUOpCode::RTR);  } 
                case NOT : {
                    return makeALUInput<system_bit_type>(
                        ALUOpCode::COMP, m_register.getGeneralPurpose(instruction.m_Rm), 0x0
                    );
                }
                case LDR : {
                    return makeALUInput<system_bit_type>(
                        ALUOpCode::PASS, m_register.getGeneralPurpose(instruction.m_Rm), 0x0
                    );
                }
                case STR : {
                    return makeALUInput<system_bit_type>(
                        ALUOpCode::PASS, m_register.getGeneralPurpose(instruction.m_Rm), 0x0
                    );
                }
                case PUSH: {
                    return makeALUInput<system_bit_type>(
                        ALUOpCode::ADD, m_register.getGeneralPurpose(SP), -1
                    );
                }
                case POP : {
                    return makeALUInput<system_bit_type>(
                        ALUOpCode::ADD, m_register.getGeneralPurpose(SP), 1
                    );
                }
                default:   {
                    traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                        "Unknown OpCode detected."
                    );
                }
            }

            return alu_in_type{};
        }

        [[nodiscard]] constexpr
        auto execute(const instruction_type& instruction)
            -> typename alu_out_type::operand_width_type
        {
            const alu_out_type result = alu_type::execute<system_bit_type>(generateALUInput(instruction));
            updatePsr(result.m_flags);
            return result.m_result;
        }

        constexpr
        auto checkJump(const instruction_type& instruction)
            -> bool
        {
            using enum dc::OpType;
            using enum dc::OpCode;
            using enum rf::GPReg;
            using enum rf::PSRReg;

            if (instruction.m_type != Jt) {
                return false;
            }

            auto performJump = [this](bool condition, typename register_type::GP_width_type dst) noexcept
                -> void
            {
                if (condition) {
                    m_register.getGeneralPurpose(PC) = dst;
                }
            };

            switch (instruction.m_code)
            {
                case JMP:
                {
                    performJump(true, instruction.m_imm);
                    break;
                }
                case JZ: 
                {
                    performJump(m_register.getProgramStatus(Z), instruction.m_imm);
                    break; 
                }
                case JN: 
                {
                    performJump(m_register.getProgramStatus(N), instruction.m_imm);
                    break;
                }
                case JC: 
                {
                    performJump(m_register.getProgramStatus(C), instruction.m_imm);
                    break;
                }
                case JV: 
                {
                    performJump(m_register.getProgramStatus(V), instruction.m_imm);
                    break;
                }
                case JZN:
                {
                    performJump(
                        m_register.getProgramStatus(Z) || m_register.getProgramStatus(N),
                        instruction.m_imm
                    );
                    break;
                }
                case SYSCALL:
                {
                    try
                    {
                        syscall_type::template m_syscall_table<Core::memory_type, Core::register_type>.at(
                            static_cast<std::uint32_t>(instruction.m_imm)
                        ) (this->m_memory, this->m_register);
                    }
                    catch (const std::out_of_range&)
                    {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::out_of_range>(
                            "Unknown syscall number."
                        );
                    }

                    break;
                }
                default:
                {
                    traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                        "Unknown OpCode detected."
                    );

                    break;
                }
            }

            return true;
        }
        
        constexpr
        auto memoryAccess(const instruction_type& instruction, system_bit_type addr_val)
            -> void
        {
            using enum dc::OpCode;
            using enum rf::SEGReg;
            using enum rf::GPReg;

            switch (instruction.m_code)
            {
                case LDR:
                {
                    if (
                        auto result = m_memory.read(addr_val);
                        result.index() == 0
                        )
                    {
                        m_register.getGeneralPurpose(instruction.m_Rd) = std::get<0>(result);
                    }
                    else {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                            "Invalid memory access."
                        );
                    }

                    break;
                }
                case STR:
                {
                    if (
                        auto result = m_memory.write(m_register.getGeneralPurpose(instruction.m_Rd), addr_val);
                        result.index() != 0
                        )
                    {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                            "Invalid memory access."
                        );
                    }

                    break;
                }
                case PUSH:
                {
                    if (!Core::checkAddressInrange(addr_val, m_segment.at(SS)))
                    {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                            "Stack-overflow :)"
                        );
                    }

                    m_memory.write(m_register.getGeneralPurpose(instruction.m_Rd), addr_val);
                    m_register.getGeneralPurpose(SP) = addr_val;

                    break;
                }
                case POP:
                {
                    if (!Core::checkAddressInrange(addr_val - 1, m_segment.at(SS))) {
                        return;
                    }

                    if (
                        auto result = m_memory.read(m_register.getGeneralPurpose(SP));
                        result.index() == 0
                        )
                    {
                        m_register.getGeneralPurpose(instruction.m_Rd) = std::get<0>(result);
                        m_register.getGeneralPurpose(SP) = addr_val;
                    }
                    else {
                        traceLog<tr::Tracer::CriticalLvls::ERROR, std::runtime_error>(
                            "Invalid memory access."
                        );
                    }

                    break;
                }
                default: 
                {
                    m_register.getGeneralPurpose(instruction.m_Rd) = addr_val;

                    break;
                }
            }
        }
    
    private:
        memory_type   m_memory{};
        register_type m_register{};
        segment_type  m_segment{};

        tr::Tracer*   m_tracer;
    };
}
