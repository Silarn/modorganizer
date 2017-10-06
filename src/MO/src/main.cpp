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
#include "MO/logging.h"
#include "MO/mainwindow.h"
#include "MO/moapplication.h"
#include "MO/nexusinterface.h"
#include "MO/nxmaccessmanager.h"
#include "MO/organizercore.h"
#include "MO/plugincontainer.h"
#include "MO/selectiondialog.h"
#include "MO/singleinstance.h"

#include <MO/Shared/appconfig.h>
#include <common/predef.h>
#include <common/sane_windows.h>
#include <common/stringutils.h>
#include <common/util.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <uibase/iplugingame.h>
#include <uibase/report.h>
#include <uibase/tutorialmanager.h>

#include <QDir>
#include <QFileDialog>
#include <QIcon>
#include <QMainWindow>
#include <QMessageBox>
#include <QMessageLogContext>
#include <QProcess>
#include <QSplashScreen>
#include <QSslSocket>
#include <QString>
#include <QStringList>
#include <QVariant>
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

//
#pragma region "Error / external log handling."
// Callback to filter information from the Minidump.
static BOOL CALLBACK MyMiniDumpCallback(PVOID, const PMINIDUMP_CALLBACK_INPUT pInput,
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
        _MINIDUMP_CALLBACK_INFORMATION callbackInfo;
        callbackInfo.CallbackRoutine = &MyMiniDumpCallback;
        callbackInfo.CallbackParam = 0;

        MINIDUMP_TYPE mtype =
            static_cast<MINIDUMP_TYPE>(MiniDumpNormal | MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory);
        BOOL success = MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), dumpFile, mtype,
                                         &exceptionInfo, nullptr, &callbackInfo);

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
            auto tmp = fmt::format("Unfortunately I was not able to write the diagnostic file: {0:s}", errorMsg);
            MessageBoxA(nullptr, tmp.data(), "Mod Organzier has crashed!", MB_OK | MB_ICONERROR);
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
static void setupPath(Log::Logger& moLog) {
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
    ::GetEnvironmentVariableW(L"PATH", path.data(), static_cast<DWORD>(path.size()));
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
static bool isNxmLink(const QString& link) { return link.startsWith("nxm://", Qt::CaseInsensitive); }

// Bootstraping code
// Creates required directories, removes old files, verifies we can start, etc.
static bool bootstrap(fs::path dataPath) {
    // Remove the temporary backup directory in case we're restarting after an update
    fs::path backupDirectory = dataPath / "update_backup";
    if (fs::exists(backupDirectory)) {
        fs::remove_all(backupDirectory);
    }

    // Create required directories.
    try {
        fs::create_directories(dataPath / "Logs");
    } catch (const fs::filesystem_error&) {
        return false;
    }

    return true;
}

static MOBase::IPluginGame* selectGame(QSettings& settings, QDir const& gamePath, MOBase::IPluginGame* game) {
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
static MOBase::IPluginGame* determineCurrentGame(QString const& moPath, QSettings& settings,
                                                 PluginContainer const& plugins) {
    QString gameName = settings.value("gameName", "").toString();
    if (!gameName.isEmpty()) {
        MOBase::IPluginGame* game = plugins.managedGame(gameName);
        if (game == nullptr) {
            MOBase::reportError(QObject::tr("Plugin to handle %1 no longer installed").arg(gameName));
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
        for (MOBase::IPluginGame* const game : plugins.plugins<MOBase::IPluginGame>()) {
            if (game->looksValid(gameDir)) {
                return selectGame(settings, gameDir, game);
            }
        }
    }

    // OK, we are in a new setup or existing info is useless.
    // See if MO has been installed inside a game directory
    for (MOBase::IPluginGame* const game : plugins.plugins<MOBase::IPluginGame>()) {
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
            for (MOBase::IPluginGame* const game : plugins.plugins<MOBase::IPluginGame>()) {
                if (game->looksValid(gameDir)) {
                    return selectGame(settings, gameDir, game);
                }
            }
            // OK, chop off the last directory and try again
        } while (gameDir.cdUp());
    }

    // Then try a selection dialogue.
    if (!gamePath.isEmpty() || !gameName.isEmpty()) {
        MOBase::reportError(QObject::tr("Could not use configuration settings for game \"%1\", path \"%2\".")
                                .arg(gameName)
                                .arg(gamePath));
    }

    SelectionDialog selection(QObject::tr("Please select the game to manage"), nullptr, QSize(32, 32));

    for (MOBase::IPluginGame* game : plugins.plugins<MOBase::IPluginGame>()) {
        if (game->isInstalled()) {
            QString path = game->gameDirectory().absolutePath();
            selection.addChoice(game->gameName(), path, QVariant::fromValue(game), game->gameIcon());
        }
    }

    selection.addChoice(QString("Browse..."), QString(),
                        QVariant::fromValue(static_cast<MOBase::IPluginGame*>(nullptr)));

    while (selection.exec() != QDialog::Rejected) {
        MOBase::IPluginGame* game = selection.getChoiceData().value<MOBase::IPluginGame*>();
        if (game != nullptr) {
            return selectGame(settings, game->gameDirectory(), game);
        }

        gamePath = QFileDialog::getExistingDirectory(nullptr, QObject::tr("Please select the game to manage"),
                                                     QString(), QFileDialog::ShowDirsOnly);

        if (!gamePath.isEmpty()) {
            QDir gameDir(gamePath);
            for (MOBase::IPluginGame* const game : plugins.plugins<MOBase::IPluginGame>()) {
                if (game->looksValid(gameDir)) {
                    return selectGame(settings, gameDir, game);
                }
            }
            MOBase::reportError(QObject::tr("No game identified in \"%1\". The directory is required to contain "
                                            "the game binary and its launcher.")
                                    .arg(gamePath));
        }
    }

    return nullptr;
}

static QString determineProfile(QStringList& arguments, const QSettings& settings) {
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

#pragma region WIP
#include "ui_mainwindow.h"
#include <MO/aboutdialog.h>
#include <QDirIterator>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMenu>
#include <QToolButton>
#include <QWhatsThis>
#include <tuple>
#include <uibase/iplugin.h>
#include <uibase/iplugindiagnose.h>
#include <uibase/ipluginfilemapper.h>
#include <uibase/iplugingame.h>
#include <uibase/iplugininstaller.h>
#include <uibase/ipluginmodpage.h>
#include <uibase/ipluginproxy.h>
#include <uibase/iplugintool.h>
#include <vector>
// Enforce a single instance of MO.
class MySingleInstance {
public:
    MySingleInstance() {
        // Attempt to create mutex.
        m_mutex = CreateMutexA(NULL, FALSE, m_mutexid.data());
        assert(m_mutex);
        m_primary = (GetLastError() != ERROR_ALREADY_EXISTS);
    }
    ~MySingleInstance() { CloseHandle(m_mutex); }

    MySingleInstance(const MySingleInstance&) = delete;
    MySingleInstance(MySingleInstance&&) = delete;
    MySingleInstance& operator=(const MySingleInstance&) = delete;
    MySingleInstance& operator=(MySingleInstance&&) = delete;

public:
    // Return whether this is the primary instance or not.
    bool primary() const { return m_primary; }

private:
    static const std::string m_mutexid;
    bool m_primary = false; // True = primary instance. False = secondary instance.
    HANDLE m_mutex = nullptr;
};
const std::string MySingleInstance::m_mutexid = "ModOrganizer";

// Handles Interprocess Communication using a socket.
// Mainly used for communicating nxm download urls to the primary MO instance.
class MoIPC : public QObject {
    Q_OBJECT
public:
    // Start the server and listen for messages.
    // This should only be called for the primary Mod Organizer instance.
    void listen() {
        connect(&m_server, SIGNAL(newConnection()), this, SLOT(receiveMessage()));
        m_server.setSocketOptions(QLocalServer::WorldAccessOption);
        m_server.listen(QString::fromStdString(m_key));
    }

    // Send a message to the primary instance.
    // This can be used to transmit download urls
    // @param message message to send
    void sendMessage(const QString& message) {
        QLocalSocket socket;
        QString key = QString::fromStdString(m_key);

        // Attempt connection.
        socket.connectToServer(key, QIODevice::WriteOnly);
        bool connected = socket.waitForConnected(); // Wait the default 30 seconds.

        if (!connected) {
            MOBase::reportError(tr("failed to connect to running instance: %1").arg(socket.errorString()));
            return;
        }

        socket.write(message.toUtf8());
        if (!socket.waitForBytesWritten(m_timeout)) {
            MOBase::reportError(tr("failed to communicate with running instance: %1").arg(socket.errorString()));
            return;
        }

        socket.disconnectFromServer();
    }
signals:
    // @brief emitted when a secondary instance has sent a message (to us)
    // Should be connected to a slot.
    // @param message the message we received
    void messageSent(const QString& message);

private slots:
    // Receive a message from a secondary process.
    void receiveMessage() {
        QLocalSocket* socket = m_server.nextPendingConnection();
        assert(socket);
        if (!socket->waitForReadyRead(m_timeout)) {
            MOBase::reportError(tr("failed to receive data from secondary instance: %1").arg(socket->errorString()));
            return;
        }

        QString message = QString::fromUtf8(socket->readAll().constData());

        QMessageBox::information(nullptr, "Mod Organizer", message);

        emit messageSent(message);
        socket->disconnectFromServer();
    }

private:
    static const int m_timeout = 5000;
    static const std::string m_key;

    QLocalServer m_server;
};
const std::string MoIPC::m_key = "mo-43d1a3ad-eeb0-4818-97c9-eda5216c29b5";

// Backend Mod Organizer logic.
class MyPluginContainer : public MOBase::IPluginDiagnose {
public:
    MyPluginContainer() {}

    // Return loaded plugins of interface T.
    template <typename T>
    const std::vector<T*>& plugins() const {
        return std::get<std::vector<T*>>(m_plugins);
    }

public:
    // Load Plugins. Unloads currently loaded plugins.
    void loadPlugins() {
        unloadPlugins();

        // Register statically linked Plugins?
        // Not quite sure what this does. So far no apparant effect.
        for (QObject* plugin : QPluginLoader::staticInstances()) {
            registerPlugin(plugin, "");
        }

        // Check if we failed to load a plugin.
        // If so, inform the user and ask them whether they want to disable it.
        QFile loadCheck(qApp->property("dataPath").toString() + "/plugin_loadcheck.tmp");
        if (loadCheck.exists() && loadCheck.open(QIODevice::ReadOnly)) {
            // Get the name of the last loaded plugin.
            QString fileName;
            while (!loadCheck.atEnd()) {
                fileName = QString::fromUtf8(loadCheck.readLine().constData()).trimmed();
            }
            MOLog::instance().warn("Plugin '{}' failed to load last start.", fileName.toStdString());
            // if (QMessageBox::question(
            //        nullptr, QObject::tr("Plugin error"),
            //        QObject::tr("It appears the plugin \"%1\" failed to load last startup and caused MO "
            //                    "to crash. Do you want to disable it?\n"
            //                    "(Please note: If this is the first time you have seen this message for this "
            //                    "plugin you may want to give it another try. "
            //                    "The plugin may be able to recover from the problem)")
            //            .arg(fileName),
            //        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes) {
            //     m_Organizer->settings().addBlacklistPlugin(fileName);
            //}
            loadCheck.close();
        }
        // Open the loadCheck file for writing.
        loadCheck.open(QIODevice::WriteOnly);

        // Get the plugins location
        fs::path pluginPath = common::get_exe_dir() / AppConfig::pluginPath();
        MOLog::instance().info("Looking for plugins in '{}'", pluginPath);

        // Load plugins from pluginPath
        for (const auto& plugin : fs::directory_iterator(pluginPath)) {
            std::string pluginName = plugin.path().filename().u8string();
            // if (m_Organizer->settings().pluginBlacklisted(pluginName)) {
            //    MOLog::instance().warn("Plugin '{}' was blacklisted", pluginName);
            //    continue;
            //}
            // Write the Plugin filename to loadCheck
            loadCheck.write(QByteArray::fromStdString(pluginName));
            loadCheck.write("\n");
            loadCheck.flush();
            // Validate and load plugin
            QString pluginPath = QString::fromStdWString(plugin.path());
            if (!QLibrary::isLibrary(pluginPath)) {
                continue;
            }
            std::unique_ptr<QPluginLoader> pluginLoader(new QPluginLoader(pluginPath));
            auto instance = pluginLoader->instance();
            if (!instance) {
                m_failedPlugins.push_back(pluginName);
                MOLog::instance().error("Failed to load plugin: '{}'.\nReason: '{}'", pluginName,
                                        pluginLoader->errorString().toStdString());
            }

            // Register plugin
            if (registerPlugin(pluginLoader->instance(), pluginPath)) {
                MOLog::instance().info("Loaded plugin '{}'", pluginName);
                // m_PluginLoaders.push_back(pluginLoader.release());
            } else {
                m_failedPlugins.push_back(pluginName);
                MOLog::instance().warn("Plugin '{}' failed to load. (Possibly outddated)", pluginName);
            }
        }
        // Remove the load check file on success
        loadCheck.remove();
        // Add ourselves
        this->plugins<IPluginDiagnose>().push_back(this);
        // m_Organizer->connectPlugins(this);
    }
    void unloadPlugins() {
        // if (m_UserInterface) {
        //    m_UserInterface->disconnectPlugins();
        //}

        // disconnect all slots before unloading plugins so plugins don't have to take care of that
        // m_Organizer->disconnectPlugins();

        // Clear every vector of loaded plugins.
        common::forTuple([](auto& vec) { vec.clear(); }, m_plugins);

        // for (const boost::signals2::connection& connection : m_DiagnosisConnections) {
        //    connection.disconnect();
        //}
        // m_DiagnosisConnections.clear();

        // while (!m_PluginLoaders.empty()) {
        //    std::unique_ptr<QPluginLoader> loader(m_PluginLoaders.back());
        //    m_PluginLoaders.pop_back();
        //    if (loader && !loader->unload()) {
        //        qDebug("failed to unload %s: %s", qUtf8Printable(loader->fileName()),
        //               qUtf8Printable(loader->errorString()));
        //    }
        //}
    }

private:
    // Verify the dll is a plugin
    bool verifyIPlugin(QObject* plugin, std::string fileName) {
        MOBase::IPlugin* pluginObj = qobject_cast<MOBase::IPlugin*>(plugin);
        if (!pluginObj) {
            MOLog::instance().warn("Plugin '{}' is not an IPlugin", fileName);
            return false;
        }
        return true;
    }

    // Register a Plugin.
    bool registerPlugin(QObject* plugin, const QString& qfileName) {
        std::string fileName = qfileName.toStdString();
        if (!plugin) {
            MOLog::instance().warn("Attempted to register plugin '{}' with a null instance.", fileName);
            return false;
        }
        if (!verifyIPlugin(plugin, fileName)) {
            return false;
        }
        plugin->setProperty("filename", qfileName);
        // m_Organizer->settings().registerPlugin(qobject_cast<MOBase::IPlugin*>(plugin));
        return true;
    }

    // Returned loaded plugins of interface T and allow it to be modified.
    template <typename T>
    std::vector<T*>& plugins() {
        return std::get<std::vector<T*>>(m_plugins);
    }

public: // IPluginDiagnose interface
    virtual std::vector<unsigned int> activeProblems() const override {
        std::vector<unsigned int> problems;
        if (m_failedPlugins.size()) {
            problems.push_back(PROBLEM_PLUGINSNOTLOADED);
        }
        return problems;
    }

    virtual QString shortDescription(unsigned int key) const override {
        assert(key == 1);
        return QObject::tr("Some plugins could not be loaded");
    }

    virtual QString fullDescription(unsigned int key) const override {
        assert(key == 1);
        QString result = QObject::tr("The following plugins could not be loaded. The reason may be missing "
                                     "dependencies (i.e. python) or an outdated version:") +
                         "<ul>";
        for (const std::string& plugin : m_failedPlugins) {
            result.append("<li>" + QString::fromStdString(plugin) + "</li>");
        }
        result.append("<ul>");
        return result;
    }

    virtual bool hasGuidedFix(unsigned int) const override { return false; }

    virtual void startGuidedFix(unsigned int) const override {}

private:
    // Tuple of all supported Plugin interfaces.
    // Non owning pointers.
    using PluginMap = std::tuple<std::vector<MOBase::IPlugin*>, std::vector<MOBase::IPluginDiagnose*>,
                                 std::vector<MOBase::IPluginGame*>, std::vector<MOBase::IPluginInstaller*>,
                                 std::vector<MOBase::IPluginModPage*>, std::vector<MOBase::IPluginPreview*>,
                                 std::vector<MOBase::IPluginTool*>, std::vector<MOBase::IPluginProxy*>,
                                 std::vector<MOBase::IPluginFileMapper*>>;
    PluginMap m_plugins;
    // for IPluginDiagnose
    static const unsigned int PROBLEM_PLUGINSNOTLOADED = 1;
    std::vector<std::string> m_failedPlugins;
};
// Mod Organizer backend.
// Ties together various backend systems into one interface provided to the frontend.
// May also act as a bridge between the different backend systems.
// Loads Plugins
class MyOrganizerCore {
public:
    MyOrganizerCore() {
        // Load Mod Organizer Plugins.
        m_plugins.loadPlugins();
    }

public: // MyPluginContainer
    // Return list of loaded plugins of interface T.
    template <typename T>
    decltype(auto) plugins() const {
        return m_plugins.plugins<T>();
    }

private:
    MyPluginContainer m_plugins;
};
// The Mod Organizer Main Window.
// Hooks backend logic to frontend.
class MyMainWindow : public QMainWindow {
    Q_OBJECT
public:
    MyMainWindow(MyOrganizerCore& o) : QMainWindow(nullptr), ui(new Ui::MainWindow), m_organizer(o) {
        // Setup UI
        ui->setupUi(this);
        // Update the window title to match game, version, and nexus user.
        updateWindowTitle();
        // Hide status bar
        statusBar()->clearMessage();
        statusBar()->hide();
        // Hide MO Endorse button
        ui->actionEndorseMO->setVisible(false);
        // Update the Problems UI and Toolbar
        updateProblemsButton();
        updateToolBar();
        // Setup signals and slots.
        // connect(ui->actionHelp, SIGNAL(triggered(bool)), this, SLOT(helpTriggered()));
    }
    ~MyMainWindow() {}

private:
    void updateWindowTitle(std::string accountName = {}) {
        // m_OrganizerCore.managedGame()->gameName(), m_OrganizerCore.getVersion().displayString()
        std::string title = fmt::format("{} Mod Organizer v{}", "[game]", 2);

        if (accountName.empty()) {
            title.append(fmt::format(" ({})", accountName));
        }

        this->setWindowTitle(QString::fromStdString(title));
    }

    // Check for problems reported by IPluginDiagnose.
    size_t checkForProblems() {
        size_t numProblems = 0;
        for (MOBase::IPluginDiagnose* diagnose : m_organizer.plugins<MOBase::IPluginDiagnose>()) {
            numProblems += diagnose->activeProblems().size();
        }
        return numProblems;
    }

    // Update the Problems button with new problems.
    void updateProblemsButton() {
        size_t numProblems = checkForProblems();
        if (numProblems > 0) {
            ui->actionProblems->setEnabled(true);
            ui->actionProblems->setIconText(tr("Problems"));
            ui->actionProblems->setToolTip(tr("There are potential problems with your setup"));

            QPixmap mergedIcon = QPixmap(":/MO/gui/warning").scaled(64, 64);
            {
                QPainter painter(&mergedIcon);
                std::string badgeName =
                    std::string(":/MO/gui/badge_") +
                    (numProblems < 10 ? std::to_string(static_cast<long long>(numProblems)) : "more");
                painter.drawPixmap(32, 32, 32, 32, QPixmap(badgeName.c_str()));
            }
            ui->actionProblems->setIcon(QIcon(mergedIcon));
        } else {
            ui->actionProblems->setEnabled(false);
            ui->actionProblems->setIconText(tr("No Problems"));
            ui->actionProblems->setToolTip(tr("Everything seems to be in order"));
            ui->actionProblems->setIcon(QIcon(":/MO/gui/warning"));
        }
    }

    // Add a Menu to a QAction.
    void addMenuToAction(QAction* action) {
        QToolButton* toolBtn = qobject_cast<QToolButton*>(ui->toolBar->widgetForAction(action));
        assert(toolBtn);
        toolBtn->setPopupMode(QToolButton::InstantPopup);
        QMenu* menu = new QMenu(action->text(), toolBtn);
        toolBtn->setMenu(menu);
    }

    // Create the Help widget.
    void createHelpWidget() {
        QToolButton* toolBtn = qobject_cast<QToolButton*>(ui->toolBar->widgetForAction(ui->actionHelp));
        QMenu* buttonMenu = toolBtn->menu();
        assert(buttonMenu);

        // Add actions to the Help Menu.
        QAction* helpAction = new QAction(tr("Help on UI"), buttonMenu);
        connect(helpAction, SIGNAL(triggered()), this, SLOT(helpTriggered()));
        helpAction->setShortcut(Qt::Key_F1);
        buttonMenu->addAction(helpAction);

        QAction* wikiAction = new QAction(tr("Documentation Wiki"), buttonMenu);
        connect(wikiAction, SIGNAL(triggered()), this, SLOT(wikiTriggered()));
        buttonMenu->addAction(wikiAction);

        QAction* issueAction = new QAction(tr("Report Issue"), buttonMenu);
        connect(issueAction, SIGNAL(triggered()), this, SLOT(issueTriggered()));
        buttonMenu->addAction(issueAction);

        // Setup the Tutorial action.
        QMenu* tutorialMenu = new QMenu(tr("Tutorials"), buttonMenu);

        using ActionList = std::vector<std::pair<int, QAction*>>;

        ActionList tutorials;

        // QDirIterator dirIter(QApplication::applicationDirPath() + "/tutorials", QStringList("*.js"), QDir::Files);
        // while (dirIter.hasNext()) {
        //    dirIter.next();
        //    QString fileName = dirIter.fileName();

        //    QFile file(dirIter.filePath());
        //    if (!file.open(QIODevice::ReadOnly)) {
        //        qCritical() << "Failed to open " << fileName;
        //        continue;
        //    }
        //    QString firstLine = QString::fromUtf8(file.readLine());
        //    if (firstLine.startsWith("//TL")) {
        //        QStringList params = firstLine.mid(4).trimmed().split('#');
        //        if (params.size() != 2) {
        //            qCritical() << "invalid header line for tutorial " << fileName << " expected 2 parameters";
        //            continue;
        //        }
        //        QAction* tutAction = new QAction(params.at(0), tutorialMenu);
        //        tutAction->setData(fileName);
        //        tutorials.push_back(std::make_pair(params.at(1).toInt(), tutAction));
        //    }
        //}

        std::sort(
            tutorials.begin(), tutorials.end(),
            [](const ActionList::value_type& LHS, const ActionList::value_type& RHS) { return LHS.first < RHS.first; });

        for (auto iter = tutorials.begin(); iter != tutorials.end(); ++iter) {
            connect(iter->second, SIGNAL(triggered()), this, SLOT(tutorialTriggered()));
            tutorialMenu->addAction(iter->second);
        }

        buttonMenu->addMenu(tutorialMenu);
        buttonMenu->addAction(tr("About"), this, SLOT(about()));
        buttonMenu->addAction(tr("About Qt"), qApp, SLOT(aboutQt()));
    }

    // Update the toolbar.
    void updateToolBar() {
        // Remove any already existing custom__ objects.
        for (QAction* action : ui->toolBar->actions()) {
            if (action->objectName().startsWith("custom__")) {
                ui->toolBar->removeAction(action);
            }
        }

        // Create Spacers
        QWidget* spacer = new QWidget(ui->toolBar);
        spacer->setObjectName("custom__spacer");
        spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        // Add dropdown menus to the Tool and Help menu actions.
        addMenuToAction(ui->actionTool);
        addMenuToAction(ui->actionHelp);
        // Setup the Help widget.
        createHelpWidget();

        // Setup configured toolbar icons and Spacers
        for (QAction* action : ui->toolBar->actions()) {
            if (action->isSeparator()) {
                // Insert spacers directly before the seperator.
                ui->toolBar->insertWidget(action, spacer);

                // Setup custom toolbar buttons.
                std::vector<Executable>::iterator begin;
                std::vector<Executable>::iterator end;
                // m_OrganizerCore.executablesList()->getExecutables(begin, end);
                for (auto iter = begin; iter != end; ++iter) {
                    if (iter->isShownOnToolbar()) {
                        // QAction* exeAction =
                        //    new QAction(iconForExecutable(iter->m_BinaryInfo.filePath()), iter->m_Title, ui->toolBar);
                        // exeAction->setObjectName(QString("custom__") + iter->m_Title);
                        // if (!connect(exeAction, SIGNAL(triggered()), this, SLOT(startExeAction()))) {
                        //    qDebug("failed to connect trigger?");
                        //}
                        // ui->toolBar->insertAction(action, exeAction);
                    }
                }
            }
        }
    }

private slots:
    // Show the About MO window.
    void about() {
        // FIXME: Version.
        // m_OrganizerCore.getVersion().displayString()
        AboutDialog dialog({}, this);
        dialog.exec();
    }

    // Show in-application Help.
    void helpTriggered() { QWhatsThis::enterWhatsThisMode(); }

    // Open the Mod Organizer Wiki Page.
    void wikiTriggered() { common::open_url("http://wiki.step-project.com/Guide:Mod_Organizer"); }

    // Open the Mod Organizer Issues Page.
    void issueTriggered() { common::open_url("https://github.com/ModOrganizer/modorganizer/issues"); }

    // When the Tutorial is triggered.
    void tutorialTriggered() {
        QAction* tutorialAction = qobject_cast<QAction*>(sender());
        if (tutorialAction != nullptr) {
            if (QMessageBox::question(
                    this, tr("Start Tutorial?"),
                    tr("You're about to start a tutorial. For technical reasons it's not possible to end "
                       "the tutorial early. Continue?"),
                    QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
                // TutorialManager::instance().activateTutorial("MainWindow", tutorialAction->data().toString());
            }
        }
    }

private:
    Ui::MainWindow* ui;
    MyOrganizerCore& m_organizer;
};
#include "main.moc"
#pragma endregion

// Run the bulk of the application.
// application, a reference to our application.
// instance, An instance
// splashPath, the path to a image file used as a splash screen.
// dataPath, the MO Application data path.
static int runApplication(Log::Logger& moLog, MOApplication& application, SingleInstance& instance, fs::path splashPath,
                          fs::path dataPath, QStringList arguments) {
    // Display splash screen
    QPixmap pixmap(QString::fromStdWString(splashPath.native()));
    QSplashScreen splash(pixmap);
    // Run bootstrap code.
    if (!bootstrap(dataPath)) {
        MOBase::reportError("failed to set up data paths");
        return 1;
    }
    moLog.info("Current Working Directory: '{}'", fs::current_path());
    splash.show();
    // Setup Settings
    fs::path settingsPath = dataPath / AppConfig::iniFileName();
    QSettings settings(QString::fromStdWString(settingsPath.native()), QSettings::IniFormat);
    moLog.info("Initializing Core");
    // WIP BELOW
    MyOrganizerCore organizer;
    MyMainWindow mainWindow(organizer);
    moLog.info("Displaying Main Window");
    mainWindow.show();
    splash.finish(&mainWindow);
    return application.exec();
#if 0
    // Bootstrap OrganizerCore
    OrganizerCore organizer(settings);
    moLog.info("Initialize plugins");
    PluginContainer pluginContainer(&organizer);
    pluginContainer.loadPlugins();
    // Setup MO for game.
    MOBase::IPluginGame* game = determineCurrentGame(application.applicationDirPath(), settings, pluginContainer);
    if (game == nullptr) {
        return 1;
    }
    if (splashPath.string()[0] == ':') {
        // currently using MO splash, see if the plugin contains one
        QString pluginSplash = QString(":/%1/splash").arg(game->gameShortName());
        QImage image(pluginSplash);
        if (!image.isNull()) {
            image.save(QString::fromStdWString(dataPath / "splash.png"));
        } else {
            moLog.info("No Plugin Splash");
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
    moLog.info("Managed game at: {}", game->gameDirectory().absolutePath().toStdString());
    organizer.updateExecutablesList(settings);
    QString selectedProfileName = determineProfile(arguments, settings);
    organizer.setCurrentProfile(selectedProfileName);
    
    NexusInterface::instance()->getAccessManager()->startLoginCheck();
    moLog.info("Initializing tutorials");
    MOBase::TutorialManager::init(
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

        moLog.info("Displaying Main Window");
        mainWindow.show();
        splash.finish(&mainWindow);
        return application.exec();
    }
#endif
    // TMP
    return 1;
}

// Handle command-line arguments.
// These arguments are only valid on a new Mod Organizer Instance.
// Returns true if the program should terminate and false if startup should continue.
static bool handleArguments(Log::Logger& moLog, QStringList arguments) {
    // If no arguments, continue program.
    if (arguments.size() == 1) {
        return false;
    }
    auto arg1 = arguments.at(1);
    // Handle launch argument.
    // First argument should be launch
    // Second should be the working directory
    // third the program to run
    // Fourth and onwards, arguments to the program.
    // TODO: What is this paramater even used for? It just launches a proccess?
    // It does not seem to launch it as if through MO, so whats the point.
    if ((arguments.length() >= 4) && (arg1 == "launch")) {
        assert(0); // To find out if anything uses this.
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
        //
        return true;
    }
    // Handle nxm links for Primary Instance.
    if (isNxmLink(arg1)) {
        moLog.info("Starting download from command line: {}", arg1.toStdString());
        // Start a secondary MO instance.
        // The Primary Instance should be listening and dealing with messages within the timeout.
        QProcess proc;
        proc.setProgram(QCoreApplication::applicationFilePath());
        proc.setArguments({arg1});
        proc.start();
        proc.waitForFinished(-1);
        return false; // Continue startup.
    } else {
        // TODO: Find out what this one is used for.
        // Probably desktop/startmenu shortcuts?
        QString exeName = arg1;
        moLog.info("Starting {} from command line", arg1.toStdString());
        arguments.removeFirst(); // remove application name (ModOrganizer.exe)
        arguments.removeFirst(); // remove binary name
                                 // pass the remaining parameters to the binary
        try {
            // organizer.startApplication(exeName, arguments, QString(), QString());
        } catch (const std::exception& e) {
            MOBase::reportError(QObject::tr("failed to start application: %1").arg(e.what()));
        }
        return true; //
    }
    return false;
}

// Handle Commandline arguments used for IPC.
static void handleIpcArgs(QStringList arguments, MoIPC& instance) {
    if ((arguments.size() == 2) && isNxmLink(arguments.at(1))) {
        instance.sendMessage(arguments.at(1));
    } else if (arguments.size() == 1) {
        QMessageBox::information(nullptr, QObject::tr("Mod Organizer"),
                                 QObject::tr("An instance of Mod Organizer is already running"));
    }
}

int main(int argc, char* argv[]) {
    Log::Logger* pmoLog = nullptr;
    // This try...catch allows proper cleanup before rethrowing the
    // exception to let MyUnhandledExceptionFilter handle it.
    // This allows, for example, the log to be flushed flushed before the crash.
    try {
        // Create QApplication
        MOApplication application(argc, argv);
        QStringList arguments = application.arguments();
        // Setup Interprocess Communication
        MoIPC ipc;
        // Enforce a single instance of MO.
        MySingleInstance inst;
        if (!inst.primary()) {
            // If not primary instance, assume we have command line arguments
            // and handle those, and then exit.
            handleIpcArgs(arguments, ipc);
            return 1;
        }
        // Listen for ipc messages
        ipc.listen();
        // Setup startup log.
        Log::Logger moLog("mo_init", common::get_exe_dir() / "Logs");
        pmoLog = &moLog;
        // Exception and error handling.
        // Overwide the default windows crash behaviour.
        SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
        // Handle internal Qt logging.
        qInstallMessageHandler(&myMessageOutput);
        // Log useful information.
        moLog.info("Mod Organizer started.");
        moLog.info("MO Located At: {}", common::get_exe_dir());
        // Handle arguments.
        if (handleArguments(moLog, arguments)) {
            return 0;
        }
        // Setup Paths.
        setupPath(moLog);
        // Start the application
#if 0
        application.exec();
        // ...
#else

#if !defined(QT_NO_SSL)
        moLog.info("Qt supports SSL: {}", QSslSocket::supportsSsl());
#else
        moLog.info("Qt does not support SSL.");
#endif
        moLog.info("Enforcing Single Instance");
        // Enforce a single Instance of MO.
        // Handle NXM Downloads
        // FIXME: Won't logging up until this point conflict, since they're writing to the same file.
        // Solution could be to enforce this earlier?
        SingleInstance instance(false);

        moLog.info("Primary Instance");
        // Find Mod Organizer data Directory.
        // In previous versions of MO this was the same place as the executable, but
        // Now it can be any location the User desires.
        // This handles the user choosing whether to use portable mode or a custom location or what.
        moLog.info("Getting MO Data Path");
        // Setup Instance
        fs::path dataPath;
        try {
            dataPath = InstanceManager::instance().determineDataPath();
        } catch (const std::exception& e) {
            QMessageBox::critical(nullptr, QObject::tr("Failed to set up instance"), e.what());
            return 1;
        }
        application.setProperty("dataPath", QString::fromStdWString(dataPath.native()));
        moLog.info("MO Data Path: '{}'", dataPath);
        // Setup logging.
        moLog.info("Initalizing Application Log.");
        const fs::path logPath = dataPath / "Logs" / "mo_interface.log";
        MOLog::init(logPath);
        // Display Splash Screen
        fs::path splash = dataPath / "splash.png";
        // If a splash image doesnt exist, use the MO Provided one as part of Qt Resources.
        if (!fs::exists(splash)) {
            splash = ":/MO/gui/splash";
        }
        // TESTING
        moLog.info("Start Main Application.");
        int result = runApplication(moLog, application, instance, splash, dataPath, arguments);
        // if (result != INT_MAX) {
        //    return result;
        //}
#endif
    } catch (const std::exception& e) {
        auto msg = e.what();
        if (pmoLog) {
            pmoLog->error("Mod Organizer crashed...");
            pmoLog->error(msg);
            pmoLog->flush();
        }

        MOBase::reportError(msg);
        throw;
    } catch (...) {
        if (pmoLog) {
            pmoLog->error("Mod Organizer crashed...");
            pmoLog->flush();
        }
        throw;
    }
}
