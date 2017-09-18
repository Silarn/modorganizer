/*
Copyright (C) 2012 Sebastian Herbord. All rights reserved.

This file is part of Mod Organizer.

Mod Organizer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Mod Organizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mod Organizer.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "hookdll/skyriminfo.h"
#include "MO/shared/error_report.h"
#include "MO/shared/util.h"
#include "MO/shared/windows_error.h"
#include <common/sane_windows.h>
#include <sstream>
#include <string>

namespace MOShared {

SkyrimInfo::SkyrimInfo(const std::wstring& gameDirectory) : GameInfo(gameDirectory) {
    identifyMyGamesDirectory(L"skyrim");

    // TODO: Make sure this works.
    std::string tmp = std::getenv("LOCALAPPDATA");
    m_AppData.resize(tmp.size());
    std::mbstowcs(m_AppData.data(), tmp.data(), m_AppData.size());
    // wchar_t appDataPath[MAX_PATH];
    //if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, appDataPath))) {
    //    m_AppData = appDataPath;
    //}
}

bool SkyrimInfo::identifyGame(const std::wstring& searchPath) {
    return FileExists(searchPath, L"TESV.exe") && FileExists(searchPath, L"SkyrimLauncher.exe");
}

std::wstring SkyrimInfo::getRegPathStatic() {
    HKEY key;
    LONG errorcode =
        ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Bethesda Softworks\\Skyrim", 0, KEY_QUERY_VALUE, &key);

    if (errorcode != ERROR_SUCCESS) {
        return std::wstring();
    }

    WCHAR temp[MAX_PATH];
    DWORD bufferSize = MAX_PATH;

    if (::RegQueryValueExW(key, L"Installed Path", nullptr, nullptr, (LPBYTE)temp, &bufferSize) == ERROR_SUCCESS) {
        return std::wstring(temp);
    } else {
        return std::wstring();
    }
}

std::vector<std::wstring> SkyrimInfo::getIniFileNames() const { return {L"skyrim.ini", L"skyrimprefs.ini"}; }

std::wstring SkyrimInfo::getReferenceDataFile() const { return L"Skyrim - Meshes.bsa"; }

bool SkyrimInfo::rerouteToProfile(const wchar_t* fileName, const wchar_t* fullPath) const {
    static LPCWSTR profileFiles[] = {L"skyrim.ini", L"skyrimprefs.ini", L"loadorder.txt", nullptr};

    for (int i = 0; profileFiles[i] != nullptr; ++i) {
        if (_wcsicmp(fileName, profileFiles[i]) == 0) {
            return true;
        }
    }

    // TODO: Replace StrStrIW with std::string thing. string.find i think.
    // if ((_wcsicmp(fileName, L"plugins.txt") == 0) && (m_AppData.empty() || (StrStrIW(fullPath, m_AppData.c_str()) !=
    // nullptr))) {
    if ((_wcsicmp(fileName, L"plugins.txt") == 0) && (m_AppData.empty())) {
        return true;
    }

    return false;
}

} // namespace MOShared
