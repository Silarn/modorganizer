#include <QJsonDocument>
#include <QEventLoop>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include "github.h"

#include <QThread>
#include <QCoreApplication>

static const QString GITHUB_URL("https://api.github.com");
static const QString USER_AGENT("GitHubPP");


GitHub::GitHub(const char *clientId)
  : m_AccessManager(new QNetworkAccessManager(this))
{
  if (m_AccessManager->networkAccessible()
      == QNetworkAccessManager::UnknownAccessibility) {
    m_AccessManager->setNetworkAccessible(QNetworkAccessManager::Accessible);
  }
}

QJsonArray GitHub::releases(const Repository &repo)
{
  QJsonDocument result
      = request(Method::GET,
                QString("repos/%1/%2/releases").arg(repo.owner, repo.project),
                QByteArray(),
                true);
  if (!result.isArray()) {
    throw GitHubException(result.object());
  }
  return result.array();
}

void GitHub::releases(const Repository &repo,
                      const std::function<void(const QJsonArray &)> &callback)
{
  request(Method::GET,
          QString("repos/%1/%2/releases").arg(repo.owner, repo.project),
          QByteArray(), [callback](const QJsonDocument &result) {
            if (!result.isArray()) {
              throw GitHubException(result.object());
            }
            callback(result.array());
          }, true);
}

QJsonDocument GitHub::handleReply(QNetworkReply *reply)
{
  int statusCode
      = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (statusCode != 200) {
    return QJsonDocument(QJsonObject(
        {{"http_status", statusCode},
         {"redirection",
          reply->attribute(QNetworkRequest::RedirectionTargetAttribute)
              .toString()},
         {"reason", reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute)
                        .toString()}}));
  }

  QByteArray data = reply->readAll();
  if (data.isNull() || data.isEmpty()
      || (strcmp(data.constData(), "null") == 0)) {
    return QJsonDocument();
  }

  QJsonParseError parseError;
  QJsonDocument result = QJsonDocument::fromJson(data, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    return QJsonDocument(
        QJsonObject({{"parse_error", parseError.errorString()}}));
  }

  return result;
}

QNetworkReply *GitHub::genReply(Method method, const QString &path,
                                const QByteArray &data, bool relative)
{
  QNetworkRequest request(relative ? GITHUB_URL + "/" + path : path);

  request.setHeader(QNetworkRequest::UserAgentHeader, USER_AGENT);
  request.setRawHeader("Accept", "application/vnd.github.v3+json");

  switch (method) {
    case Method::GET:
      return m_AccessManager->get(request);
    case Method::POST:
      return m_AccessManager->post(request, data);
    default:
      // this shouldn't be possible as all enum options are handled
      throw std::runtime_error("invalid method");
  }
}

QJsonDocument GitHub::request(Method method, const QString &path,
                              const QByteArray &data, bool relative)
{
  QEventLoop wait;
  QNetworkReply *reply = genReply(method, path, data, relative);

  connect(reply, SIGNAL(finished), &wait, SLOT(quit()));
  wait.exec();
  QJsonDocument result = handleReply(reply);
  reply->deleteLater();

  QJsonObject object = result.object();
  if (object.value("http_status").toDouble() == 301.0) {
    return request(method, object.value("redirection").toString(), data, false);
  } else {
    return result;
  }
}

void GitHub::request(Method method, const QString &path, const QByteArray &data,
                     const std::function<void(const QJsonDocument &)> &callback,
                     bool relative)
{
  QTimer *timer = new QTimer();
  timer->setSingleShot(true);
  timer->setInterval(30000);
  QNetworkReply *reply = genReply(method, path, data, relative);

  connect(reply, &QNetworkReply::finished, [this, reply, timer, method, data, callback]() {
    QJsonDocument result = handleReply(reply);
    QJsonObject object = result.object();
    timer->stop();
    if (object.value("http_status").toDouble() == 301.0) {
      request(method, object.value("redirection").toString(), data, callback,
              false);
    } else {
      callback(result);
    }
    reply->deleteLater();
  });

  connect(reply,
          static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(
              &QNetworkReply::error),
          [reply, timer, callback](QNetworkReply::NetworkError error) {
            qDebug("network error %d", error);
            timer->stop();
            callback(QJsonDocument(
                QJsonObject({{"network_error", reply->errorString()}})));
            reply->deleteLater();
          });

  connect(timer, &QTimer::timeout, [reply]() {
    qDebug("timeout");
    reply->abort();
  });
  timer->start();
}
