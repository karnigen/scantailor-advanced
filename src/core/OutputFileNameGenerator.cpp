// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "OutputFileNameGenerator.h"

#include <QFileInfo>
#include <utility>

#include "AbstractRelinker.h"
#include "PageId.h"
#include "PathUtils.h"
#include "ProjectPages.h"
#include "RelinkablePath.h"

OutputFileNameGenerator::OutputFileNameGenerator()
    : m_disambiguator(std::make_shared<FileNameDisambiguator>()), m_outDir(), m_layoutDirection(Qt::LeftToRight) {}

OutputFileNameGenerator::OutputFileNameGenerator(std::shared_ptr<FileNameDisambiguator> disambiguator,
                                                 std::filesystem::path outDir,
                                                 Qt::LayoutDirection layoutDirection,
                                                 std::shared_ptr<const ProjectPages> projectPages)
    : m_disambiguator(std::move(disambiguator)),
      m_outDir(std::move(outDir)),
      m_layoutDirection(layoutDirection),
      m_projectPages(std::move(projectPages)) {
  assert(m_disambiguator);
}

void OutputFileNameGenerator::performRelinking(const AbstractRelinker& relinker) {
  m_disambiguator->performRelinking(relinker);
  m_outDir = relinker.substitutionPathFor(RelinkablePath(m_outDir, RelinkablePath::Dir));
}

QString OutputFileNameGenerator::fileNameFor(const PageId& page) const {
  const Qt::LayoutDirection direction
      = m_projectPages ? m_projectPages->layoutDirectionFor(page.imageId()) : m_layoutDirection;
  const bool ltr = (direction == Qt::LeftToRight);
  const PageId::SubPage subPage = page.subPage();
  const std::filesystem::path& imagePath = page.imageId().filePath();
  const int label = m_disambiguator->getLabel(imagePath);

  QString name(QFileInfo(core::toQString(imagePath)).completeBaseName());
  if (label != 0) {
    name += QString::fromLatin1("(%1)").arg(label);
  }
  if (page.imageId().isMultiPageFile()) {
    name += QString::fromLatin1("_page%1").arg(page.imageId().page(), 4, 10, QLatin1Char('0'));
  }
  if (subPage != PageId::SINGLE_PAGE) {
    name += QLatin1Char('_');
    name += QLatin1Char(ltr == (subPage == PageId::LEFT_PAGE) ? '1' : '2');
    name += QLatin1Char(subPage == PageId::LEFT_PAGE ? 'L' : 'R');
  }
  name += QString::fromLatin1(".tif");
  return name;
}

std::filesystem::path OutputFileNameGenerator::filePathFor(const PageId& page) const {
  return std::filesystem::absolute(m_outDir / core::toFileSystemPath(fileNameFor(page)));
}
