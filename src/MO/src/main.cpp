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
#include "MO/instancemanager.h"
#include "MO/moapplication.h"
#include "MO/singleinstance.h"

// Spdlog optimizations
#define SPDLOG_NO_THREAD_ID      // We don't use thread id.
#define SPDLOG_NO_REGISTRY_MUTEX // We don't use the registry.
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include <common/predef.h>
#include <common/sane_windows.h>
#include <common/stringutils.h>
#include <common/util.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <QDir>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QProcess>
#include <QSslSocket>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <DbgHelp.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
namespace fs = std::experimental::filesystem;
using namespace std::string_literals;

#pragma comment(linker, "/manifestdependency:\"name='dlls' version='1.0.0.0' type='win32'\"")

// All messages logged through the Log::Logger class are also logged here in a thread safe manner.
// This is so MyUnhandledExceptionFilter can access the log and include it in the dump.
static std::stringstream errorLog;

namespace Log {
// Logging specific details
namespace details {
// Implementation for setting up logging file sink.
// log_path is a required argument taking the path to the full path to the log file.
static spdlog::sink_ptr file_sink(fs::path log_path) {
    fs::path full_path(log_path);
    return std::make_shared<spdlog::sinks::simple_file_sink_mt>(full_path.string());
}
static spdlog::sink_ptr console_sink = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
static spdlog::sink_ptr minidump_sink = std::make_shared<spdlog::sinks::ostream_sink_mt>(errorLog);
} // namespace details

// An abstraction that handles logging.
// This class is thread safe.
class Logger {
public:
    // Create a new log file with the name `name` at path `log_path`
    // Log files must be unique, or else strange things may happen.
    // This is because spdlog requires logs to the same file to use the same sink, and a new sink is created each time
    // this is called.
    Logger(std::string filename, fs::path log_path) : m_name(filename), m_logPath(log_path) {
        // Make the path canoical and immune to working directory changes.
        log_path = fs::canonical(log_path);
        // First create the log directory.
        fs::create_directories(log_path);
        // Setup spdlog sinks.
        std::vector<spdlog::sink_ptr> sinks;
        // Add file sink.
        sinks.push_back(details::file_sink(log_path / (filename + ".log")));
        // Add Minidump sink.
        sinks.push_back(details::minidump_sink);
        // If debug configuration, log to the console as well.
#if COMMON_IS_DEBUG
        sinks.push_back(details::console_sink);
#endif
        // Create the Logger.
        m_logger = std::make_unique<spdlog::logger>(m_name, std::begin(sinks), std::end(sinks));
        // Change level to debug.
#if COMMON_IS_DEBUG
        m_logger->set_level(spdlog::level::debug);
#endif
    }

public:
    fs::path get_log_dir() { return m_logPath; }
    void flush() { m_logger->flush(); }
#pragma region Public Log API
    // Abosultely fatal error.
    // Flushes logger, terminates the program.
    template <typename... Args>
    void fatal(Args&&... args) {
        m_logger->critical(format(std::forward<Args>(args)...));
        m_logger->flush();
        std::terminate();
    }

    // The emitting component isn't working, or isn't working as intended.
    template <typename... Args>
    void error(Args&&... args) {
        m_logger->error(format(std::forward<Args>(args)...));
    }

    // The emitting component is working as intended, but an error may be imminent.
    template <typename... Args>
    void warn(Args&&... args) {
        m_logger->warn(format(std::forward<Args>(args)...));
    }

    // The emitting component has successfully completed an operation.
    template <typename... Args>
    void success(Args&&... args) {
        m_logger->info("Success: {0:s}", format(std::forward<Args>(args)...));
    }

    // Information thats only useful when debugging.
    template <typename... Args>
    void debug(Args&&... args) {
        m_logger->debug(format(std::forward<Args>(args)...));
    }

    // Everything else, doesn't reflect a change in the component status, just information about what it's doing.
    template <typename... Args>
    void info(Args&&... args) {
        m_logger->info(format(std::forward<Args>(args)...));
    }
#pragma endregion
private:
    std::unique_ptr<spdlog::logger> m_logger;
    std::string m_name;
    fs::path m_logPath;

protected:
    template <typename... Args>
    std::string format(Args&&... args) const {
        // spdlog passes all args to this internally
        return fmt::format(std::forward<Args>(args)...);
    }
};

} // namespace Log

static Log::Logger moLog("mo_interface", common::get_exe_dir() / "Logs");

//
#pragma region "Error / external log handling."
// Callback to filter information from the Minidump.
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

