// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Utils.h"

#include <QDir>
#include <QRegularExpression>
#include <QTextDocument>
#include <cmath>

#include "ApplicationSettings.h"
#include "PathUtils.h"

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <stdio.h>
#endif

namespace core {
bool Utils::overwritingRename(const QString& from, const QString& to) {
#ifdef Q_OS_WIN
  return MoveFileExW((WCHAR*) from.utf16(), (WCHAR*) to.utf16(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
  return rename(QFile::encodeName(from).data(), QFile::encodeName(to).data()) == 0;
#endif
}

QString Utils::richTextForLink(const QString& label, const QString& target) {
  return QString::fromLatin1(
             "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\""
             "\"http://www.w3.org/TR/REC-html40/strict.dtd\">"
             "<html><head><meta name=\"qrichtext\" content=\"1\" />"
             "</head><body><p style=\"margin-top:0px; margin-bottom:0px;"
             "margin-left:0px; margin-right:0px; -qt-block-indent:0;"
             "text-indent:0px;\"><a href=\"%1\">%2</a></p></body></html>")
      .arg(target.toHtmlEscaped(), label.toHtmlEscaped());
}

void Utils::maybeCreateCacheDir(const std::filesystem::path& outputDir) {
  QDir(core::toQString(outputDir)).mkdir(QString::fromLatin1("cache"));

  // QDir::mkdir() returns false if the directory already exists,
  // so to prevent confusion this function return void.
}

std::filesystem::path Utils::outputDirToThumbDir(const std::filesystem::path& outputDir) {
  if (outputDir.empty()) {
    return std::filesystem::path("/cache/thumbs");
  }
  return outputDir / "cache" / "thumbs";
}

std::shared_ptr<ThumbnailPixmapCache> Utils::createThumbnailCache(const std::filesystem::path& outputDir) {
  const QSize maxPixmapSize = ApplicationSettings::getInstance().getThumbnailQuality();
  const std::filesystem::path thumbsCachePath(outputDirToThumbDir(outputDir));
  return std::make_shared<ThumbnailPixmapCache>(thumbsCachePath, maxPixmapSize, 40, 5);
}

QString Utils::qssConvertPxToEm(const QString& stylesheet, const double base, const int precision) {
  QString result = "";
  const QRegularExpression pxToEm(R"((\d+(\.\d+)?)px)");

  QRegularExpressionMatchIterator iter = pxToEm.globalMatch(stylesheet);
  int prevIndex = 0;
  while (iter.hasNext()) {
    QRegularExpressionMatch match = iter.next();
    result.append(stylesheet.mid(prevIndex, match.capturedStart() - prevIndex));

    double value = match.captured(1).toDouble();
    value /= base;
    result.append(QString::number(value, 'f', precision)).append("em");

    prevIndex = match.capturedEnd();
  }
  result.append(stylesheet.mid(prevIndex));
  return result;
}
}  // namespace core
