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
// Spdlog optimizations
#define SPDLOG_NO_THREAD_ID      // We don't use thread id.
#define SPDLOG_NO_REGISTRY_MUTEX // We don't use the registry.
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

#include <common/predef.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

#include <QAbstractItemModel>
#include <QMutex>
#include <QTime>

#include <filesystem>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
namespace fs = std::experimental::filesystem;

namespace Log {
using namespace std::string_literals;
// Logging specific details
namespace details {
// Global log
static std::stringstream errorLog;
// Implementation for setting up logging file sink.
// log_path is a required argument taking the path to the full path to the log file.
spdlog::sink_ptr file_sink(fs::path log_path);
spdlog::sink_ptr console_sink();
spdlog::sink_ptr ostream_sink();
} // namespace details

enum class Level {
    NOTSET,
    DEBUG,
    INFO,
    WARNING,
    ERR,
    FATAL,
};

// An abstraction that handles logging.
// This class is thread safe.
// If logging fails, an error message is printed to stderr.
// This class supports formatting provided by fmtlib.
// All logging methods are format aware.
class Logger {
public:
    // Create a new log file with the name `name` at path `log_path`
    // Log files must be unique, or else strange things may happen.
    // This is because spdlog requires logs to the same file to use the same sink, and a new sink is created each time
    // this is called.
    Logger(std::string filename, fs::path log_path, Log::Level = Log::Level::DEBUG);
    // Full path to a log file.
    Logger(fs::path log_file_path, Log::Level = Log::Level::DEBUG);

public:
    fs::path get_log_dir() { return m_logPath; }
    fs::path get_log_path() { return m_logPath / m_name += ".log"s; }
    void flush() { m_logger->flush(); }
    // Set or Get the Log Level
    Log::Level log_level() const { return m_level; }
    void log_level(Log::Level l);
    // Log based on a passed in Log Level.
    template <typename... Args>
    void log(Log::Level lev, Args&&... args) {
        switch (lev) {
        case Log::Level::DEBUG:
            debug(std::forward<Args>(args)...);
            break;
        case Log::Level::INFO:
            info(std::forward<Args>(args)...);
            break;
        case Log::Level::WARNING:
            warn(std::forward<Args>(args)...);
            break;
        case Log::Level::ERR:
            error(std::forward<Args>(args)...);
            break;
        case Log::Level::FATAL:
            fatal(std::forward<Args>(args)...);
            break;
        default:
            warn("Unreconginzed Log Level: '{}'", l);
            break;
        }
    }
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
    Log::Level m_level;

protected:
    template <typename... Args>
    std::string format(Args&&... args) const {
        // spdlog passes all args to this internally
        return fmt::format(std::forward<Args>(args)...);
    }
};

} // namespace Log

// Singleton class that handles displaying logs to Qt
// Intended to be used with a QAbstractItemView.
class LogBuffer : public QAbstractItemModel {
    Q_OBJECT
public:
    // Initalize a new LogBuffer instance.
    // messageCount is how many messages will fit in the Buffer.
    // minMsgType is the minimum log level for the message to be displayed.
    // logFile is the full path to the log file that will be used.
    // Installs a Qt Message Handler, logging all messages through the Log function.
    // Should not be called from multipul threads.
    static void init(int messageCount, Log::Level minMsgType, fs::path logFile);
    // Qt Message Handler. All Log messages from Qt go through here.
    static void log(QtMsgType type, const QMessageLogContext& context, const QString& message);

    static void writeNow();
    static void cleanQuit();

    static LogBuffer* instance() { return s_Instance.get(); }

public:
    virtual ~LogBuffer();

    // Actually Log a message.
    void logMessage(Log::Level type, const std::string& message);

    // QAbstractItemModel interface
public:
    QModelIndex index(int row, int column, const QModelIndex& parent) const;
    QModelIndex parent(const QModelIndex& child) const;
    int rowCount(const QModelIndex& parent) const;
    int columnCount(const QModelIndex& parent) const;
    QVariant data(const QModelIndex& index, int role) const;
signals:
public slots:

private:
    explicit LogBuffer(int messageCount, Log::Level minMsgType, fs::path logFile);
    LogBuffer(const LogBuffer& reference);            // not implemented
    LogBuffer& operator=(const LogBuffer& reference); // not implemented

    void write() const;

    static char msgTypeID(QtMsgType type);

private:
    struct Message {
        Log::Level type;
        QTime time;
        std::string message;
        QString toString() const;
    };

private:
    static std::unique_ptr<LogBuffer> s_Instance;
    static QMutex s_Mutex;
    static QtMessageHandler old_handler;

    fs::path m_logFile;
    bool m_ShutDown = false;
    Log::Level m_MinMsgType;
    size_t m_NumMessages = 0;
    std::vector<Message> m_Messages;
    std::unique_ptr<Log::Logger> m_log;
};
