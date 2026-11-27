// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_RECENTPROJECTS_H_
#define SCANTAILOR_CORE_RECENTPROJECTS_H_

#include <QString>
#include <filesystem>
#include <limits>
#include <list>

class RecentProjects {
 public:
  /**
   * \brief The default value for maxItems parameters of
   *        write() and enumerate().
   */
  enum { DEFAULT_MAX_ITEMS = 7 };

  /**
   * \brief Reads the list of recent projects from application settings
   *        without validating them.
   *
   * The current list will be overwritten.
   */
  void read();

  /**
   * \brief Removes non-existing project files.
   *
   * \return true if no projects were removed, false otherwise.
   */
  bool validate();

  /**
   * \brief Appends a project to the list or moves it to the
   *        top of the list, if it was already there.
   */
  void setMostRecent(const std::filesystem::path& filePath);

  void write(int maxItems = DEFAULT_MAX_ITEMS) const;

  bool isEmpty() const { return m_projectFiles.empty(); }

  /**
   * \brief Calls out((const std::filesystem::path&)filePath) for every entry.
   *
   * Modifying this object from the callback is not allowed.
   */
  template <typename Out>
  void enumerate(Out out, int maxItems = DEFAULT_MAX_ITEMS) const;

 private:
  std::list<std::filesystem::path> m_projectFiles;
};


template <typename Out>
void RecentProjects::enumerate(Out out, int maxItems) const {
  std::list<std::filesystem::path>::const_iterator it(m_projectFiles.begin());
  const std::list<std::filesystem::path>::const_iterator end(m_projectFiles.end());
  for (; it != end && maxItems > 0; ++it, --maxItems) {
    out(*it);
  }
}

#endif  // ifndef SCANTAILOR_CORE_RECENTPROJECTS_H_
