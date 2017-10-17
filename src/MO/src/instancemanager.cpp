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
#include "MO/instancemanager.h"
#include "MO/selectiondialog.h"

#include <MO/shared/appconfig.h>
#include <common/sane_windows.h>
#include <common/util.h>

#include <QDialog>
#include <QInputDialog>
#include <QObject>
#include <QRegExp>
#include <QSettings>
#include <QString>
#include <QVariant>

#include <assert.h>
#include <cstdint>

const std::string InstanceManager::InstanceKey = "CurrentInstance";

InstanceManager::InstanceManager() {}
InstanceManager::~InstanceManager() {}

fs::path InstanceManager::determineInstancePath() {
    // Return the application directory if we're in Portable mode.
    if (portableMode()) {
        return common::get_exe_dir();
    }
    // If we have a current instance, good, return that!
    if (hasCurrentInstance()) {
        return instancePath() / currentInstance();
    }
    // Otherwise, ask the user to pick a new instance.
    auto instance = chooseInstance();
    // User selected Portable install.
    if (instance.empty()) {
        return common::get_exe_dir();
    }
    setCurrentInstance(instance);
    auto path = instancePath() / instance;
    fs::create_directories(path);
    return path;
}

void InstanceManager::clearCurrentInstance() {
    if (portableMode()) {
        return;
    }
    createSettings();
    setCurrentInstance("");
}

bool InstanceManager::portableMode() const { return fs::exists(common::get_exe_dir() / AppConfig::iniFileName()); }

void InstanceManager::createSettings() {
    if (!m_settings) {
        m_settings.reset(new QSettings("Tannin", "Mod Organizer"));
    }
}

bool InstanceManager::hasCurrentInstance() {
    if (instances().empty()) {
        return false;
    }
    createSettings();
    return !currentInstance().empty();
}

std::string InstanceManager::currentInstance() const {
    assert(m_settings);
    return m_settings->value(QString::fromStdString(InstanceKey), "").toString().toStdString();
}

void InstanceManager::setCurrentInstance(const std::string& name) {
    assert(m_settings);
    m_settings->setValue(QString::fromStdString(InstanceKey), QString::fromStdString(name));
}

fs::path InstanceManager::instancePath() const { //
    auto bufsize = ::GetEnvironmentVariableW(L"LOCALAPPDATA", NULL, 0);
    std::wstring path;
    path.resize(bufsize);
    ::GetEnvironmentVariableW(L"LOCALAPPDATA", path.data(), static_cast<DWORD>(path.size()));
    path.resize(path.size() - 1); // Null terminator.
    return fs::path(path) / "ModOrganizer";
}

std::string InstanceManager::chooseNewInstance() const {
    // FIXME: Would be nice to eliminate this entirely and support it in MO itself.
    // IE, proper seperate profiles rather than seperate instances emulating it.
    std::string instanceId;
    QInputDialog dialog;
    // FIXME: Would be neat if we could take the names from the game plugins but
    // the required initialization order requires the ini file to be
    // available *before* we load plugins
    // dialog.setComboBoxItems({"Oblivion", "Skyrim", "SkyrimSE", "Fallout 3", "Fallout NV", "Fallout 4"});
    // dialog.setComboBoxEditable(true);
    dialog.setWindowTitle(QObject::tr("Enter Instance Name"));
    dialog.setLabelText(QObject::tr("Name"));
    dialog.resize(dialog.size().width() - 400, dialog.size().height());
    if (dialog.exec() == QDialog::Rejected) {
        throw Canceled();
    }
    // TODO: Remove Special Characters utility function.
    instanceId = dialog.textValue().replace(QRegExp("[^0-9a-zA-Z ]"), "").toStdString();
    return instanceId;
}

std::string InstanceManager::chooseInstance() const {
    enum class Special : uint8_t { NewInstance, Portable };

    // Setup the selection dialog.
    SelectionDialog selection(QString("<h3>%1</h3><br>%2")
                                  .arg(QObject::tr("Choose Instance"))
                                  .arg(QObject::tr("Each Instance is a full set of MO data files (mods, "
                                                   "downloads, profiles, configuration, ...). Use multiple "
                                                   "instances for different games. If your MO folder is "
                                                   "writable, you can also store a single instance locally (called "
                                                   "a portable install).")));
    // Disabling cancelling and closing the dialoge.
    selection.disableCancel();
    // Add choices from existing instances
    for (const auto& instance : instances()) {
        auto tmp = QString::fromStdString(instance);
        selection.addChoice(tmp, "", tmp);
    }
    // Add the New option.
    selection.addChoice(QObject::tr("New"), QObject::tr("Create a new instance."),
                        static_cast<uint8_t>(Special::NewInstance), QIcon(":/MO/gui/add"));
    // Add the Portable option, if the MO directory is writeable.
    if (common::is_writeable(common::get_exe_dir())) {
        selection.addChoice(QObject::tr("Portable"), QObject::tr("Use MO folder for data."),
                            static_cast<uint8_t>(Special::Portable), QIcon(":/MO/gui/package"));
    }
    if (selection.exec() == QDialog::Rejected) {
        throw Canceled();
    }
    // Get the Users Choice.
    QVariant choice = selection.getChoiceData();
    // Existing instance selected and returned.
    if (choice.type() == QVariant::String) {
        return choice.toString().toStdString();
    }
    // New Instance, either portable or global.
    switch (static_cast<Special>(choice.value<uint8_t>())) {
    case Special::NewInstance:
        // TODO: Allow the user to cancel this and go back to the selection menu.
        return chooseNewInstance();
    case Special::Portable:
        return {};
    }
    assert(0); // Should not get here.
}

std::vector<std::string> InstanceManager::instances() const {
    std::vector<std::string> tmp;
    for (const auto& p : fs::directory_iterator(instancePath())) {
        if (fs::is_directory(p)) {
            tmp.push_back(p.path().filename().string());
        }
    }
    return tmp;
}
