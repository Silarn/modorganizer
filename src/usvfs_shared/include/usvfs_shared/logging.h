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
#include <cstdint>
#include <ostream>

enum class LogLevel : uint8_t { Debug, Info, Warning, Error };

// TODO according to the standard (17.4.3.1) I shouldn't add these to std but if they are in global namespace
// the lookup seems to fail?
namespace std {
ostream& operator<<(ostream& os, const wchar_t* str);
ostream& operator<<(ostream& os, wchar_t* str);
ostream& operator<<(ostream& os, const std::wstring& str);
} // namespace std