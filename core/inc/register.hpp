/**
 * @file register.hpp
 * @brief Register file for the prototype CPU
 * 
 * @author Yetong (Tony) Li
 * @date Sep, 2023
*/

#pragma once


#include <array>
#include <cstdint>
#include <cassert>
#include "freefunc.hpp"


namespace prototype::register_file
{
    enum GPReg : std::uint8_t
    {
        R0 = 0, // --+
        R1,     //   |
        R2,     //   |
        R3,     //   |
        R4,     //   |
        R5,     //   |  General
        R6,     //   |  Purpose
        R7,     //   |  Registers
        R8,     //   |
        R9,     //   |
        R10,    //   |
        R11,    //   |
        R12,    // --+

        SP,     //      Stack Pointer
        LR,     //      Link Register
        PC      //      Program Counter
    };

    enum SEGReg : std::uint8_t
    {
        CS,		//      Code Segment
        DS,		//      Data Segment
        SS,		//      Stack Segment
        ES		//      Extra Segment
    };

    enum PSRReg : std::uint8_t
    {
        N,	    //      Negative or less than flag
        Z,	    //      Zero flag
        C,	    //      Carry/borrow flag
        V	    //      Overflow flag
    };

    template <std::unsigned_integral uint>
    class Registers
    {
    public:
        using PSR_width_type = std::uint8_t;
        using GP_width_type  = uint;
        using PSR_model_type = PSR_width_type;
        using GP_model_type  = std::array<GP_width_type, 16>;

    public:
        [[nodiscard]] inline constexpr
        auto getGeneralPurpose(const std::uint8_t register_name) noexcept
            -> GP_width_type&
        {
            return m_gp[register_name];
        }

        [[nodiscard]] inline constexpr
        auto getGeneralPurpose(const std::uint8_t register_name) const noexcept
            -> GP_width_type
        {
            return m_gp.at(register_name);
        }

        [[nodiscard]] inline constexpr
        auto getProgramStatus(const std::uint8_t flag_name) const noexcept
            -> bool
        {
            namespace ff = ::prototype::freefunc;
            using return_t = std::variant<bool, std::domain_error>;

            const return_t result = ff::testBit<PSR_width_type>(m_psr, flag_name);
            assert(result.index() == 0);

            return std::get<0>(result);
        }

        constexpr
        auto setProgramStatus(const std::uint8_t flag_name, const bool value) noexcept
            -> std::variant<bool, std::runtime_error>
        {
            namespace ff = ::prototype::freefunc;
            using return_t = std::variant<bool, std::domain_error>;

            const return_t result = (value) ? ff::setBit  <PSR_width_type>(m_psr, flag_name) :
                                              ff::resetBit<PSR_width_type>(m_psr, flag_name) ;

            if (result.index() != 0)
            {
                return std::runtime_error("setBit / resetBit failed");
            }

            return true;
        }

        [[nodiscard]] inline constexpr
        auto getPsrValue() const noexcept
            -> PSR_width_type
        {
            return m_psr;
        }

        inline constexpr
        auto clearPsrValue() noexcept
            -> void
        {
            m_psr = 0x0;
        }

    private:
        GP_model_type  m_gp{};
        PSR_model_type m_psr{};
    };
}
