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
#include "MO/Shared/appconfig.h"
#include "MO/Shared/windows_error.h"
#include "MO/helper.h"
#include "MO/instancemanager.h"
#include "MO/logbuffer.h"
#include "MO/mainwindow.h"
#include "MO/moapplication.h"
#include "MO/nxmaccessmanager.h"
#include "MO/selectiondialog.h"
#include "MO/singleinstance.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QSplashScreen>
#include <common/predef.h>
#include <common/stringutils.h>
#include <fmt/format.h>
#include <uibase/report.h>
#include <uibase/tutorialmanager.h>

#include <DbgHelp.h>
#include <ShellAPI.h>
#include <array>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::experimental::filesystem;

#pragma comment(linker, "/manifestdependency:\"name='dlls' version='1.0.0.0' type='win32'\"")

using namespace MOBase;
using namespace MOShared;

// Debug output will be logged here as well as to the console.
// This way it can be used in the Minidump.
static thread_local std::stringstream errorLog;

// Create directory and make sure it's writeable.
static bool createAndMakeWritable(const fs::path& fullPath) {
    QString const dataPath = qApp->property("dataPath").toString();
    if (!fs::exists(fullPath)) {
        try {
            fs::create_directories(fullPath);
        } catch (const fs::filesystem_error&) {
            return false;
        }
    }
    return true;
}

// Bootstraping code
// Creates required directories, removes old files, verifies can start.
static bool bootstrap() {
    // remove the temporary backup directory in case we're restarting after an update
    fs::path backupDirectory = qApp->applicationDirPath().toStdString() / fs::path("update_backup");
    if (fs::exists(backupDirectory)) {
        fs::remove_all(backupDirectory);
    }
    fs::path dataPath = qApp->property("dataPath").toString().toStdString();

    // Remove all logfiles matching ModOrganizer*.log, except for five. Sorted by name.
    fs::path logPath = dataPath / AppConfig::logPath();
    removeOldFiles(QString::fromStdString(logPath.string()), "usvfs*.log", 5, QDir::Name);

    if (!createAndMakeWritable(AppConfig::logPath())) {
        return false;
    }

    return true;
}

// Determines if the string `link` is a nexus link.
bool isNxmLink(const QString& link) { return link.startsWith("nxm://", Qt::CaseInsensitive); }

static BOOL CALLBACK MyMiniDumpCallback(PVOID pParam, const PMINIDUMP_CALLBACK_INPUT pInput,
                                        PMINIDUMP_CALLBACK_OUTPUT pOutput) {
    BOOL bRet = FALSE;

    // Check parameters
    if (pInput == 0 || pOutput == 0) {
        return FALSE;
    }

    // Process the callbacks
    switch (pInput->CallbackType) {
    case IncludeModuleCallback: {
        // Include the module into the dump
        bRet = TRUE;
    } break;

    case IncludeThreadCallback: {
        // Include the thread into the dump
        bRet = TRUE;
    } break;

    case ModuleCallback: {
        // Does the module have ModuleReferencedByMemory flag set ?
        if (!(pOutput->ModuleWriteFlags & ModuleReferencedByMemory)) {
            // No, it does not - exclude it
            wprintf(L"Excluding module: %s \n", pInput->Module.FullPath);
            pOutput->ModuleWriteFlags &= (~ModuleWriteModule);
        }
        bRet = TRUE;
    } break;

    case ThreadCallback: {
        // Include all thread information into the minidump
        bRet = TRUE;
    } break;

    case ThreadExCallback: {
        // Include this information
        bRet = TRUE;
    } break;

    case MemoryCallback: {
        // We do not include any information here -> return FALSE
        bRet = FALSE;
    } break;

    case CancelCallback:
        break;
    }

    return bRet;
}