// Helper function to create the Mini Dump.
static std::string CreateMiniDump(std::wstring dumpname, EXCEPTION_POINTERS* exceptionPtrs) {
    std::string errorMsg;
    // Create Dump File.
    fs::path dumpPath = LR"(\\?\)" + dumpname;
    HANDLE dumpFile = CreateFileW(dumpPath.native().data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, nullptr,
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
            errorMsg = fmt::format("failed to save minidump to {} (error code: {})", dumpPath.string(), GetLastError());
        }
    } else {
        errorMsg = fmt::format("failed to create {} (error code: {})", dumpPath.string(), GetLastError());
    }
    return errorMsg;
}

// Error Handling for all Unhandled Exceptions.
// Writes a minidump and displays information to the User.
static LONG WINAPI MyUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* exceptionPtrs) {
#ifdef COMMON_IS_DEBUG
    // Windows doesnt call this hook if a debugger is attatched.
    // As a workaround, we can provide a hook point where execution flow is
    // paused so we can attatch a debugger INSIDE the hook.
    MessageBoxA(nullptr, "Hook Debugger Now.", "Debug Hook",
                MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
#endif
    LONG result = EXCEPTION_EXECUTE_HANDLER;
    // TODO: Make this immune to the working directory.
    // Some sort of global setting for the app path?
    fs::path dumpFile = fs::canonical(common::get_exe_dir() / fs::path("Logs") / "ModOrganizer.exe.dmp");
    auto msg = fmt::format("Should a diagnostic file be created? "
                           "If you make an issue at https://github.com/ModOrganizer/modorganizer, "
                           "including this file ({0:s}), "
                           "the bug is a lot more likely to be fixed. "
                           "Please include a short description of what you were "
                           "doing when the crash happened.",
                           dumpFile.string());
    bool createDump = MessageBoxA(nullptr, msg.data(), "Mod Organizer has crashed!", MB_YESNO | MB_ICONERROR) == IDYES;
    if (createDump) {
        std::string errorMsg = CreateMiniDump(dumpFile, exceptionPtrs);
        if (!errorMsg.empty()) {
            auto msg = fmt::format("Unfortunately I was not able to write the diagnostic file: {0:s}", errorMsg);
            MessageBoxA(nullptr, msg.data(), "Mod Organzier has crashed!", MB_OK | MB_ICONERROR);
        }
    }
    return result;
}

// Handle all internal Qt Logging.
void myMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    static Log::Logger logQ("QT", common::get_exe_dir() / "Logs");
    std::string smsg =
        fmt::format("{:s} ({:s}:{:d}, {:s})\n", msg.toStdString(), context.file, context.line, context.function);
    switch (type) {
    case QtDebugMsg:
        logQ.debug(smsg);
        break;
    case QtInfoMsg:
        logQ.info(smsg);
        break;
    case QtWarningMsg:
        logQ.warn(smsg);
        break;
    case QtCriticalMsg:
        logQ.error(smsg);
        break;
    case QtFatalMsg:
        logQ.fatal(smsg);
        break;
    }
}
#pragma endregion
//

// Extend the PATH enviroment variable for this process to include the dlls subdirectory.
// This way plugins don't need a manifest.
void setupPath() {
    // TODO: Possibly use SetDllDirectory?
    // May need to turn it off when loading other processes?
    // FIXME: This doesnt seem to be working.
    // The FO4 plugin requires liblz4.
    // It can't find it in the dlls directory without the manifest.
    fs::path appDirPath = common::get_exe_dir();
    moLog.info("Setting up PATH");

    auto bufsize = ::GetEnvironmentVariableW(L"PATH", NULL, 0);
    std::wstring path;
    path.resize(bufsize);
    ::GetEnvironmentVariableW(L"PATH", path.data(), path.size());
    moLog.debug("Old PATH: {}", common::toString(path));
    path[path.size() - 1] = ';';
    path += appDirPath / "dlls";
    moLog.debug("New PATH: {}", common::toString(path));

    auto err = ::SetEnvironmentVariableW(L"PATH", path.data());
    if (!err) {
        auto code = ::GetLastError();
        moLog.warn("Could not setup PATH. Error Code {}", code);
    }
}

// Determines if the string `link` is a nexus link.
bool isNxmLink(const QString& link) { return link.startsWith("nxm://", Qt::CaseInsensitive); }

#if 0
#include "MO/Shared/appconfig.h"
#include "MO/Shared/windows_error.h"
#include "MO/helper.h"
#include "MO/logbuffer.h"
#include "MO/mainwindow.h"
#include "MO/nxmaccessmanager.h"
#include "MO/selectiondialog.h"

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

using namespace MOBase;
using namespace MOShared;

// Debug output will be logged here as well as to the console.
// This way it can be used in the Minidump.
static thread_local std::stringstream errorLog;

