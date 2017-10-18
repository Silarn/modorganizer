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

// Uses a win32 mutex to determine if there are multiple instances of MO currently open.
class SingleInstance {
public:
    SingleInstance();
    ~SingleInstance();

    SingleInstance(const SingleInstance&) = delete;
    SingleInstance(SingleInstance&&) = delete;
    SingleInstance& operator=(const SingleInstance&) = delete;
    SingleInstance& operator=(SingleInstance&&) = delete;

public:
    // Return whether this is the primary instance or not.
    bool primary() const { return m_primary; }

private:
    static const std::string m_mutexid;
    bool m_primary = false;  // True = primary instance. False = secondary instance.
    void* m_mutex = nullptr; // Windows HANDLE is just a void pointer.
};
