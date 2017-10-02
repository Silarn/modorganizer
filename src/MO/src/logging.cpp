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

std::unique_ptr<MOLog> MOLog::m_instance;

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

//// Log::Logger \\\\

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
