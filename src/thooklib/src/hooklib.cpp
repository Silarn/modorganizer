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
#include "thooklib/hooklib.h"
#include "thooklib/utility.h"

#include <common/sane_windows.h>

#include <MinHook.h>

#include <common/predef.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <map>

#if COMMON_IS_64
#pragma message("64bit build")
#define JUMP_SIZE 13
#elif COMMON_IS_86
#pragma message("32bit build")
#define JUMP_SIZE 5
#else
#error "Unsupported Architecture"
#endif
#define IS_X64 COMMON_IS_64

using namespace HookLib;

struct THookInfo {
    LPVOID originalFunction;
    LPVOID replacementFunction;
    LPVOID detour;     // detour to call the original function after hook was installed.
    LPVOID trampoline; // code fragment that decides whether the replacement function or detour is executed (preventing
                       // endless loops)
    std::vector<uint8_t> preamble; // part of the detour that needs to be re-inserted into the original function to
                                   // return it to vanilla state
    bool stub; // if this is true, the trampoline calls the "replacement"-function that before the original function,
               // not instead of it
    enum {
        TYPE_HOTPATCH,   // official hot-patch variant as used on 32-bit windows
        TYPE_WIN64PATCH, // custom patch variant used on 64-bit windows
        TYPE_CHAINPATCH, // the hook is part of the hook chain (and not the first)
        TYPE_OVERWRITE,  // full jump overwrite used if none of the above work
        TYPE_RIPINDIRECT // the function already started on a rip-relative jump so we only modified that variable
    } type;
};

static std::map<HOOKHANDLE, THookInfo> s_Hooks;

static HOOKHANDLE GenerateHandle() {
    static ULONG NextHandle = 0;
    return ++NextHandle;
}

HOOKHANDLE applyHook(THookInfo info, HookError* error) {
    MH_CreateHook(info.originalFunction, info.replacementFunction, &info.trampoline);
    MH_EnableHook(info.originalFunction);

    HOOKHANDLE handle = GenerateHandle();
    s_Hooks[handle] = info;
    return handle;
}

HOOKHANDLE HookLib::InstallHook(LPVOID functionAddress, LPVOID hookAddress, HookError* error) {
    if (!functionAddress) {
        if (error) {
            *error = ERR_INVALIDPARAMETERS;
        }
        return INVALID_HOOK;
    }
    THookInfo info;
    info.originalFunction = functionAddress;
    info.replacementFunction = hookAddress;
    info.stub = false;
    info.detour = nullptr;
    info.trampoline = nullptr;
    info.type = THookInfo::TYPE_OVERWRITE;

    return applyHook(info, error);
}

HOOKHANDLE HookLib::InstallHook(HMODULE module, LPCSTR functionName, LPVOID hookAddress, HookError* error) {
    LPVOID funcAddr = MyGetProcAddress(module, functionName);
    return InstallHook(funcAddr, hookAddress, error);
}

void HookLib::RemoveHook(HOOKHANDLE handle) {
    auto iter = s_Hooks.find(handle);
    if (iter != s_Hooks.end()) {
        THookInfo info = iter->second;
        MH_RemoveHook(info.originalFunction);
        s_Hooks.erase(iter);
    } else {
        spdlog::get("usvfs")->info("handle unknown: {0:x}", handle);
    }
}

const char* HookLib::GetErrorString(HookError err) {
    switch (err) {
    case ERR_NONE:
        return "No Error";
    case ERR_INVALIDPARAMETERS:
        return "Invalid parameters";
    case ERR_FUNCEND:
        return "Function too short";
    case ERR_JUMP:
        return "Function starts on a jump";
    case ERR_RIP:
        return "RIP-relative addressing can't be relocated.";
    case ERR_RELJUMP:
        return "Relative Jump can't be relocated.";
    default:
        return "Unkown error code";
    }
}

const char* HookLib::GetHookType(HOOKHANDLE handle) {
    auto iter = s_Hooks.find(handle);
    if (iter != s_Hooks.end()) {
        THookInfo info = iter->second;
        switch (info.type) {
        case THookInfo::TYPE_HOTPATCH:
            return "hot patch";
        case THookInfo::TYPE_WIN64PATCH:
            return "64-bit hot patch";
        case THookInfo::TYPE_CHAINPATCH:
            return "chained patch";
        case THookInfo::TYPE_OVERWRITE:
            return "overwrite";
        case THookInfo::TYPE_RIPINDIRECT:
            return "rip indirection modified";
        default: {
            spdlog::get("usvfs")->error("invalid hook type {0}", info.type);
            return "invalid hook type";
        }
        }
    }
    return "invalid handle";
}

LPVOID HookLib::GetDetour(HOOKHANDLE handle) {
    auto iter = s_Hooks.find(handle);
    if (iter != s_Hooks.end()) {
        THookInfo info = iter->second;
        return info.detour;
    }
    return nullptr;
}
