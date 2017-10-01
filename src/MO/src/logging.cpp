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
#include "MO/logging.h"

#include <QFile>
#include <QIcon>
#include <common/sane_windows.h>
#include <uibase/report.h>
#include <uibase/scopeguard.h>

#include <iostream>

using MOBase::reportError;

std::unique_ptr<LogBuffer> LogBuffer::s_Instance;
QMutex LogBuffer::s_Mutex;
QtMessageHandler LogBuffer::old_handler;

////
// Convert Qt log levels to Log::Level
Log::Level QtToLog(QtMsgType type) {
    switch (type) {
    case QtDebugMsg:
        return Log::Level::DEBUG;
    case QtInfoMsg:
        return Log::Level::INFO;
    case QtWarningMsg:
        return Log::Level::WARNING;
    case QtCriticalMsg:
        return Log::Level::ERR;
    case QtFatalMsg:
        return Log::Level::FATAL;
    default:
        return Log::Level::NOTSET;
    }
}
////

void LogBuffer::init(int messageCount, Log::Level minMsgType, fs::path outputFileName) {
    s_Instance.reset(new LogBuffer(messageCount, minMsgType, outputFileName));
    qInstallMessageHandler(&LogBuffer::log);
}

void LogBuffer::log(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    assert(s_Instance);
    // Qt Logs seem to have an extra newline at the end.
    std::string msg = message.toStdString();
    msg.resize(msg.size() - 1);
    s_Instance->logMessage(QtToLog(type), msg);
}

LogBuffer::LogBuffer(int messageCount, Log::Level minMsgType, fs::path logFile)
    : QAbstractItemModel(nullptr), m_logFile(logFile), m_MinMsgType(minMsgType) {
    m_log.reset(new Log::Logger(m_logFile));
    m_log;
    m_Messages.resize(messageCount);
}

LogBuffer::~LogBuffer() {
    qInstallMessageHandler(0);
    write();
}

void LogBuffer::logMessage(Log::Level type, const std::string& message) {
    m_log->log(type, message);
    if (type >= m_MinMsgType) {
        Message msg = {type, QTime::currentTime(), message};
        if (m_NumMessages < m_Messages.size()) {
            beginInsertRows(QModelIndex(), static_cast<int>(m_NumMessages), static_cast<int>(m_NumMessages) + 1);
        }
        m_Messages.at(m_NumMessages % m_Messages.size()) = msg;
        if (m_NumMessages < m_Messages.size()) {
            endInsertRows();
        } else {
            emit dataChanged(createIndex(0, 0), createIndex(static_cast<int>(m_Messages.size()), 0));
        }
        ++m_NumMessages;
        if (type >= Log::Level::ERR) {
            write();
        }
    }
}

void LogBuffer::write() const {
    if (m_NumMessages == 0) {
        return;
    }

    DWORD lastError = ::GetLastError();

    QFile file(m_OutFileName);
    if (!file.open(QIODevice::WriteOnly)) {
        reportError(tr("failed to write log to %1: %2").arg(m_OutFileName).arg(file.errorString()));
        return;
    }

    unsigned int i =
        (m_NumMessages > m_Messages.size()) ? static_cast<unsigned int>(m_NumMessages - m_Messages.size()) : 0U;
    for (; i < m_NumMessages; ++i) {
        file.write(m_Messages.at(i % m_Messages.size()).toString().toUtf8());
        file.write("\r\n");
    }
    ::SetLastError(lastError);
}

char LogBuffer::msgTypeID(QtMsgType type) {
    switch (type) {
    case QtDebugMsg:
        return 'D';
    case QtWarningMsg:
        return 'W';
    case QtCriticalMsg:
        return 'C';
    case QtFatalMsg:
        return 'F';
    default:
        return '?';
    }
}

QModelIndex LogBuffer::index(int row, int column, const QModelIndex&) const { return createIndex(row, column, row); }

QModelIndex LogBuffer::parent(const QModelIndex&) const { return QModelIndex(); }

int LogBuffer::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    } else {
        return static_cast<int>(std::min(m_NumMessages, m_Messages.size()));
    }
}

int LogBuffer::columnCount(const QModelIndex&) const { return 2; }

