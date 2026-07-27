#ifndef TERMINUSPLAYER_LOGREDACTION_H
#define TERMINUSPLAYER_LOGREDACTION_H

#include <QString>

namespace LogRedaction
{
  void CensorAuthTokens(QString& msg);
}

#endif // TERMINUSPLAYER_LOGREDACTION_H
