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
#include "MO/singleinstance.h"

#include <QLocalSocket>
#include <uibase/report.h>
#include <uibase/utility.h>

static const char s_Key[] = "mo-43d1a3ad-eeb0-4818-97c9-eda5216c29b5";
static const int s_Timeout = 5000;

using MOBase::reportError;

SingleInstance::SingleInstance(bool forcePrimary) : QObject(nullptr) {
    // Set Shared Memory key to s_Key.
    m_SharedMem.setKey(s_Key);
    // Create shared memory.
    // If failure, check if forcePrimary is true and do it anyway.
    // If forcePrimary is false, attempt to attatch to the already existing memory.
    if (!m_SharedMem.create(1)) {
        if (forcePrimary) {
            while (m_SharedMem.error() == QSharedMemory::AlreadyExists) {
                Sleep(500);
                if (m_SharedMem.create(1)) {
                    m_PrimaryInstance = true;
                    break;
                }
            }
        }

        if (m_SharedMem.error() == QSharedMemory::AlreadyExists) {
            m_SharedMem.attach();
            m_PrimaryInstance = false;
        }
        if ((m_SharedMem.error() != QSharedMemory::NoError) && (m_SharedMem.error() != QSharedMemory::AlreadyExists)) {
            throw MOBase::MyException(tr("SHM error: %1").arg(m_SharedMem.errorString()));
        }
    } else {
        m_PrimaryInstance = true;
    }
    // Listen for messages if we're the primary instance.
    if (m_PrimaryInstance) {
        connect(&m_Server, SIGNAL(newConnection()), this, SLOT(receiveMessage()));
        m_Server.setSocketOptions(QLocalServer::WorldAccessOption);
        // Listen on s_Key.
        m_Server.listen(s_Key);
    }
}

void SingleInstance::sendMessage(const QString& message) {
    if (m_PrimaryInstance) {
        // nobody there to receive the message
        return;
    }
    QLocalSocket socket(this);

    // Attempt connection with two retries.
    bool connected = false;
    for (int i = 0; i < 2 && !connected; ++i) {
        if (i > 0) {
            Sleep(250);
        }

        // other instance may be just starting up
        socket.connectToServer(s_Key, QIODevice::WriteOnly);
        connected = socket.waitForConnected(s_Timeout);
    }

    if (!connected) {
        reportError(tr("failed to connect to running instance: %1").arg(socket.errorString()));
        return;
    }

    socket.write(message.toUtf8());
    if (!socket.waitForBytesWritten(s_Timeout)) {
        reportError(tr("failed to communicate with running instance: %1").arg(socket.errorString()));
        return;
    }

    socket.disconnectFromServer();
}

void SingleInstance::receiveMessage() {
    QLocalSocket* socket = m_Server.nextPendingConnection();
    if (!socket->waitForReadyRead(s_Timeout)) {
        reportError(tr("failed to receive data from secondary instance: %1").arg(socket->errorString()));
        return;
    }

    QString message = QString::fromUtf8(socket->readAll().constData());
    emit messageSent(message);
    socket->disconnectFromServer();
}