QVariant LogBuffer::data(const QModelIndex& index, int role) const {
    unsigned offset =
        m_NumMessages < m_Messages.size() ? 0 : static_cast<unsigned int>(m_NumMessages - m_Messages.size());
    unsigned int msgIndex = (offset + index.row() + 1) % m_Messages.size();
    switch (role) {
    case Qt::DisplayRole: {
        if (index.column() == 0) {
            return m_Messages[msgIndex].time;
        } else if (index.column() == 1) {
            const QString& msg = m_Messages[msgIndex].message;
            if (msg.length() < 200) {
                return msg;
            } else {
                return msg.mid(0, 200) + "...";
            }
        }
    } break;
    case Qt::DecorationRole: {
        if (index.column() == 1) {
            switch (m_Messages[msgIndex].type) {
            case QtDebugMsg:
                return QIcon(":/MO/gui/information");
            case QtWarningMsg:
                return QIcon(":/MO/gui/warning");
            case QtCriticalMsg:
                return QIcon(":/MO/gui/important");
            case QtFatalMsg:
                return QIcon(":/MO/gui/problem");
            }
        }
    } break;
    case Qt::UserRole: {
        if (index.column() == 1) {
            switch (m_Messages[msgIndex].type) {
            case QtDebugMsg:
                return "D";
            case QtWarningMsg:
                return "W";
            case QtCriticalMsg:
                return "C";
            case QtFatalMsg:
                return "F";
            }
        }
    } break;
    }
    return QVariant();
}

void LogBuffer::writeNow() {
    QMutexLocker guard(&s_Mutex);
    if (!s_Instance.isNull()) {
        s_Instance->write();
    }
}

void LogBuffer::cleanQuit() {
    QMutexLocker guard(&s_Mutex);
    if (!s_Instance.isNull()) {
        s_Instance->m_ShutDown = true;
    }
}

void log(const char* format, ...) {
    // FIXME: C style va_args? In C++? The horror.
    // FIXME: Removing this causes compilation to fail.
    // Somehow it's being used in MOShared.lib(directoryentry.obj).
    // error LNK2019: unresolved external symbol "void __cdecl log(char const *,...)" (?log@@YAXPEBDZZ) referenced in
    // function "public: bool __cdecl MOShared::FileRegister::removeFile(unsigned int)"
    // (?removeFile@FileRegister@MOShared@@QEAA_NI@Z)
    va_list argList;
    va_start(argList, format);

    static const int BUFFERSIZE = 1000;

    char buffer[BUFFERSIZE + 1];
    buffer[BUFFERSIZE] = '\0';

    vsnprintf(buffer, BUFFERSIZE, format, argList);

    qCritical("%s", buffer);

    va_end(argList);
}

QString LogBuffer::Message::toString() const {
    return QString("%1 [%2] %3").arg(time.toString()).arg(msgTypeID(type)).arg(message);
}

////

////
// Convert Log::Level log levels to spdlog::level
static spdlog::level::level_enum LogToSpd(Log::Level type) {
    switch (type) {
    case Log::Level::DEBUG:
        return spdlog::level::debug;
    case Log::Level::INFO:
        return spdlog::level::info;
    case Log::Level::WARNING:
        return spdlog::level::warn;
    case Log::Level::ERR:
        return spdlog::level::err;
    case Log::Level::FATAL:
        return spdlog::level::critical;
    default:
        return spdlog::level::debug;
    }
}
////

spdlog::sink_ptr Log::details::file_sink(fs::path log_path) {
    fs::path full_path(log_path);
    return std::make_shared<spdlog::sinks::simple_file_sink_mt>(full_path.string());
}

spdlog::sink_ptr Log::details::console_sink() {
    static auto tmp = std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>();
    return tmp;
}

spdlog::sink_ptr Log::details::ostream_sink() {
    static auto tmp = std::make_shared<spdlog::sinks::ostream_sink_mt>(errorLog);
    return tmp;
}

Log::Logger::Logger(std::string filename, fs::path log_path, Log::Level l)
    : m_name(filename), m_logPath(log_path), m_level(l) {
    // Make the path canoical and immune to working directory changes.
    log_path = fs::canonical(log_path);
    // First create the log directory.
    fs::create_directories(log_path);
    // Setup spdlog sinks.
    std::vector<spdlog::sink_ptr> sinks;
    // Add file sink.
    sinks.push_back(details::file_sink(log_path / (filename + ".log")));
    // Add global log sink.
    sinks.push_back(details::ostream_sink());
    // If debug configuration, log to the console as well.
#if COMMON_IS_DEBUG
    sinks.push_back(details::console_sink());
#endif
    // Create the Logger.
    m_logger = std::make_unique<spdlog::logger>(m_name, std::begin(sinks), std::end(sinks));
    // Change level to supplied.
    m_logger->set_level(LogToSpd(m_level));
    // If logging fails, print to stderr.
    m_logger->set_error_handler([](const std::string& msg) { std::cerr << "Failed to log: " << msg << '\n'; });
}

Log::Logger::Logger(fs::path log_file_path, Log::Level l)
    : Logger(log_file_path.stem().string(), log_file_path.parent_path(), l) {}

void Log::Logger::log_level(Log::Level l) {
    m_level = l;
    m_logger->set_level(LogToSpd(l));
}
