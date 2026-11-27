// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>
#include <Qt>
#include <filesystem>
#include <memory>

#include "FileNameDisambiguator.h"

class PageId;
class AbstractRelinker;
class ProjectPages;

class OutputFileNameGenerator {
  // Member-wise copying is OK.
 public:
  OutputFileNameGenerator();

  OutputFileNameGenerator(std::shared_ptr<FileNameDisambiguator> disambiguator,
                          std::filesystem::path outDir,
                          Qt::LayoutDirection layoutDirection,
                          std::shared_ptr<const ProjectPages> projectPages = nullptr);

  void performRelinking(const AbstractRelinker& relinker);

  void setLayoutDirection(Qt::LayoutDirection dir) { m_layoutDirection = dir; }

  Qt::LayoutDirection layoutDirection() const { return m_layoutDirection; }

  const std::filesystem::path& outDir() const { return m_outDir; }

  FileNameDisambiguator* disambiguator() { return m_disambiguator.get(); }

  const FileNameDisambiguator* disambiguator() const { return m_disambiguator.get(); }

  QString fileNameFor(const PageId& page) const;

  std::filesystem::path filePathFor(const PageId& page) const;

 private:
  std::shared_ptr<FileNameDisambiguator> m_disambiguator;
  std::filesystem::path m_outDir;
  Qt::LayoutDirection m_layoutDirection;
  std::shared_ptr<const ProjectPages> m_projectPages;
};
