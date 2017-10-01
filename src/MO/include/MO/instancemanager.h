/*
Copyright (C) 2016 Sebastian Herbord. All rights reserved.

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
#include <QSettings>

#include <filesystem>
#include <list>
#include <string>
namespace fs = std::experimental::filesystem;

// Singleton class managing certain non application specific settings and instances.
// Settings such as where the application settings are and which ones to use?
// TODO: Isnt there already a Settings class? May need to consoloidate them.
class InstanceManager {
public:
    // Return the Instance.
    static InstanceManager& instance();

    // Determine the Application data directory for the current instance.
    fs::path determineDataPath();
    void clearCurrentInstance();

private:
    InstanceManager();

    // Set or get the current instance value
    std::string currentInstance() const;
    void setCurrentInstance(const std::string& name);

    // Let the User choose an Instance to be used.
    std::string chooseInstance(const std::list<std::string>& instanceList) const;

    // Get a list of possible instances
    std::list<std::string> instances() const;

    // Return the base instance path for non portable installs.
    fs::path instancePath() const;

    // Ask the user for the name of their Instance.
    std::string queryInstanceName() const;
    // Create a new Instance
    void createDataPath(const fs::path& dataPath) const;
    // Determine if this is a portable install.
    bool portableInstall() const;

private:
    QSettings m_AppSettings;
    bool m_Reset = false;
};
