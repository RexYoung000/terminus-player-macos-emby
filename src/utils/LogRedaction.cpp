#include "LogRedaction.h"

#include <QRegularExpression>
#include <QVector>

namespace
{
struct SecretRange
{
  int start;
  int length;
};

void redactCapturedValues(QString& msg, const QRegularExpression& expression)
{
  QVector<SecretRange> ranges;
  QRegularExpressionMatchIterator matches = expression.globalMatch(msg);

  while (matches.hasNext())
  {
    const QRegularExpressionMatch match = matches.next();
    ranges.append({match.capturedStart(1), match.capturedLength(1)});
  }

  for (auto range = ranges.crbegin(); range != ranges.crend(); ++range)
    msg.replace(range->start, range->length, QStringLiteral("[REDACTED]"));
}
}

void LogRedaction::CensorAuthTokens(QString& msg)
{
  const auto caseInsensitive = QRegularExpression::CaseInsensitiveOption;
  const QString tokenNames =
    QStringLiteral("(?:api_key|ApiKey|AccessToken|X-MediaBrowser-Token|X-Emby-Token)");

  redactCapturedValues(
    msg,
    QRegularExpression(
      tokenNames + QStringLiteral("(?:=|%3D)([^&\\s\"'<>]+)"),
      caseInsensitive));

  redactCapturedValues(
    msg,
    QRegularExpression(
      QStringLiteral("[\"']") + tokenNames +
        QStringLiteral("[\"']\\s*:\\s*[\"']([^\"']+)[\"']"),
      caseInsensitive));

  redactCapturedValues(
    msg,
    QRegularExpression(
      QStringLiteral("[\"']?Authorization[\"']?\\s*[:=]\\s*[\"']?\\s*Bearer\\s+([^\"'\\s,}]+)"),
      caseInsensitive));

  redactCapturedValues(
    msg,
    QRegularExpression(
      tokenNames + QStringLiteral("\\s*:\\s*([^\\s,\"'<>}]+)"),
      caseInsensitive));

  redactCapturedValues(
    msg,
    QRegularExpression(
      QStringLiteral("[\"']?Authorization[\"']?\\s*[:=]\\s*[\"']?\\s*MediaBrowser[^\\r\\n]*?\\bToken\\s*=\\s*[\"']?([^\"'\\s,}]+)"),
      caseInsensitive));
}
