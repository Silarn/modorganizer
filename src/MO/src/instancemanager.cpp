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

#include <MO/Shared/appconfig.h>
#include <common/util.h>
#include <uibase/utility.h>

#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QObject>
#include <QRegExp>
#include <QString>
#include <QVariant>

#include <cstdint>
#include <stdexcept>

static const char COMPANY_NAME[] = "Tannin";
static const char APPLICATION_NAME[] = "Mod Organizer";
static const char INSTANCE_KEY[] = "CurrentInstance";

InstanceManager::InstanceManager() : m_AppSettings(COMPANY_NAME, APPLICATION_NAME) {}

InstanceManager& InstanceManager::instance() {
    static InstanceManager s_Instance;
    return s_Instance;
}

std::string InstanceManager::currentInstance() const {
    return m_AppSettings.value(INSTANCE_KEY, "").toString().toStdString();
}

void InstanceManager::clearCurrentInstance() {
    setCurrentInstance("");
    m_Reset = true;
}

void InstanceManager::setCurrentInstance(const std::string& name) {
    m_AppSettings.setValue(INSTANCE_KEY, QString::fromStdString(name));
}

std::string InstanceManager::queryInstanceName() const {
    // FIXME: WOuld be nice to eliminate this entirely and support it in MO itself.
    // IE, proper seperate profiles rather than seperate instances emulating it.
    QString instanceId;
    while (instanceId.isEmpty()) {
        QInputDialog dialog;
        // FIXME: Would be neat if we could take the names from the game plugins but
        // the required initialization order requires the ini file to be
        // available *before* we load plugins
        dialog.setComboBoxItems({"Oblivion", "Skyrim", "SkyrimSE", "Fallout 3", "Fallout NV", "Fallout 4"});
        dialog.setComboBoxEditable(true);
        dialog.setWindowTitle(QObject::tr("Enter Instance Name"));
        dialog.setLabelText(QObject::tr("Name"));
        if (dialog.exec() == QDialog::Rejected) {
            throw MOBase::MyException(QObject::tr("Canceled"));
        }
        instanceId = dialog.textValue().replace(QRegExp("[^0-9a-zA-Z ]"), "");
    }
    return instanceId.toStdString();
}

std::string InstanceManager::chooseInstance(const std::list<std::string>& instanceList) const {
    enum class Special : uint8_t { NewInstance, Portable };

    SelectionDialog selection(QString("<h3>%1</h3><br>%2")
                                  .arg(QObject::tr("Choose Instance"))
                                  .arg(QObject::tr("Each Instance is a full set of MO data files (mods, "
                                                   "downloads, profiles, configuration, ...). Use multiple "
                                                   "instances for different games. If your MO folder is "
                                                   "writable, you can also store a single instance locally (called "
                                                   "a portable install).")));
    // Disable the cancel button. It's an error to cancel.
    selection.disableCancel();
    // Add choices.
    for (const auto& instance : instanceList) {
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
    // Throw an exception if the user exists the dialog.
    if (selection.exec() == QDialog::Rejected) {
        qDebug("rejected");
        throw MOBase::MyException(QObject::tr("Canceled"));
    }
    // Get the Users Choice.
    QVariant choice = selection.getChoiceData();
    if (choice.type() == QVariant::String) {
        return choice.toString().toStdString();
    } else {
        switch (static_cast<Special>(choice.value<uint8_t>())) {
        case Special::NewInstance:
            return queryInstanceName();
        case Special::Portable:
            return {};
        default:
            throw std::runtime_error("invalid selection");
        }
    }
}

fs::path InstanceManager::instancePath() const { return fs::path(std::getenv("LOCALAPPDATA")) / "ModOrganizer"; }

std::list<std::string> InstanceManager::instances() const {
    std::list<std::string> tmp;
    for (auto& p : fs::directory_iterator(instancePath())) {
        if (!fs::is_directory(p)) {
            continue;
        }
        tmp.push_back(p.path().filename().string());
    }
    return tmp;
}

bool InstanceManager::portableInstall() const { return fs::exists(common::get_exe_dir() / AppConfig::iniFileName()); }

void InstanceManager::createDataPath(const fs::path& dataPath) const {
    if (!fs::exists(dataPath)) {
        if (!fs::create_directories(dataPath)) {
            throw MOBase::MyException(
                QObject::tr("failed to create %1").arg(QString::fromStdWString(dataPath.wstring())));
        } else {
            QMessageBox::information(nullptr, QObject::tr("Data directory created"),
                                     QObject::tr("New data directory created at %1. If you don't want to "
                                                 "store a lot of data there, reconfigure the storage "
                                                 "directories via settings.")
                                         .arg(QString::fromStdWString(dataPath.wstring())));
        }
    }
}

fs::path InstanceManager::determineDataPath() {
    std::string instanceId = currentInstance();
    if (instanceId.empty() && portableInstall() && !m_Reset) {
        // startup, apparently using portable mode before
        return common::get_exe_dir();
    }
    fs::path dataPath = instancePath() / instanceId;

    // Choose an instance if saved one does not exist or missing.
    if (instanceId.empty() || !fs::exists(dataPath)) {
        instanceId = chooseInstance(instances());
        if (!instanceId.empty()) {
            dataPath = instancePath() / instanceId;
        }
    }

    if (instanceId.empty()) {
        return common::get_exe_dir();
    }
    // Save the current Instance and create the required folders.
    setCurrentInstance(instanceId);
    createDataPath(dataPath);

    return dataPath;
}
