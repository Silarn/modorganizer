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
#include "MO/selfupdater.h"
#include "MO/bbcode.h"
#include "MO/installationmanager.h"
#include "MO/logging.h"
#include "MO/nexusinterface.h"
#include "MO/nxmaccessmanager.h"
#include "MO/settings.h"

#include <MO/shared/appconfig.h>
#include <MO/shared/util.h>
#include <archive/archive.h>
#include <common/util.h>
#include <uibase/report.h>
#include <uibase/utility.h>
#include <uibase/versioninfo.h>

#include <QAbstractButton>
#include <QApplication>
#include <QDir>
#include <QIODevice>
#include <QJsonObject>
#include <QJsonValue>
#include <QLibrary>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QObject>
#include <QProgressDialog>
#include <QString>
#include <QUrl>
#include <QtGlobal>

#include <assert.h>
#include <exception>
#include <shellapi.h>

using CreateArchiveType = Archive* (*)();

// Helper function to resolve functions from a DLL.
template <typename T>
static T resolveFunction(QLibrary& lib, const char* name) {
    T temp = reinterpret_cast<T>(lib.resolve(name));
    if (!temp) {
        throw std::runtime_error(QObject::tr("invalid 7-zip32.dll: %1").arg(lib.errorString()).toLatin1().constData());
    }
    return temp;
}

SelfUpdater::SelfUpdater() : m_Interface(NexusInterface::instance()) {
    auto path = common::get_exe_dir() / AppConfig::archiveDll();
    QLibrary archiveLib(QString::fromStdWString(path.native()));
    if (!archiveLib.load()) {
        throw MOBase::MyException(tr("archive.dll not loaded: \"%1\"").arg(archiveLib.errorString()));
    }

    CreateArchiveType CreateArchiveFunc = resolveFunction<CreateArchiveType>(archiveLib, "CreateArchive");

    m_ArchiveHandler.reset(CreateArchiveFunc());
    if (!m_ArchiveHandler->isValid()) {
        // FIXME: This stops startup.
        // Should probably just fix Archive.dll
        // throw MOBase::MyException(InstallationManager::getErrorString(m_ArchiveHandler->getLastError()));
    }
    // TODO: Expand on this.
    VS_FIXEDFILEINFO version = MOShared::GetFileVersion(common::get_exe_dir() / AppConfig::applicationExeName());

    m_MOVersion = MOBase::VersionInfo(version.dwFileVersionMS >> 16, version.dwFileVersionMS & 0xFFFF,
                                      version.dwFileVersionLS >> 16, version.dwFileVersionLS & 0xFFFF);
}

SelfUpdater::~SelfUpdater() {}

void SelfUpdater::setUserInterface(QWidget* widget) { m_Parent = widget; }

void SelfUpdater::testForUpdate() {
    // TODO: if prereleases are disabled we could just request the latest release
    // directly
    // TODO: AppConfig
    m_GitHub.releases(GitHub::Repository("ModOrganizer", "modorganizer"), [this](const QJsonArray& releases) {
        QJsonObject newest;
        for (const QJsonValue& releaseVal : releases) {
            QJsonObject release = releaseVal.toObject();
            if (!release["draft"].toBool() &&
                (Settings::instance().usePrereleases() || !release["prerelease"].toBool())) {
                if (newest.empty() || (MOBase::VersionInfo(release["tag_name"].toString()) >
                                       MOBase::VersionInfo(newest["tag_name"].toString()))) {
                    newest = release;
                }
            }
        }

        if (!newest.empty()) {
            MOBase::VersionInfo newestVer(newest["tag_name"].toString());
            if (newestVer > this->m_MOVersion) {
                m_UpdateCandidate = newest;
                MOLog::instance().info("Update Available: {} -> {}", this->m_MOVersion.displayString().toStdString(),
                                       newestVer.displayString().toStdString());
                emit updateAvailable();
            } else if (newestVer < this->m_MOVersion) {
                // INFO: this could happen if the user switches from using prereleases to
                // stable builds. Should we downgrade?
                MOLog::instance().info("this version is newer than the newest installed one: {} -> {}",
                                       this->m_MOVersion.displayString().toStdString(),
                                       newestVer.displayString().toStdString());
            }
        }
    });
}

void SelfUpdater::startUpdate() {
    // the button can't be pressed if there isn't an update candidate
    assert(!m_UpdateCandidate.empty());

    QMessageBox query(
        QMessageBox::Question, tr("New update available (%1)").arg(m_UpdateCandidate["tag_name"].toString()),
        BBCode::convertToHTML(m_UpdateCandidate["body"].toString()), QMessageBox::Yes | QMessageBox::Cancel, m_Parent);

    query.button(QMessageBox::Yes)->setText(QObject::tr("Install"));

    query.exec();

    if (query.result() == QMessageBox::Yes) {
        bool found = false;
        for (const QJsonValue& assetVal : m_UpdateCandidate["assets"].toArray()) {
            QJsonObject asset = assetVal.toObject();
            if (asset["content_type"].toString() == "application/x-msdownload") {
                openOutputFile(asset["name"].toString());
                download(asset["browser_download_url"].toString());
                found = true;
                break;
            }
        }
        if (!found) {
            QMessageBox::warning(m_Parent, tr("Download failed"),
                                 tr("Failed to find correct download, please try again later."));
        }
    }
}