static std::string CreateMiniDump(std::wstring dumpname, EXCEPTION_POINTERS* exceptionPtrs) {
    std::string errorMsg;
    // Create Dump File.
    fs::path dumpPath = LR"(\\?\)" + dumpname;
    HANDLE dumpFile = ::CreateFileW(dumpPath.native().data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, nullptr,
                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (dumpFile != INVALID_HANDLE_VALUE) {
        _MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
        exceptionInfo.ThreadId = ::GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = exceptionPtrs;
        exceptionInfo.ClientPointers = FALSE;
        _MINIDUMP_USER_STREAM_INFORMATION userInfo;
        std::string txt = errorLog.str();
        std::array<_MINIDUMP_USER_STREAM, 1> tmp = {
            {CommentStreamA, txt.size() + 1, txt.data()},
        };
        userInfo.UserStreamCount = tmp.size();
        userInfo.UserStreamArray = &tmp[0];
        _MINIDUMP_CALLBACK_INFORMATION callbackInfo;
        callbackInfo.CallbackRoutine = &MyMiniDumpCallback;
        callbackInfo.CallbackParam = 0;

        MINIDUMP_TYPE mtype =
            static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory);
        BOOL success = MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), dumpFile, mtype,
                                         &exceptionInfo, &userInfo, &callbackInfo);

        ::FlushFileBuffers(dumpFile);
        ::CloseHandle(dumpFile);
        if (!success) {
            errorMsg = QString("failed to save minidump to %1 (error %2)")
                           .arg(QString::fromStdWString(dumpPath.wstring()))
                           .arg(::GetLastError())
                           .toStdString();
        }
    } else {
        errorMsg = QString("failed to create %1 (error %2)")
                       .arg(QString::fromStdWString(dumpPath.wstring()))
                       .arg(::GetLastError())
                       .toStdString();
    }
    return errorMsg;
}

// Error Handling for all Unhandled Exceptions.
// Writes a minidump and displays information to the User.
static LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* exceptionPtrs) {
#ifdef COMMON_IS_DEBUG
    // This is so we can step into the handler.
    QMessageBox::critical(nullptr, QObject::tr("Test"), QObject::tr("TEST"));
#endif
    LONG result = EXCEPTION_EXECUTE_HANDLER;
    std::wstring dumpName = qApp->applicationFilePath().append(".dmp").toStdWString();
    bool createDump =
        QMessageBox::question(nullptr, QObject::tr("Whoops!"),
                              QObject::tr("ModOrganizer has crashed! "
                                          "Should a diagnostic file be created? "
                                          "If you make an issue at https://github.com/ModOrganizer/modorganizer, "
                                          "including this file (%1), "
                                          "the bug is a lot more likely to be fixed. "
                                          "Please include a short description of what you were "
                                          "doing when the crash happened")
                                  .arg(QString::fromStdWString(dumpName))) == QMessageBox::Yes;
    if (createDump) {
        // Message to display to the user for why this couldnt be handled.
        std::string errorMsg = CreateMiniDump(dumpName, exceptionPtrs);
        if (!errorMsg.empty()) {
            QMessageBox::critical(
                nullptr, QObject::tr("Whoops!"),
                QObject::tr("ModOrganizer has crashed! Unfortunately I was not able to write a diagnostic file: %1")
                    .arg(QString::fromStdString(errorMsg)));
        }
    }
    return result;
}

// Test whether we have write access for path `path`
static bool HaveWriteAccess(const fs::path& path) {
    auto perms = fs::status(path).permissions();
    return (perms & fs::perms::owner_write) != fs::perms::none;
}

QString determineProfile(QStringList& arguments, const QSettings& settings) {
    QString selectedProfileName = QString::fromUtf8(settings.value("selected_profile", "").toByteArray());
    { // see if there is a profile on the command line
        int profileIndex = arguments.indexOf("-p", 1);
        if ((profileIndex != -1) && (profileIndex < arguments.size() - 1)) {
            qDebug("profile overwritten on command line");
            selectedProfileName = arguments.at(profileIndex + 1);
        }
        arguments.removeAt(profileIndex);
        arguments.removeAt(profileIndex);
    }
    if (selectedProfileName.isEmpty()) {
        qDebug("no configured profile");
        selectedProfileName = "Default";
    } else {
        qDebug("configured profile: %s", qPrintable(selectedProfileName));
    }

    return selectedProfileName;
}

MOBase::IPluginGame* selectGame(QSettings& settings, QDir const& gamePath, MOBase::IPluginGame* game) {
    settings.setValue("gameName", game->gameName());
    // Sadly, hookdll needs gamePath in order to run. So following code block is
    // commented out
    /*if (gamePath == game->gameDirectory()) {
    settings.remove("gamePath");
  } else*/ {
        QString gameDir = gamePath.absolutePath();
        game->setGamePath(gameDir);
        settings.setValue("gamePath", QDir::toNativeSeparators(gameDir).toUtf8().constData());
    }
    return game; // Woot
}