// Create directory and make sure it's writeable.
static bool createAndMakeWritable(const fs::path& fullPath) {
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

    if (!createAndMakeWritable(logPath)) {
        return false;
    }

    return true;
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

#endif

int main(int argc, char* argv[]) {
    // This try...catch allows proper cleanup before rethrowing the
    // exception to let MyUnhandledExceptionFilter handle it.
    // This allows, for example, the log to be flushed flushed before the crash.
    try {
        // Setup Logging.
        moLog.info("Mod Organizer started.");
        moLog.info("MO Located At: {}", common::get_exe_dir());
        moLog.info("Setting up Exception Handlers and external logging wrappers.");
        // Exception and error handling.
        // Overwide the default windows crash behaviour.
        SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
        // Handle internal Qt logging.
        qInstallMessageHandler(&myMessageOutput);
        moLog.success("Handlers and logging successfully setup.");
        // Setup application
        moLog.info("Setting up Application and processing commandline");
        MOApplication application(argc, argv);
        QStringList arguments = application.arguments();
        // Handle launch argument.
        // First argument should be launch
        // Second should be the working directory
        // third the program to run
        // Fourth and onwards, arguments to the program.
        if ((arguments.length() >= 4) && (arguments.at(1) == "launch")) {
            // All we're supposed to do is launch another process, so do that.
            moLog.info("Launch argument passed.");
            auto wdir = QDir::fromNativeSeparators(arguments.at(2));
            auto prog = QDir::fromNativeSeparators(arguments.at(3));
            auto args = arguments.mid(4);
            moLog.info("Launching {} with arguments: '{}' and working directory '{}'", prog.toStdString(),
                       args.join(" ").toStdString(), wdir.toStdString());
            QProcess process;
            process.setWorkingDirectory(wdir);
            process.setProgram(prog);
            process.setArguments(args);
            process.start();
            process.waitForFinished(-1);
            return process.exitCode();
        }
        // Handle update argument.
        bool forcePrimary = false;
        if (arguments.contains("update")) {
            moLog.info("We updated! Forcing primary instance");
            arguments.removeAll("update");
            // Force ourselves to be the primary instance if we're updating.
            forcePrimary = true;
        }
        // Setup Paths.
        setupPath();
#if !defined(QT_NO_SSL)
        moLog.info("Qt supports SSL: {}", QSslSocket::supportsSsl());
#else
        moLog.info("Qt does not supports SSL.");
#endif
        moLog.info("Enforcing Single Instance");
        // Enforce a single Instance of MO.
        // Handle NXM Downloads
        // FIXME: Won't logging up until this point conflict, since they're writing to the same file.
        // Solution could be to enforce this earlier?
        SingleInstance instance(forcePrimary);
        if (!instance.primaryInstance()) {
            moLog.warn("Not Primary Instance");
            if ((arguments.size() == 2) && isNxmLink(arguments.at(1))) {
                moLog.info("Just handling a NXM Link");
                instance.sendMessage(arguments.at(1));
                return 0;
            } else if (arguments.size() == 1) {
                moLog.error("Duplicate Instance");
                QMessageBox::information(nullptr, QObject::tr("Mod Organizer"),
                                         QObject::tr("An instance of Mod Organizer is already running"));
                return 0;
            }
        } // We continue for the Primary Instance only.
        moLog.info("Primary Instance");
        // Find Mod Organizer data Directory.
        // In previous versions of MO this was the same place as the executable, but
        // Now it can be any location the User desires.
        // This handles the user choosing whether to use portable mode or a custom location or what.
        fs::path dataPath;
        try {
            dataPath = InstanceManager::instance().determineDataPath();
        } catch (const std::exception& e) {
            QMessageBox::critical(nullptr, QObject::tr("Failed to set up instance"), e.what());
            return 1;
        }
        application.setProperty("dataPath", QString::fromStdWString(dataPath.native()));
    } catch (...) {
        moLog.error("Mod Organizer crashed...");
        moLog.flush();
        throw;
    }
#if 0
    do {
        // Setup logging
        // INFO: Calls qInstallMessageHandler, overwriting the one here.
        // Solution was to remove printing from ours and only log it for the minidump
        // And have LogBuffer call the old one.
        const fs::path logPath =
            qApp->property("dataPath").toString().toStdString() / fs::path("logs") / "mo_interface.log";
        LogBuffer::init(100, QtDebugMsg, QString::fromStdString(logPath.string()));

        fs::path splash = dataPath / "splash.png";
        if (!fs::exists(splash)) {
            splash = ":/MO/gui/splash";
        }

        int result = runApplication(application, instance, QString::fromStdWString(splash.native()));
        if (result != INT_MAX) {
            return result;
        }
        argc = 1;
    } while (true);
#endif
}
