// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_OUTPUTFILENAMEGENERATOR_H_
#define SCANTAILOR_CORE_OUTPUTFILENAMEGENERATOR_H_

#include <QString>
#include <Qt>
#include <filesystem>
#include <memory>

#include "FileNameDisambiguator.h"

class PageId;
class AbstractRelinker;

class OutputFileNameGenerator {
  // Member-wise copying is OK.
 public:
  OutputFileNameGenerator();

  OutputFileNameGenerator(std::shared_ptr<FileNameDisambiguator> disambiguator,
                          std::filesystem::path outDir,
                          Qt::LayoutDirection layoutDirection);

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
};


#endif  // ifndef SCANTAILOR_CORE_OUTPUTFILENAMEGENERATOR_H_