// Determine what game we are running where. Be very paranoid in case the
// user has done something odd.
// If the game name has been set up, use that.
MOBase::IPluginGame* determineCurrentGame(QString const& moPath, QSettings& settings, PluginContainer const& plugins) {
    QString gameName = settings.value("gameName", "").toString();
    if (!gameName.isEmpty()) {
        MOBase::IPluginGame* game = plugins.managedGame(gameName);
        if (game == nullptr) {
            reportError(QObject::tr("Plugin to handle %1 no longer installed").arg(gameName));
            return nullptr;
        }
        QString gamePath = QString::fromUtf8(settings.value("gamePath", "").toByteArray());
        if (gamePath == "") {
            gamePath = game->gameDirectory().absolutePath();
        }
        QDir gameDir(gamePath);
        if (game->looksValid(gameDir)) {
            return selectGame(settings, gameDir, game);
        }
    }

    // gameName wasn't set, or otherwise can't be found. Try looking through all
    // the plugins using the gamePath
    QString gamePath = QString::fromUtf8(settings.value("gamePath", "").toByteArray());
    if (!gamePath.isEmpty()) {
        QDir gameDir(gamePath);
        // Look to see if one of the installed games binary file exists in the current
        // game directory.
        for (IPluginGame* const game : plugins.plugins<IPluginGame>()) {
            if (game->looksValid(gameDir)) {
                return selectGame(settings, gameDir, game);
            }
        }
    }

    // OK, we are in a new setup or existing info is useless.
    // See if MO has been installed inside a game directory
    for (IPluginGame* const game : plugins.plugins<IPluginGame>()) {
        if (game->isInstalled() && moPath.startsWith(game->gameDirectory().absolutePath())) {
            // Found it.
            return selectGame(settings, game->gameDirectory(), game);
        }
    }

    // Try walking up the directory tree to see if MO has been installed inside a game
    {
        QDir gameDir(moPath);
        do {
            // Look to see if one of the installed games binary file exists in the current
            // directory.
            for (IPluginGame* const game : plugins.plugins<IPluginGame>()) {
                if (game->looksValid(gameDir)) {
                    return selectGame(settings, gameDir, game);
                }
            }
            // OK, chop off the last directory and try again
        } while (gameDir.cdUp());
    }

    // Then try a selection dialogue.
    if (!gamePath.isEmpty() || !gameName.isEmpty()) {
        reportError(QObject::tr("Could not use configuration settings for game \"%1\", path \"%2\".")
                        .arg(gameName)
                        .arg(gamePath));
    }

    SelectionDialog selection(QObject::tr("Please select the game to manage"), nullptr, QSize(32, 32));

    for (IPluginGame* game : plugins.plugins<IPluginGame>()) {
        if (game->isInstalled()) {
            QString path = game->gameDirectory().absolutePath();
            selection.addChoice(game->gameIcon(), game->gameName(), path, QVariant::fromValue(game));
        }
    }

    selection.addChoice(QString("Browse..."), QString(), QVariant::fromValue(static_cast<IPluginGame*>(nullptr)));

    while (selection.exec() != QDialog::Rejected) {
        IPluginGame* game = selection.getChoiceData().value<IPluginGame*>();
        if (game != nullptr) {
            return selectGame(settings, game->gameDirectory(), game);
        }

        gamePath = QFileDialog::getExistingDirectory(nullptr, QObject::tr("Please select the game to manage"),
                                                     QString(), QFileDialog::ShowDirsOnly);

        if (!gamePath.isEmpty()) {
            QDir gameDir(gamePath);
            for (IPluginGame* const game : plugins.plugins<IPluginGame>()) {
                if (game->looksValid(gameDir)) {
                    return selectGame(settings, gameDir, game);
                }
            }
            reportError(QObject::tr("No game identified in \"%1\". The directory is required to contain "
                                    "the game binary and its launcher.")
                            .arg(gamePath));
        }
    }

    return nullptr;
}

void myMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    std::string smsg =
        fmt::format("{:s} ({:s}:{:d}, {:s})\n", localMsg.constData(), context.file, context.line, context.function);
    switch (type) {
    case QtDebugMsg:
        smsg = fmt::format("Debug: {:s}", smsg);
        break;
    case QtInfoMsg:
        smsg = fmt::format("Info: {:s}", smsg);
        break;
    case QtWarningMsg:
        smsg = fmt::format("Warning: {:s}", smsg);
        break;
    case QtCriticalMsg:
        smsg = fmt::format("Critical: {:s}", smsg);
        break;
    case QtFatalMsg:
        smsg = fmt::format("Fatal: {:s}", smsg);
        break;
    }
    errorLog << smsg;
}

