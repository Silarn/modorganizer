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
#pragma once
#include <string>

namespace AppConfig {

#define APPPARAM(partype, parid, value)                                                                                \
    partype parid() { return value; }

APPPARAM(std::wstring, translationPrefix, L"organizer")
APPPARAM(std::wstring, pluginPath, L"plugins")
APPPARAM(std::wstring, profilesPath, L"profiles")
APPPARAM(std::wstring, modsPath, L"mods")
APPPARAM(std::wstring, downloadPath, L"downloads")
APPPARAM(std::wstring, overwritePath, L"overwrite")
APPPARAM(std::wstring, stylesheetsPath, L"stylesheets")
APPPARAM(std::wstring, cachePath, L"webcache")
APPPARAM(std::wstring, tutorialsPath, L"tutorials")
APPPARAM(std::wstring, logPath, L"logs")
APPPARAM(std::wstring, profileTweakIni, L"profile_tweaks.ini")
APPPARAM(std::wstring, logFileName, L"ModOrganizer.log")
APPPARAM(std::wstring, iniFileName, L"ModOrganizer.ini")
APPPARAM(std::wstring, proxyDLLTarget, L"steam_api.dll")
APPPARAM(std::wstring, proxyDLLOrig,
         L"steam_api_orig.dll") // needs to be identical to the value used in proxydll-project
APPPARAM(std::wstring, proxyDLLSource, L"proxy.dll")
APPPARAM(const wchar_t*, localSavePlaceholder, L"__MOProfileSave__\\")

APPPARAM(std::wstring, firstStepsTutorial, L"tutorial_firststeps_main.js")
APPPARAM(int, numLogFiles, 5) // number of log files to keep

#undef APPPARAM

} // namespace AppConfig
