// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_PROJECTREADER_H_
#define SCANTAILOR_CORE_PROJECTREADER_H_

#include <QDomDocument>
#include <QString>
#include <Qt>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ImageId.h"
#include "ImageInfo.h"
#include "ImageMetadata.h"
#include "PageId.h"
#include "ProjectMigration.h"
#include "SelectedPage.h"

class QDomElement;
class ProjectPages;
class FileNameDisambiguator;
class AbstractFilter;

class ProjectReader {
 public:
  using FilterPtr = std::shared_ptr<AbstractFilter>;

  explicit ProjectReader(const QDomDocument& doc, std::filesystem::path projectFilePath = {});

  ~ProjectReader();

  static bool isVersionReadable(const QString& version);

  void readFilterSettings(const std::vector<FilterPtr>& filters) const;

  bool success() const { return (m_pages != nullptr); }

  const std::filesystem::path& outputDirectory() const { return m_outDir; }

  const QString& getVersion() const { return m_version; }

  const ProjectMigrationReport& migrationReport() const { return m_migrationReport; }

  const std::shared_ptr<ProjectPages>& pages() const { return m_pages; }

  const SelectedPage& selectedPage() const { return m_selectedPage; }

  const std::shared_ptr<FileNameDisambiguator>& namingDisambiguator() const { return m_disambiguator; }

  ImageId imageId(int numericId) const;

  PageId pageId(int numericId) const;

 private:
  struct FileRecord {
    std::filesystem::path filePath;
    bool compatMultiPage;  // Backwards compatibility.

    FileRecord() : compatMultiPage(false) {}

    FileRecord(std::filesystem::path filePath, bool compatMultiPage)
        : filePath(std::move(filePath)), compatMultiPage(compatMultiPage) {}
  };

  using DirMap = std::unordered_map<int, std::filesystem::path>;
  using FileMap = std::unordered_map<int, FileRecord>;
  using ImageMap = std::unordered_map<int, ImageInfo>;
  using PageMap = std::unordered_map<int, PageId>;

  void processDirectories(const QDomElement& dirsEl);

  void processFiles(const QDomElement& filesEl);

  void processImages(const QDomElement& imagesEl, Qt::LayoutDirection layoutDirection);

  ImageMetadata processImageMetadata(const QDomElement& imageEl);

  void processPages(const QDomElement& pagesEl);

  std::filesystem::path getDirPath(int id) const;

  FileRecord getFileRecord(int id) const;

  std::filesystem::path expandFilePath(const QString& pathShorthand) const;

  ImageInfo getImageInfo(int id) const;

  std::filesystem::path resolvePath(const std::filesystem::path& path) const;

  QDomDocument m_doc;
  std::filesystem::path m_projectDirPath;
  std::filesystem::path m_outDir;
  QString m_version;
  ProjectMigrationReport m_migrationReport;
  DirMap m_dirMap;
  FileMap m_fileMap;
  ImageMap m_imageMap;
  PageMap m_pageMap;
  SelectedPage m_selectedPage;
  std::shared_ptr<ProjectPages> m_pages;
  std::shared_ptr<FileNameDisambiguator> m_disambiguator;
};


#endif  // ifndef SCANTAILOR_CORE_PROJECTREADER_H_
