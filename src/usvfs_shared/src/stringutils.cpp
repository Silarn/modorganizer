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
#include "usvfs_shared/stringutils.h"
#include "usvfs_shared/windows_error.h"

#include <common/sane_windows.h>
#include <common/stringutils.h>
#include <fmt/format.h>

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

void usvfs::shared::strncpy_sz(char* dest, const char* src, size_t destSize) {
    if (destSize > 0) {
        strncpy(dest, src, destSize - 1);
        dest[destSize - 1] = '\0';
    }
}

void usvfs::shared::wcsncpy_sz(wchar_t* dest, const wchar_t* src, size_t destSize) {
    if ((destSize > 0) && (dest != nullptr)) {
        wcsncpy(dest, src, destSize - 1);
        dest[destSize - 1] = L'\0';
    }
}

bool usvfs::shared::startswith(const wchar_t* string, const wchar_t* subString) {
    while ((*string != '\0') && (*subString != '\0')) {
        if (towlower(*string) != towlower(*subString)) {
            return false;
        }
        ++string;
        ++subString;
    }

    return *subString == '\0';
}

fs::path usvfs::shared::make_relative(const fs::path& fromIn, const fs::path& toIn) {
    // TODO: Use std::filesystem::relative when MSVC Implements it?
    // return fs::relative(fromIn, toIn);
    // converting path to lower case to make iterator comparison work correctly
    // on case-insenstive filesystems
    fs::path from(fs::absolute(fromIn));
    fs::path to(fs::absolute(toIn));

    // find common base
    fs::path::const_iterator fromIter(from.begin());
    fs::path::const_iterator toIter(to.begin());
    fs::path::const_iterator fromIterEnd = from.end();
    fs::path::const_iterator toIterEnd = to.end();

    // TODO the following equivalent test is probably quite expensive as new
    // paths are created for each iteration but the case sensitivity depends on the fs
    while ((fromIter != fromIterEnd) && (toIter != toIterEnd) &&
           (common::iequals(fromIter->string(), toIter->string()))) {
        ++fromIter;
        ++toIter;
    }

    // Navigate backwards in directory to reach previously found base
    fs::path result;
    for (; fromIter != fromIterEnd; ++fromIter) {
        if (*fromIter != ".") {
            result /= "..";
        }
    }

    // Now navigate down the directory branch
    for (; toIter != toIterEnd; ++toIter) {
        result /= *toIter;
    }
    return result;
}

std::string usvfs::shared::to_hex(void* bufferIn, size_t bufferSize) {
    unsigned char* buffer = static_cast<unsigned char*>(bufferIn);
    std::ostringstream temp;
    temp << std::hex;
    for (size_t i = 0; i < bufferSize; ++i) {
        temp << fmt::format("{:0>2X}", static_cast<unsigned int>(buffer[i]));
        if ((i % 16) == 15) {
            temp << "\n";
        } else {
            temp << " ";
        }
    }
    return temp.str();
}

std::wstring usvfs::shared::to_upper(const std::wstring& input) {
    std::wstring result;
    result.resize(input.size());
    ::LCMapStringEx(LOCALE_NAME_INVARIANT, LCMAP_UPPERCASE, input.data(), static_cast<int>(input.size()), &result[0],
                    static_cast<int>(result.size()), NULL, NULL, 0);
    return result;
}