void SelfUpdater::showProgress() {
    if (m_Progress == nullptr) {
        m_Progress = new QProgressDialog(m_Parent, Qt::Dialog);
        connect(m_Progress, SIGNAL(canceled()), this, SLOT(downloadCancel()));
    }
    m_Progress->setModal(true);
    m_Progress->show();
    m_Progress->setValue(0);
    m_Progress->setWindowTitle(tr("Update"));
    m_Progress->setLabelText(tr("Download in progress"));
}

void SelfUpdater::closeProgress() {
    if (m_Progress != nullptr) {
        m_Progress->hide();
        m_Progress->deleteLater();
        m_Progress = nullptr;
    }
}

void SelfUpdater::openOutputFile(const QString& fileName) {
    QString outputPath = QDir::fromNativeSeparators(qApp->property("dataPath").toString()) + "/" + fileName;
    MOLog::instance().info("Downloading to {}", outputPath.toStdString());
    m_UpdateFile.setFileName(outputPath);
    m_UpdateFile.open(QIODevice::WriteOnly);
}

void SelfUpdater::download(const QString& downloadLink) {
    QNetworkAccessManager* accessManager = m_Interface->getAccessManager();
    QUrl dlUrl(downloadLink);
    QNetworkRequest request(dlUrl);
    m_Canceled = false;
    m_Reply = accessManager->get(request);
    showProgress();

    connect(m_Reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProgress(qint64, qint64)));
    connect(m_Reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
    connect(m_Reply, SIGNAL(readyRead()), this, SLOT(downloadReadyRead()));
}

void SelfUpdater::downloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (m_Reply != nullptr) {
        if (m_Canceled) {
            m_Reply->abort();
        } else {
            if (bytesTotal != 0) {
                if (m_Progress != nullptr) {
                    m_Progress->setValue((bytesReceived * 100) / bytesTotal);
                }
            }
        }
    }
}

void SelfUpdater::downloadReadyRead() {
    if (m_Reply != nullptr) {
        m_UpdateFile.write(m_Reply->readAll());
    }
}

void SelfUpdater::downloadFinished() {
    int error = QNetworkReply::NoError;

    if (m_Reply != nullptr) {
        if (m_Reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 302) {
            QUrl url = m_Reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            m_UpdateFile.reset();
            download(url.toString());
            return;
        }
        m_UpdateFile.write(m_Reply->readAll());

        error = m_Reply->error();

        if (m_Reply->header(QNetworkRequest::ContentTypeHeader).toString().startsWith("text", Qt::CaseInsensitive)) {
            m_Canceled = true;
        }

        closeProgress();

        m_Reply->close();
        m_Reply->deleteLater();
        m_Reply = nullptr;
    }

    m_UpdateFile.close();

    if ((m_UpdateFile.size() == 0) || (error != QNetworkReply::NoError) || m_Canceled) {
        if (!m_Canceled) {
            MOBase::reportError(tr("Download failed: %1").arg(error));
        }
        m_UpdateFile.remove();
        return;
    }

    MOLog::instance().info("Download: {}", m_UpdateFile.fileName().toUtf8().toStdString());

    try {
        installUpdate();
    } catch (const std::exception& e) {
        MOBase::reportError(tr("Failed to install update: %1").arg(e.what()));
    }
}

void SelfUpdater::downloadCancel() { m_Canceled = true; }

void SelfUpdater::installUpdate() {
    // FIXME: This.
    const QString mopath = QDir::fromNativeSeparators(qApp->property("dataPath").toString());

    HINSTANCE res =
        ::ShellExecuteW(nullptr, L"open", m_UpdateFile.fileName().toStdWString().c_str(), nullptr, nullptr, SW_SHOW);

    // As per
    // https://msdn.microsoft.com/en-us/library/windows/desktop/bb762153%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
    // res is not a true HINSTANCE and should be cast to int.
    // Ugly, i know.
#pragma warning(suppress : 4311)
#pragma warning(suppress : 4302)
    int ires = reinterpret_cast<int>(res);
    if (ires > 32) {
        QCoreApplication::quit();
    } else {
        MOBase::reportError(tr("Failed to start %1: %2").arg(m_UpdateFile.fileName()).arg(ires));
    }

    m_UpdateFile.remove();
}

void SelfUpdater::report7ZipError(QString const& errorMessage) {
    QMessageBox::critical(m_Parent, tr("Error"), errorMessage);
}
