#ifndef WEBCLIENTSERVER_H
#define WEBCLIENTSERVER_H

#include <QObject>
#include <QString>

namespace qhttp {
namespace server {
class QHttpRequest;
class QHttpResponse;
class QHttpServer;
}
}

class WebClientServer : public QObject
{
public:
  explicit WebClientServer(QObject* parent = nullptr);

  bool start(const QString& webClientRoot, const QString& extensionRoot);
  QString baseUrl() const;

private:
  void handleRequest(qhttp::server::QHttpRequest* request,
                     qhttp::server::QHttpResponse* response);

  qhttp::server::QHttpServer* m_server;
  QString m_webClientRoot;
  QString m_extensionRoot;
  QString m_baseUrl;
};

#endif // WEBCLIENTSERVER_H
