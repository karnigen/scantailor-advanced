// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RecentProjects.h"

#include <QFile>
#include <QStringList>

#include "PathUtils.h"
#include "SettingsStore.h"

void RecentProjects::read() {
  SettingsStore settings;
  std::list<std::filesystem::path> newList;

  const QStringList paths = settings.stringList("project/recent");
  for (const QString& path : paths) {
    newList.push_back(core::toFileSystemPath(path));
  }

  m_projectFiles.swap(newList);
}

void RecentProjects::write(const int maxItems) const {
  SettingsStore settings;
  QStringList paths;
  for (const std::filesystem::path& path : m_projectFiles) {
    if (paths.size() >= maxItems) {
      break;
    }
    paths.push_back(core::toQString(path));
  }
  settings.setStringList("project/recent", paths);
}

bool RecentProjects::validate() {
  bool allOk = true;

  auto it(m_projectFiles.begin());
  const auto end(m_projectFiles.end());
  while (it != end) {
    if (QFile::exists(core::toQString(*it))) {
      ++it;
    } else {
      m_projectFiles.erase(it++);
      allOk = false;
    }
  }
  return allOk;
}

void RecentProjects::setMostRecent(const std::filesystem::path& filePath) {
  const auto begin(m_projectFiles.begin());
  const auto end(m_projectFiles.end());
  auto it(std::find(begin, end, filePath));
  if (it != end) {
    m_projectFiles.splice(begin, m_projectFiles, it);
  } else {
    m_projectFiles.push_front(filePath);
  }
}
