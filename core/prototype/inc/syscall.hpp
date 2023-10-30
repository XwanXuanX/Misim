/**
 * @file syscall.hpp
 * @brief Define the syscall table
 * 
 * @date Sep, 2023
 * @author Yetong (Tony) Li
*/

#pragma once


#include <string>
#include <iostream>
#include <algorithm>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include "register.hpp"


namespace prototype::syscall
{
    template <typename Object>
    using lref_type = std::add_lvalue_reference_t<Object>;

    struct SyscallTable
    {
    private:
        template <typename Memory, typename Register> static
        auto welcome(lref_type<Memory>, lref_type<Register>) noexcept
            -> void
        {
            printf(
                "Welcome stranger!\n\n"
                "This is the CPU speaking - I'm glad that you found this eastern egg left by my creator.\n"
                "If you see this message, it means that you must be browsing through the code or experimenting with me.\n"
                "I hope you have the same enthusiasm with C++ as my creator does - because enthusiasm is the "
                "most important thing in the world.\n\n"
                "Well, wish you a good day. Bye, adios!\n"
            );
        }

        /*
         * Doc-string:
         * The console_out() function allow user to print text on terminal.
         * @inputs:
         *	- R0: the starting addr of a contigious memory (string) that will be displayed;
         *	- R1: the length of contigious memory;
         * @outputs:
         *	- None
         */
        template <typename Memory, typename Register> static
        auto consoleOut(lref_type<Memory> memory, lref_type<Register> registers)
            -> void
        {
            using enum ::prototype::register_file::GPReg;

            std::string temp{};
            temp.reserve(registers.getGeneralPurpose(R1));
        
            for (
                 auto i = registers.getGeneralPurpose(R0);
                 i < (registers.getGeneralPurpose(R0) + registers.getGeneralPurpose(R1));
                 ++i
                )
            {
                if (auto result = memory.read(i); result.index() == 0)
                {
                    temp.push_back(std::get<0>(result));
                }
                else
                {
                    throw std::out_of_range("Memory access out of range.");
                }
            }

            std::cout << temp;
        }
            
        /*
         * Doc-string:
         * The console_in() function allow user to input string with keyboard.
         * @inputs:
         *	- R0: the starting addr of a contigious memory that will be filled;
         *	- R1: the length of contigious memory;
         * @outputs:
         *	- User input string will be placed in the contigious memory. All spaces will be rewritten.
         *	  Thus please make sure to save necessary contents on the stack before doing the operation.
         */
        template <typename Memory, typename Register> static
        auto consoleIn(lref_type<Memory> memory, lref_type<Register> registers)
            -> void
        {
            using enum ::prototype::register_file::GPReg;

            std::string temp;
            std::getline(std::cin, temp);

            if (temp.length() > registers.getGeneralPurpose(R1))
            {
                throw std::length_error(
                    "User-input string exceeds maximum space length."
                );
            }

            std::ranges::for_each(
                temp,
                [i = registers.getGeneralPurpose(R0), &memory](auto character) mutable noexcept {
                    memory.write(character, i);
                    ++i;
                }
            );
        }

    public:
        template <typename Memory, typename Register>
        inline static const std::unordered_map<std::uint32_t,
                                               std::function<void(lref_type<Memory>, lref_type<Register>)>>
        m_syscall_table =
        {
            {0, SyscallTable::template welcome   <Memory, Register>},
            {1, SyscallTable::template consoleOut<Memory, Register>},
            {2, SyscallTable::template consoleIn <Memory, Register>}
        };
    };
}