// extend path to include dll directory so plugins don't need a manifest
// (using AddDllDirectory would be an alternative to this but it seems fairly
// complicated esp.
//  since it isn't easily accessible on Windows < 8
//  SetDllDirectory replaces other search directories and this seems to
//  propagate to child processes)
void setupPath() {
    static const int BUFSIZE = 4096;
    qDebug("MO at: %s", qUtf8Printable(QDir::toNativeSeparators(QCoreApplication::applicationDirPath())));

    std::vector<TCHAR> oldPath(BUFSIZE);
    DWORD offset = ::GetEnvironmentVariableW(L"PATH", oldPath.data(), BUFSIZE);
    if (offset > BUFSIZE) {
        oldPath.clear();
        oldPath.resize(offset);
        ::GetEnvironmentVariableW(L"PATH", oldPath.data(), offset);
    }

    std::wstring newPath(oldPath.data());
    newPath += L";";
    newPath += ToWString(QDir::toNativeSeparators(QCoreApplication::applicationDirPath())).c_str();
    newPath += L"\\dlls";

    ::SetEnvironmentVariableW(L"PATH", newPath.c_str());
}

int runApplication(MOApplication& application, SingleInstance& instance, const QString& splashPath) {
    qDebug("start main application");
    // Display splash screen
    QPixmap pixmap(splashPath);
    QSplashScreen splash(pixmap);
    QString dataPath = application.property("dataPath").toString();
    qDebug("data path: %s", qUtf8Printable(dataPath));
    if (!bootstrap()) {
        reportError("failed to set up data paths");
        return 1;
    }
    QStringList arguments = application.arguments();
    try {
        qDebug("Working directory: %s", qUtf8Printable(QDir::toNativeSeparators(QDir::currentPath())));
        splash.show();
    } catch (const std::exception& e) {
        reportError(e.what());
        return 1;
    }
    try {
        // Setup settings
        fs::path settingsPath = fs::path(dataPath.toStdString()) / AppConfig::iniFileName();
        QSettings settings(QString::fromStdString(settingsPath.string()), QSettings::IniFormat);
        // Setup the Core application.
        qDebug("initializing core");
        OrganizerCore organizer(settings);
        if (!organizer.bootstrap()) {
            reportError("failed to set up data paths");
            return 1;
        }
        qDebug("initialize plugins");
        PluginContainer pluginContainer(&organizer);
        pluginContainer.loadPlugins();
        // Setup MO for game.
        MOBase::IPluginGame* game = determineCurrentGame(application.applicationDirPath(), settings, pluginContainer);
        if (game == nullptr) {
            return 1;
        }
        if (splashPath.startsWith(':')) {
            // currently using MO splash, see if the plugin contains one
            QString pluginSplash = QString(":/%1/splash").arg(game->gameShortName());
            QImage image(pluginSplash);
            if (!image.isNull()) {
                image.save(dataPath + "/splash.png");
            } else {
                qDebug("no plugin splash");
            }
        }
        organizer.setManagedGame(game);
        organizer.createDefaultProfile();
        if (!settings.contains("game_edition")) {
            QStringList editions = game->gameVariants();
            if (editions.size() > 1) {
                SelectionDialog selection(QObject::tr("Please select the game edition you have (MO can't start the "
                                                      "game correctly if this is set incorrectly!)"),
                                          nullptr);
                int index = 0;
                for (const QString& edition : editions) {
                    selection.addChoice(edition, "", index++);
                }
                if (selection.exec() == QDialog::Rejected) {
                    return 1;
                } else {
                    settings.setValue("game_edition", selection.getChoiceString());
                }
            }
        }
        game->setGameVariant(settings.value("game_edition").toString());
        qDebug("managing game at %s", qPrintable(QDir::toNativeSeparators(game->gameDirectory().absolutePath())));
        organizer.updateExecutablesList(settings);
        QString selectedProfileName = determineProfile(arguments, settings);
        organizer.setCurrentProfile(selectedProfileName);
        // if we have a command line parameter, it is either a nxm link or
        // a binary to start
        if (arguments.size() > 1) {
            if (isNxmLink(arguments.at(1))) {
                qDebug("starting download from command line: %s", qUtf8Printable(arguments.at(1)));
                organizer.externalMessage(arguments.at(1));
            } else {
                QString exeName = arguments.at(1);
                qDebug("starting %s from command line", qPrintable(exeName));
                arguments.removeFirst(); // remove application name (ModOrganizer.exe)
                arguments.removeFirst(); // remove binary name
                                         // pass the remaining parameters to the binary
                try {
                    organizer.startApplication(exeName, arguments, QString(), QString());
                    return 0;
                } catch (const std::exception& e) {
                    reportError(QObject::tr("failed to start application: %1").arg(e.what()));
                    return 1;
                }
            }
        }
        NexusInterface::instance()->getAccessManager()->startLoginCheck();
        qDebug("initializing tutorials");
        TutorialManager::init(
            qApp->applicationDirPath() + "/" + QString::fromStdWString(AppConfig::tutorialsPath()) + "/", &organizer);
        if (!application.setStyleFile(settings.value("Settings/style", "").toString())) {
            // disable invalid stylesheet
            settings.setValue("Settings/style", "");
        }
        int res = 1;
        { // scope to control lifetime of mainwindow
          // set up main window and its data structures
            MainWindow mainWindow(settings, organizer, pluginContainer);

            QObject::connect(&mainWindow, SIGNAL(styleChanged(QString)), &application, SLOT(setStyleFile(QString)));
            QObject::connect(&instance, SIGNAL(messageSent(QString)), &organizer, SLOT(externalMessage(QString)));

            mainWindow.readSettings();

            qDebug("displaying main window");
            mainWindow.show();
            splash.finish(&mainWindow);
            return application.exec();
        }
    } catch (const std::exception& e) {
        reportError(e.what());
        return 1;
    }
}

