// Copyright (C) 2026
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CompletionSound.h"

#include <QApplication>
#include <QProcess>
#include <QStandardPaths>

#include <cstdlib>

bool CompletionSound::play(const QString& command) {
  const QString trimmedCommand = command.trimmed();
  if (!trimmedCommand.isEmpty()) {
    const int result = std::system(trimmedCommand.toLocal8Bit().constData());
    if (result == 0) {
      return true;
    }
    qWarning("The configured completion sound command failed with status %d.", result);
  }

#if defined(Q_OS_LINUX)
  const QString canberraPlayer = QStandardPaths::findExecutable(QStringLiteral("canberra-gtk-play"));
  if (!canberraPlayer.isEmpty() &&
      QProcess::startDetached(canberraPlayer, {QStringLiteral("--id=complete")})) {
    return true;
  }

  QString soundFile = QStandardPaths::locate(
      QStandardPaths::GenericDataLocation, QStringLiteral("sounds/freedesktop/stereo/complete.oga"));
  if (soundFile.isEmpty()) {
    soundFile = QStandardPaths::locate(
        QStandardPaths::GenericDataLocation, QStringLiteral("sounds/freedesktop/stereo/bell.oga"));
  }
  if (!soundFile.isEmpty()) {
    for (const QString& playerName : {QStringLiteral("paplay"), QStringLiteral("pw-play")}) {
      const QString player = QStandardPaths::findExecutable(playerName);
      if (!player.isEmpty() && QProcess::startDetached(player, {soundFile})) {
        return true;
      }
    }
  }

  qWarning("No Linux completion sound player and sound theme were available.");
#endif

  QApplication::beep();
  return false;
}
