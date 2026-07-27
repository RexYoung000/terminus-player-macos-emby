#include "utils/LogRedaction.h"

#include <QCoreApplication>
#include <QTextStream>

namespace
{
bool expectRedaction(const QString& input, const QString& expected)
{
  QString actual = input;
  LogRedaction::CensorAuthTokens(actual);

  if (actual == expected)
    return true;

  QTextStream(stderr) << "Expected: " << expected << "\n"
                      << "Actual:   " << actual << "\n";
  return false;
}
}

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  bool passed = true;

  passed &= expectRedaction(
    "https://example.test/Videos?api_key=short&start=0",
    "https://example.test/Videos?api_key=[REDACTED]&start=0");
  passed &= expectRedaction(
    "X-MediaBrowser-Token=abcdefghijklmnopqrstuvwxyz0123456789",
    "X-MediaBrowser-Token=[REDACTED]");
  passed &= expectRedaction(
    "X-MediaBrowser-Token%3Dencoded-token%2Fvalue&x=1",
    "X-MediaBrowser-Token%3D[REDACTED]&x=1");
  passed &= expectRedaction(
    R"({"AccessToken":"json-secret","Name":"Rex"})",
    R"({"AccessToken":"[REDACTED]","Name":"Rex"})");
  passed &= expectRedaction(
    R"("Authorization":"Bearer header.payload.signature")",
    R"("Authorization":"Bearer [REDACTED]")");
  passed &= expectRedaction(
    "X-Emby-Token: emby-header-secret",
    "X-Emby-Token: [REDACTED]");
  passed &= expectRedaction(
    R"(Authorization: MediaBrowser Client="Terminus", Token="media-browser-secret")",
    R"(Authorization: MediaBrowser Client="Terminus", Token="[REDACTED]")");
  passed &= expectRedaction(
    "api_key=first&ApiKey=second",
    "api_key=[REDACTED]&ApiKey=[REDACTED]");
  passed &= expectRedaction(
    "https://example.test/Items?search=AccessToken",
    "https://example.test/Items?search=AccessToken");

  return passed ? 0 : 1;
}
