// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkablePath.h"

#include "PathUtils.h"

#include <QStringList>

RelinkablePath::RelinkablePath(std::filesystem::path path, Type type)
    : m_normalizedPath(normalize(path)), m_type(type) {}

std::filesystem::path RelinkablePath::normalize(const std::filesystem::path& path) {
  QString frontSlashes(core::toQString(path));
  frontSlashes.replace(QChar('\\'), QLatin1String("/"));

  QStringList newComponents;
  auto opt = Qt::KeepEmptyParts;
  for (const QString& comp : frontSlashes.split(QChar('/'), opt)) {
    if (comp.isEmpty()) {
      if (newComponents.isEmpty()
#if _WIN32
          || (newComponents.size() == 1 && newComponents.front().isEmpty())
#endif
      ) {
        newComponents.push_back(comp);
      } else {
        // This will get rid of redundant slashes, including the trailing slash.
        continue;
      }
    } else if (comp == ".") {
      continue;
    } else if (comp == "..") {
      if (newComponents.isEmpty()) {
        return {};
      }
      const QString& lastComp = newComponents.back();
      if (lastComp.isEmpty() || lastComp.endsWith(QChar(':'))) {
        return {};
      }
      newComponents.pop_back();
    } else {
      newComponents.push_back(comp);
    }
  }
  return core::toFileSystemPath(newComponents.join(QChar('/')));
}  // RelinkablePath::normalize
