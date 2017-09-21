/*
Userspace Virtual Filesystem

Copyright (C) 2015 Sebastian Herbord. All rights reserved.

This file is part of usvfs.

usvfs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

usvfs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with usvfs. If not, see <http://www.gnu.org/licenses/>.
*/
#include "usvfs_shared/shmlogger.h"

#include <boost/interprocess/ipc/message_queue.hpp>

#include <algorithm>
#include <iterator>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

using namespace boost::interprocess;

// TODO: Move into common/stringutils
template <typename Out>
void split(const std::string& s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

SHMLogger* SHMLogger::s_Instance = nullptr;

SHMLogger::owner_t SHMLogger::owner;
SHMLogger::client_t SHMLogger::client;

SHMLogger::SHMLogger(owner_t, const std::string& queueName)
    : m_QueueName(queueName), m_LogQueue(create_only, queueName.c_str(), MESSAGE_COUNT, MESSAGE_SIZE),
      m_DroppedMessages(0) {
    if (s_Instance != nullptr) {
        throw std::runtime_error("duplicate shm logger instantiation");
    } else {
        s_Instance = this;
    }
}

SHMLogger::SHMLogger(client_t, const std::string& queueName)
    : m_QueueName(queueName), m_LogQueue(open_only, queueName.c_str()), m_DroppedMessages(0) {
    if (s_Instance != nullptr) {
        throw std::runtime_error("duplicate shm logger instantiation");
    } else {
        s_Instance = this;
    }
}

SHMLogger::~SHMLogger() {
    s_Instance = nullptr;
    message_queue_interop::remove(m_QueueName.c_str());
}

SHMLogger& SHMLogger::create(const char* instanceName) {
    if (s_Instance != nullptr) {
        throw std::runtime_error("duplicate shm logger instantiation");
    } else {
        std::string queueName = std::string("__shm_sink_") + instanceName;
        // message_queue::remove(queueName.c_str());
        new SHMLogger(owner, queueName);
        atexit([]() { delete s_Instance; });
    }
    return *s_Instance;
}

SHMLogger& SHMLogger::open(const char* instanceName) {
    if (s_Instance != nullptr) {
        throw std::runtime_error("duplicate shm logger instantiation");
    } else {
        new SHMLogger(client, std::string("__shm_sink_") + instanceName);
    }
    return *s_Instance;
}

void SHMLogger::free() {
    if (s_Instance != nullptr) {
        SHMLogger* temp = s_Instance;
        s_Instance = nullptr;
        delete temp;
    }
}

bool SHMLogger::tryGet(char* buffer, size_t bufferSize) {
    message_queue_interop::size_type receivedSize;
    unsigned int prio;
    bool res = m_LogQueue.try_receive(buffer, static_cast<unsigned int>(bufferSize), receivedSize, prio);
    if (res) {
        buffer[std::min(bufferSize - 1, static_cast<size_t>(receivedSize))] = '\0';
    }
    return res;
}

void SHMLogger::get(char* buffer, size_t bufferSize) {
    message_queue_interop::size_type receivedSize;
    unsigned int prio;
    m_LogQueue.receive(buffer, static_cast<unsigned int>(bufferSize), receivedSize, prio);
    buffer[std::min(bufferSize - 1, static_cast<size_t>(receivedSize))] = '\0';
}

spdlog::sinks::shm_sink::shm_sink(const char* queueName)
    : m_LogQueue(open_only, (std::string("__shm_sink_") + queueName).c_str()) {}

void spdlog::sinks::shm_sink::flush() {}

void spdlog::sinks::shm_sink::log(const details::log_msg& msg) {
    int droppedMessages = m_DroppedMessages.load(std::memory_order_relaxed);
    if (droppedMessages > 0) {
        std::string dropMessage = fmt::format("{} debug messages dropped", droppedMessages);
        // if (m_LogQueue.try_send(dropMessage.c_str(), static_cast<unsigned int>(dropMessage.size()), 0)) {
        //    m_DroppedMessages.store(0, std::memory_order_relaxed);
        //}
        // FIXME: Not implement
    }

    std::string message = msg.formatted.str();

    if (message.length() > SHMLogger::MESSAGE_SIZE) {
        // FIXME: Verify for correctness. \n or \r\n? Was 2nd orginially. -D
        std::vector<std::string> splitVec = split(message, '\n');
        for (const std::string& line : splitVec) {
            output(msg.level, line);
        }
    } else {
        output(msg.level, message);
    }
}

void spdlog::sinks::shm_sink::output(level::level_enum lev, const std::string& message) {
    bool sent = true;

    // spdlog auto-append line breaks which we don't need. Probably would be
    // better to not write the breaks to begin with?
    size_t count = std::min(message.find_last_not_of("\r\n") + 1, 1024ULL);

    // depending on the log level, drop less important messages if the receiver
    // can't keep up
    switch (lev) {
    case level::trace:
    case level::debug:
    case level::info: {
        sent = m_LogQueue.try_send(message.c_str(), static_cast<unsigned int>(count), 0);
    } break;
    case level::warn:
    case level::critical:
    case level::err: {
        m_LogQueue.send(message.c_str(), static_cast<unsigned int>(count), 0);
    } break;
    default: {
        boost::posix_time::ptime time = microsec_clock::universal_time() + boost::posix_time::milliseconds(200);
        sent = m_LogQueue.timed_send(message.c_str(), static_cast<unsigned int>(count), 0, time);
    } break;
    }

    if (!sent) {
        m_DroppedMessages.fetch_add(1, std::memory_order_relaxed);
    }
}
