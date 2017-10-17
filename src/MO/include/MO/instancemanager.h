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
#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
namespace fs = std::experimental::filesystem;
class QSettings;

// FIXME: Hack. Exceptions are not flow control, and cancelling isnt exceptional.
class Canceled : std::exception {
public:
    Canceled() : std::exception("Canceled") {}
};

// Manages MO instances.
// A Mod Organizer Instance is a setup of Mod Organizer, profiles, downloads, mods, etc.
// This can be, for example, one instance per game.
// The currently selected Instance is stored in a system-wide configuration.
// UNLESS this Mod Organizer instance is being used in Portable mode, in which case
// No system-wide changes are to happen.
class InstanceManager {
public:
    InstanceManager();
    ~InstanceManager();

    // Determine the directory for the current instance.
    // Checks if MO is operating in portable mode.
    // Checks if a current instance exists, and prompts the user to create one if one does not.
    fs::path determineInstancePath();

    // Clear the saved current instance.
    // This is a noop for a portable install.
    void clearCurrentInstance();

private:
    // Return whether MO is being used in Portable mode.
    // Portable mode is determined by the existence of an Ini file in the application directory.
    bool portableMode() const;

    // Create the settings object if it does not already exist.
    void createSettings();

    // Return true if there is a saved current instance
    // Return false if there is not or if there could not plausibly be.
    // IE, if nothing is returned from instances() then, regardless of any saved instance,
    // It is impossible for an instance to exist.
    // This function will create the QSetting object if needed.
    bool hasCurrentInstance();

    // Return the name of the Instance currently in use.
    // Returns an empty string if none exists.
    std::string currentInstance() const;

    // Set the current instance.
    void setCurrentInstance(const std::string& name);

    // Return the base path of the location where Instances are created.
    // This is %LOCALAPPDATA%/ModOrganizer
    fs::path instancePath() const;

    // Ask the user for the name of their new Instance.
    std::string chooseNewInstance() const;

    // Choose either an existing instance or create a new one.
    // Returns the name of the selected instance
    // Returns an empty string if the user selects a Portable Install.
    std::string chooseInstance() const;

    // Returns a vector of instance names.
    // The orginial path can be obtained as instancePath() / instances()[N]
    std::vector<std::string> instances() const;

private:
    // Settings key for the Current Instance.
    static const std::string InstanceKey;
    // This will only be created if we're not in portable mode.
    std::unique_ptr<QSettings> m_settings;
};
