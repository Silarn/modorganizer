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
#include "asmjit_sane.h"
#include "usvfs_shared/logging.h"
#include "usvfs_shared/windows_sane.h"
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

#if defined(_M_X64) || defined(__amd64__)
#pragma message("64bit build")
#define IS_X64 1
#elif _M_IX86
#pragma message("32bit build")
#define IS_X64 0
#else
#error "Unsupported Architecture"
#endif

namespace HookLib {

namespace detail {
template <typename Bar>
class thread_specific_ptr {
  private:
    static thread_local std::unordered_map<thread_specific_ptr*, Bar*> tls;
    Bar* get_() const {
        auto I = tls.find(const_cast<thread_specific_ptr*>(this));
        if (I != tls.end()) {
            return I->second;
        }
        auto II = tls.emplace(const_cast<thread_specific_ptr*>(this),
                              nullptr); // Could use std::piecewise_construct here...
        return II.first->second;
    }
    void set_(Bar* new_value) {
        get_();
        tls[this] = new_value;
    }

  public:
    explicit thread_specific_ptr() = default;
    ~thread_specific_ptr() { reset(); };
    Bar* get() const { return get_(); }
    Bar* operator->() const { return get(); }
    Bar& operator*() const { return *get(); }
    void reset(Bar* new_value = nullptr) {
        auto Old = get();
        if (Old != new_value && Old) {
            delete Old;
        }
        set_(new_value);
    }
    Bar* release() {
        // TODO: Set current thread pointer to null.
        Bar* old = get();
        set_(new_value);
        return old;
    }
};
} // namespace detail

///
/// trampolines are runtime-generated mini-functions that are used to call the
/// original code of a function
///
class TrampolinePool {
  public:
    static TrampolinePool& instance();

    void setBlock(bool block);

    ///
    /// store a stub without moving code from the original function. This is used in cases
    /// where the hook can be placed without overwriting logic (i.e. hot-patchable functions and
    /// when chaining hooks)
    /// \param reroute the stub function to call before the regular function (on x86 this needs to be cdecl calling
    /// convention!) \param original the original function \param returnAddress address under which the original
    /// functionality can be reached.
    ///                      for the first hook this should be (original + 2), otherwise the address of the next hook in
    ///                      the chain
    /// \return address of the created trampoline function
    ///
    LPVOID storeStub(LPVOID reroute, LPVOID original, LPVOID returnAddress);

    ///
    /// store a stub, moving part of the original function to the trampoline
    /// \param reroute the stub function to call before the regular function (on x86 this needs to be cdecl calling
    /// convention!) \param original the original function \param preambleSize number of bytes from the original
    /// function to backup. Needs to correspond to complete instructions \param rerouteOffset offset in bytes from the
    /// created trampoline to the preamble that leads back to the original code \return address of the created
    /// trampoline function
    ///
    LPVOID storeStub(LPVOID reroute, LPVOID original, size_t preambleSize, size_t* rerouteOffset);

    ///
    /// store a trampoline for hot-patchable functions, where the original function
    /// is unharmed.
    /// \param reroute the reroute function
    /// \param original original function
    /// \param returnAddress address under which the original functionality can be reached.
    /// \return address of the trampoline function
    ///
    LPVOID storeTrampoline(LPVOID reroute, LPVOID original, LPVOID returnAddress);

    ///
    /// store a trampoline, copying a part of the original function to the trampoline. This
    /// is used for the case where the hooking mechanism needs to overwrite part of the function
    /// \param reroute the reroute function
    /// \param original original function
    /// \param preambleSize number of bytes from the original function to backup. Needs to correspond to complete
    /// instructions \param rerouteOffset offset in bytes from the created trampoline to the preamble that leads us back
    /// to the original code \return address of the trampoline function
    ///
    LPVOID storeTrampoline(LPVOID reroute, LPVOID original, size_t preambleSize, size_t* rerouteOffset);

    ///
    /// \param addressNear used to find a trampoline buffer near the jump instruction
    /// \return retrieve address of current trampoline buffer
    ///
    LPVOID currentBufferAddress(LPVOID addressNear);

    ///
    /// \brief forces the barrier(s) for the current thread to be released
    ///
    void forceUnlockBarrier();

  private:
    struct BufferList {
        size_t offset;
        std::vector<LPVOID> buffers;
    };

    typedef std::map<LPVOID, BufferList> BufferMap;
    static const intptr_t ADDRESS_MASK =
        0xFFFFFFFFFF000000LL; // mask to "round" addresses to consolidate near trampolines

  private:
    TrampolinePool();

    TrampolinePool& operator=(const TrampolinePool& reference); // not implemented

    /**
     * @brief allocates a buffer with read, write and execute rights near the
     * specified adress. The purpose is that we want to be able to jump from
     * adressNear to generated code with a 5-byte jump, even on x64 systems.
     * @param addressNear the reference adress
     * @note the resulting buffer is stored in the m_Buffers map
     */
    BufferMap::iterator allocateBuffer(LPVOID addressNear);

    void addBarrier(LPVOID rerouteAddr, LPVOID original, asmjit::X86Assembler& assembler);

#if IS_X64
    void copyCode(asmjit::X86Assembler& assembler, LPVOID source, size_t numBytes);
#endif // IS_X64

    BufferList& getBufferList(LPVOID address);

    LPVOID roundAddress(LPVOID address) const;

  public:
    static LPVOID __stdcall barrier(LPVOID function);
    static LPVOID __stdcall release(LPVOID function);

    LPVOID barrierInt(LPVOID function);
    LPVOID releaseInt(LPVOID function);

    void addCallToStub(asmjit::X86Assembler& assembler, LPVOID original, LPVOID reroute);

  private:
    ///
    /// \brief add a jump to an address outside the custom generated asm code (without modifying registers)
    /// \param assembler the assembler generator to write to
    /// \param destination destination address
    /// \note this currently generates a lot code on x64, may be overly complicated
    ///
    void addAbsoluteJump(asmjit::X86Assembler& assembler, uint64_t destination);

    DWORD determinePageSize();

  private:
#if IS_X64
    static const int SIZE_OF_JUMP = 13;
#elif !IS_X64
    static const int SIZE_OF_JUMP = 5;
#endif

    static TrampolinePool* s_Instance;

    bool m_FullBlock{false};

    BufferMap m_Buffers;

    typedef std::map<void*, void*> TThreadMap;
    detail::thread_specific_ptr<TThreadMap> m_ThreadGuards;

    LPVOID m_BarrierAddr;
    LPVOID m_ReleaseAddr;

    DWORD m_BufferSize = {1024};
    size_t m_SearchRange;
    uint64_t m_AddressMask;

    int m_MaxTrampolineSize;
};

} // namespace HookLib
