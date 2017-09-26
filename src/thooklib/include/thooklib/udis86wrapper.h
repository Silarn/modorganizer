/*
Userspace Virtual Filesystem

Copyright (C) 2015 Sebastian Herbord. All rights reserved.

This file is part of usvfs.

usvfs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

usvfs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with usvfs. If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once
#include <udis86.h>
#undef inline // libudis86/types.h defines inline to __inline which is no longer legal since vs2012

namespace HookLib {

class UDis86Wrapper {
  public:
    UDis86Wrapper();
    ~UDis86Wrapper() = default;

    // Sets up input for the buffer `buffer` of size `size`
    // Calls ud_set_input_buffer and ud_set_pc.
    // This should be called before anything else.
    void setInputBuffer(const uint8_t* buffer, size_t size);

    // Disassemble.
    // Returns number of bytes disassembled.
    // ud_disassemble
    unsigned int disassemble() { return ud_disassemble(&m_Obj); }

    // Returns the instruction mnemonic in the form of an enumerated constant.
    // As a convention all mnemonic constants are composed by prefixing standard instruction mnemonics with UD_I.
    // For example, the enumerations for mov, xor and jmp are UD_Imov, UD_Ixor, and UD_Ijmp, respectively.
    // ud_insn_mnemonic
    ud_mnemonic_code mnemonic() { return ud_insn_mnemonic(&m_Obj); }

    // Returns a pointer to the nth (starting with 0) operand of the instruction.
    // If the instruction does not have such an operand, the function returns NULL.
    // ud_insn_opr
    const ud_operand_t* operator[](unsigned int i) { return ud_insn_opr(&m_Obj, i); }

    // Returns the number of bytes disassembled
    // ud_insn_len
    unsigned int len() { return ud_insn_len(&m_Obj); }

    ud_t& obj();

    operator ud_t*() { return &m_Obj; }

    bool isRelativeJump();

    intptr_t jumpOffset();

    ///
    /// determines the absolute jump target at the current instruction, taking into account
    /// relative instructions of all sizes and RIP-relative addressing.
    /// \return absolute address of the jump at the current disassembler instruction
    /// \note this works correctly ONLY if the input buffer has been set with setInputBuffer or
    ///       if ud_set_pc has been called
    ///
    uint64_t jumpTarget();

  private:
    ud_t m_Obj;
    const uint8_t* m_Buffer = nullptr;
};

} // namespace HookLib
