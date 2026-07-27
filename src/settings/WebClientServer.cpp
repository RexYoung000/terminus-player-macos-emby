#include "WebClientServer.h"

#include "QsLog.h"
#include "qhttpserver.hpp"
#include "qhttpserverrequest.hpp"
#include "qhttpserverresponse.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QMimeDatabase>
#include <QTcpServer>
#include <QUrl>

using namespace qhttp::server;

namespace {

constexpr quint16 WEB_CLIENT_PORT = 43110;

class LoopbackHttpServer : public QHttpServer
{
public:
  explicit LoopbackHttpServer(QObject* parent) : QHttpServer(parent)
  {
  }

  quint16 serverPort() const
  {
    return tcpServer() ? tcpServer()->serverPort() : 0;
  }
};

bool isWithinRoot(const QString& root, const QString& candidate)
{
  if (root.isEmpty() || candidate.isEmpty())
    return false;

  return candidate == root || candidate.startsWith(root + QDir::separator());
}

QByteArray mimeTypeForFile(const QString& path)
{
  if (path.endsWith(".wasm", Qt::CaseInsensitive))
    return "application/wasm";

  QMimeDatabase database;
  return database.mimeTypeForFile(path, QMimeDatabase::MatchExtension).name().toUtf8();
}

}

WebClientServer::WebClientServer(QObject* parent)
  : QObject(parent),
    m_server(nullptr)
{
}

bool WebClientServer::start(const QString& webClientRoot, const QString& extensionRoot)
{
  m_webClientRoot = QFileInfo(webClientRoot).canonicalFilePath();
  m_extensionRoot = QFileInfo(extensionRoot).canonicalFilePath();

  if (m_webClientRoot.isEmpty() || m_extensionRoot.isEmpty())
  {
    QLOG_ERROR() << "Could not locate bundled web-client resources.";
    return false;
  }

  auto server = new LoopbackHttpServer(this);
  if (!server->listen(QHostAddress::LocalHost, WEB_CLIENT_PORT))
  {
    QLOG_ERROR() << "Could not start the local web-client server.";
    server->deleteLater();
    return false;
  }

  connect(server, &QHttpServer::newRequest, this, &WebClientServer::handleRequest);

  m_server = server;
  m_baseUrl = QString("http://127.0.0.1:%1").arg(server->serverPort());
  QLOG_INFO() << "Bundled web-client available from the local loopback server.";
  return true;
}

QString WebClientServer::baseUrl() const
{
  return m_baseUrl;
}

void WebClientServer::handleRequest(QHttpRequest* request, QHttpResponse* response)
{
  if (request->method() != qhttp::EHTTP_GET && request->method() != qhttp::EHTTP_HEAD)
  {
    response->setStatusCode(qhttp::ESTATUS_METHOD_NOT_ALLOWED);
    response->end();
    return;
  }

  QString requestPath = QUrl::fromPercentEncoding(request->url().path().toUtf8());
  QString root = m_webClientRoot;

  if (requestPath.startsWith("/extension/"))
  {
    root = m_extensionRoot;
    requestPath.remove(0, QString("/extension/").size());
  }
  else
  {
    while (requestPath.startsWith('/'))
      requestPath.remove(0, 1);
  }

  if (requestPath.isEmpty())
    requestPath = "index.html";

  const QString cleanPath = QDir::cleanPath(requestPath);
  if (cleanPath == ".." || cleanPath.startsWith("../") || QDir::isAbsolutePath(cleanPath))
  {
    response->setStatusCode(qhttp::ESTATUS_FORBIDDEN);
    response->end();
    return;
  }

  QFileInfo fileInfo(QDir(root).filePath(cleanPath));
  const QString candidate = fileInfo.canonicalFilePath();
  if (!fileInfo.isFile() || !isWithinRoot(root, candidate))
  {
    response->setStatusCode(qhttp::ESTATUS_NOT_FOUND);
    response->end();
    return;
  }

  QFile file(candidate);
  if (!file.open(QIODevice::ReadOnly))
  {
    response->setStatusCode(qhttp::ESTATUS_INTERNAL_SERVER_ERROR);
    response->end();
    return;
  }

  const QByteArray data = file.readAll();
  response->setStatusCode(qhttp::ESTATUS_OK);
  response->addHeader("Content-Type", mimeTypeForFile(candidate));
  response->addHeader("Content-Length", QByteArray::number(data.size()));
  response->addHeader("Cache-Control", "no-cache");
  response->addHeader("X-Content-Type-Options", "nosniff");
  response->end(request->method() == qhttp::EHTTP_HEAD ? QByteArray() : data);
}