int main(int argc, char* argv[]) {
    // Install message handler for logging.
    qInstallMessageHandler(&myMessageOutput);
    // Unhandled Exception Error Handling.
    SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

    MOApplication application(argc, argv);
    QStringList arguments = application.arguments();

    if ((arguments.length() >= 4) && (arguments.at(1) == "launch")) {
        // all we're supposed to do is launch another process
        QProcess process;
        process.setWorkingDirectory(QDir::fromNativeSeparators(arguments.at(2)));
        process.setProgram(QDir::fromNativeSeparators(arguments.at(3)));
        process.setArguments(arguments.mid(4));
        process.start();
        process.waitForFinished(-1);
        return process.exitCode();
    }

    setupPath();

#if !defined(QT_NO_SSL)
    qDebug("ssl support: %d", QSslSocket::supportsSsl());
#else
    qDebug("non-ssl build");
#endif

    bool forcePrimary = false;
    if (arguments.contains("update")) {
        arguments.removeAll("update");
        forcePrimary = true;
    }

    SingleInstance instance(forcePrimary);
    if (!instance.primaryInstance()) {
        if ((arguments.size() == 2) && isNxmLink(arguments.at(1))) {
            qDebug("not primary instance, sending download message");
            instance.sendMessage(arguments.at(1));
            return 0;
        } else if (arguments.size() == 1) {
            QMessageBox::information(nullptr, QObject::tr("Mod Organizer"),
                                     QObject::tr("An instance of Mod Organizer is already running"));
            return 0;
        }
    } // we continue for the primary instance OR if MO was called with parameters

    do {
        QString dataPath;

        try {
            dataPath = InstanceManager::instance().determineDataPath();
        } catch (const std::exception& e) {
            QMessageBox::critical(nullptr, QObject::tr("Failed to set up instance"), e.what());
            return 1;
        }
        application.setProperty("dataPath", dataPath);

        // Setup logging
        // INFO: Calls qInstallMessageHandler, overwriting the one here.
        // Solution was to remove printing from ours and only log it for the minidump
        // And have LogBuffer call the old one.
        const fs::path logPath =
            qApp->property("dataPath").toString().toStdString() / fs::path("logs") / "mo_interface.log";
        LogBuffer::init(100, QtDebugMsg, QString::fromStdString(logPath.string()));

        QString splash = dataPath + "/splash.png";
        if (!QFile::exists(dataPath + "/splash.png")) {
            splash = ":/MO/gui/splash";
        }

        int result = runApplication(application, instance, splash);
        if (result != INT_MAX) {
            return result;
        }
        argc = 1;
    } while (true);
}
