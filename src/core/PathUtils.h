// Copyright (C) 2026 ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PATHUTILS_H_
#define SCANTAILOR_CORE_PATHUTILS_H_

#include <filesystem>
#include <string>

#include <QByteArray>
#include <QString>

namespace core {

inline std::filesystem::path toFileSystemPath(const QString& path) {
#ifdef _WIN32
  return std::filesystem::path(path.toStdWString());
#else
  const QByteArray utf8Path = path.toUtf8();
  return std::filesystem::path(utf8Path.constData());
#endif
}

inline QString toQString(const std::filesystem::path& path) {
#ifdef _WIN32
  return QString::fromStdWString(path.native());
#else
  const std::string utf8Path = path.u8string();
  return QString::fromUtf8(utf8Path.data(), static_cast<qsizetype>(utf8Path.size()));
#endif
}

}  // namespace core

#endif  // SCANTAILOR_CORE_PATHUTILS_H_
