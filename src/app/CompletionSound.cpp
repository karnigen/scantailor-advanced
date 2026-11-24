// Copyright (C) 2026
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "CompletionSound.h"

#include <QApplication>
#include <QCoreApplication>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>

#include <memory>

namespace {

constexpr int kCustomCommandTimeoutMs = 5000;

bool playPlatformFallback() {
#if defined(Q_OS_LINUX)
  const QString canberraPlayer = QStandardPaths::findExecutable(QStringLiteral("canberra-gtk-play"));
  if (!canberraPlayer.isEmpty() && QProcess::startDetached(canberraPlayer, {QStringLiteral("--id=complete")})) {
    return true;
  }

  QString soundFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                             QStringLiteral("sounds/freedesktop/stereo/complete.oga"));
  if (soundFile.isEmpty()) {
    soundFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                       QStringLiteral("sounds/freedesktop/stereo/bell.oga"));
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

}  // namespace

bool CompletionSound::play(const QString& command) {
  const QString trimmedCommand = command.trimmed();
  if (trimmedCommand.isEmpty()) {
    return playPlatformFallback();
  }

  auto* process = new QProcess(QCoreApplication::instance());
  const auto fallbackStarted = std::make_shared<bool>(false);
  const auto playFallbackOnce = [fallbackStarted]() {
    if (*fallbackStarted) {
      return;
    }
    *fallbackStarted = true;
    playPlatformFallback();
  };

  QObject::connect(process, &QProcess::errorOccurred, process,
                   [process, playFallbackOnce](QProcess::ProcessError error) {
                     if (error == QProcess::FailedToStart) {
                       qWarning("The configured completion sound command could not be started.");
                       playFallbackOnce();
                       process->deleteLater();
                     }
                   });
  QObject::connect(process, &QProcess::finished, process,
                   [process, fallbackStarted, playFallbackOnce](int exitCode, QProcess::ExitStatus exitStatus) {
                     if (!*fallbackStarted && (exitStatus != QProcess::NormalExit || exitCode != 0)) {
                       qWarning("The configured completion sound command failed with status %d.", exitCode);
                       playFallbackOnce();
                     }
                     process->deleteLater();
                   });
  QTimer::singleShot(kCustomCommandTimeoutMs, process, [process, playFallbackOnce]() {
    if (process->state() != QProcess::NotRunning) {
      qWarning("The configured completion sound command timed out.");
      playFallbackOnce();
      process->kill();
    }
  });

#if defined(Q_OS_WIN)
  process->start(QStringLiteral("cmd.exe"), {QStringLiteral("/C"), trimmedCommand});
#else
  process->start(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), trimmedCommand});
#endif
  return true;
}
