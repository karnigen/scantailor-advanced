// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProjectWriter.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QTextStream>
#include <QtXml>

#include "AbstractFilter.h"
#include "FileNameDisambiguator.h"
#include "ImageId.h"
#include "ImageMetadata.h"
#include "PageId.h"
#include "PageInfo.h"
#include "PageView.h"
#include "PathUtils.h"
#include "ProjectBackup.h"
#include "ProjectMigration.h"
#include "ProjectPages.h"
#include "version.h"

#ifndef Q_MOC_RUN

#include <functional>

#endif

#include <cassert>
#include <cstddef>

ProjectWriter::ProjectWriter(const std::shared_ptr<ProjectPages>& pageSequence,
                             const SelectedPage& selectedPage,
                             const OutputFileNameGenerator& outFileNameGen)
    : m_pageSequence(pageSequence->toPageSequence(PAGE_VIEW)),
      m_outFileNameGen(outFileNameGen),
      m_selectedPage(selectedPage),
      m_layoutDirection(pageSequence->layoutDirection()) {
  int nextId = 1;
  for (const PageInfo& page : m_pageSequence) {
    const PageId& pageId = page.id();
    const ImageId& imageId = pageId.imageId();
    const std::filesystem::path& filePath(imageId.filePath());
    const std::filesystem::path dirPath(core::toFileSystemPath(QFileInfo(core::toQString(filePath)).absolutePath()));

    m_metadataByImage[imageId] = page.metadata();

    if (m_dirs.insert(dirPath, Directory(dirPath, nextId)).second) {
      ++nextId;
    }

    if (m_files.insert(filePath, File(filePath, nextId)).second) {
      ++nextId;
    }

    if (m_images.insert(imageId, Image(page, nextId)).second) {
      ++nextId;
    }

    if (m_pages.insert(pageId, Page(pageId, nextId)).second) {
      ++nextId;
    }
  }
}

ProjectWriter::~ProjectWriter() = default;

static QString toRelativeIfPossible(const QString& projectDirPath, const QString& absolutePath) {
  if (absolutePath.isEmpty()) {
    return absolutePath;
  }
  const QDir projectDir(projectDirPath);
  const QString rel(projectDir.relativeFilePath(absolutePath));
  if (QDir::isAbsolutePath(rel)) {
    return absolutePath;
  }

  const QString canonicalAbsolutePath(QFileInfo(absolutePath).canonicalFilePath());
  const QString canonicalRelativePath(QFileInfo(projectDir.filePath(rel)).canonicalFilePath());
  if (canonicalAbsolutePath.isEmpty() || canonicalAbsolutePath != canonicalRelativePath) {
    return absolutePath;
  }
  return rel;
}

bool ProjectWriter::write(const std::filesystem::path& filePath,
                          const std::vector<FilterPtr>& filters,
                          QString* errorMessage,
                          const int backupCount) const {
  const QString qtFilePath(core::toQString(filePath));
  if (errorMessage) {
    errorMessage->clear();
  }

  std::filesystem::path backupFilePath;
  if (!ProjectMigration::createBackupForFormat3(filePath, &backupFilePath, errorMessage)) {
    return false;
  }

  const QString projectDirPath(QFileInfo(qtFilePath).absolutePath());

  QDomDocument doc;
  QDomElement rootEl(doc.createElement("project"));
  doc.appendChild(rootEl);
  rootEl.setAttribute("version", PROJECT_VERSION);
  rootEl.setAttribute("outputDirectory",
                      toRelativeIfPossible(projectDirPath, core::toQString(m_outFileNameGen.outDir())));
  rootEl.setAttribute("layoutDirection", m_layoutDirection == Qt::LeftToRight ? "LTR" : "RTL");

  rootEl.appendChild(processDirectories(doc, projectDirPath));
  rootEl.appendChild(processFiles(doc));
  rootEl.appendChild(processImages(doc));
  rootEl.appendChild(processPages(doc));
  rootEl.appendChild(m_outFileNameGen.disambiguator()->toXml(
      doc, "file-name-disambiguation", std::bind(&ProjectWriter::packFilePath, this, std::placeholders::_1)));

  QDomElement filtersEl(doc.createElement("filters"));
  rootEl.appendChild(filtersEl);
  auto it(filters.begin());
  const auto end(filters.end());
  for (; it != end; ++it) {
    filtersEl.appendChild((*it)->saveSettings(*this, doc));
  }

  QByteArray serializedProject;
  QBuffer buffer(&serializedProject);
  if (!buffer.open(QIODevice::WriteOnly)) {
    if (errorMessage) {
      *errorMessage = buffer.errorString();
    }
    return false;
  }
  QTextStream strm(&buffer);
  doc.save(strm, 2);
  strm.flush();
  if (strm.status() != QTextStream::Ok) {
    if (errorMessage) {
      *errorMessage = QString("Could not serialize project data.");
    }
    return false;
  }
  buffer.close();

  QSaveFile file(qtFilePath);
  if (!file.open(QIODevice::WriteOnly)) {
    if (errorMessage) {
      *errorMessage = file.errorString();
    }
    return false;
  }
  if (backupCount > 0 && QFileInfo::exists(qtFilePath)) {
    QFile existingProject(qtFilePath);
    if (!existingProject.open(QIODevice::ReadOnly)) {
      file.cancelWriting();
      if (errorMessage) {
        *errorMessage = existingProject.errorString();
      }
      return false;
    }
    bool projectChanged = existingProject.size() != serializedProject.size();
    qint64 offset = 0;
    const qint64 chunkSize = 1024 * 1024;
    while (!projectChanged && offset < serializedProject.size()) {
      const qint64 bytesToRead = qMin(chunkSize, serializedProject.size() - offset);
      const QByteArray existingChunk = existingProject.read(bytesToRead);
      if (existingProject.error() != QFileDevice::NoError) {
        file.cancelWriting();
        if (errorMessage) {
          *errorMessage = existingProject.errorString();
        }
        return false;
      }
      if (existingChunk.size() != bytesToRead
          || existingChunk != serializedProject.mid(offset, static_cast<int>(bytesToRead))) {
        projectChanged = true;
      }
      offset += bytesToRead;
    }
    if (projectChanged) {
      QString backupError;
      if (!createProjectBackup(filePath, backupCount, &backupError)) {
        file.cancelWriting();
        if (errorMessage) {
          *errorMessage = backupError;
        }
        return false;
      }
    }
  }
  if (file.write(serializedProject) != serializedProject.size()) {
    file.cancelWriting();
    if (errorMessage) {
      *errorMessage = file.errorString();
    }
    return false;
  }
  if (!file.commit()) {
    if (errorMessage) {
      *errorMessage = file.errorString();
    }
    return false;
  }
  return true;
}  // ProjectWriter::write

QDomElement ProjectWriter::processDirectories(QDomDocument& doc, const QString& projectDirPath) const {
  QDomElement dirsEl(doc.createElement("directories"));

  for (const Directory& dir : m_dirs) {
    QDomElement dirEl(doc.createElement("directory"));
    dirEl.setAttribute("id", dir.numericId);
    dirEl.setAttribute("path", toRelativeIfPossible(projectDirPath, core::toQString(dir.path)));
    dirsEl.appendChild(dirEl);
  }
  return dirsEl;
}

QDomElement ProjectWriter::processFiles(QDomDocument& doc) const {
  QDomElement filesEl(doc.createElement("files"));

  for (const File& file : m_files) {
    const QFileInfo fileInfo(core::toQString(file.path));
    const std::filesystem::path dirPath(core::toFileSystemPath(fileInfo.absolutePath()));
    QDomElement fileEl(doc.createElement("file"));
    fileEl.setAttribute("id", file.numericId);
    fileEl.setAttribute("dirId", dirId(dirPath));
    fileEl.setAttribute("name", fileInfo.fileName());
    filesEl.appendChild(fileEl);
  }
  return filesEl;
}

QDomElement ProjectWriter::processImages(QDomDocument& doc) const {
  QDomElement imagesEl(doc.createElement("images"));

  for (const Image& image : m_images) {
    QDomElement imageEl(doc.createElement("image"));
    imageEl.setAttribute("id", image.numericId);
    imageEl.setAttribute("subPages", image.numSubPages);
    imageEl.setAttribute("fileId", fileId(image.id.filePath()));
    imageEl.setAttribute("fileImage", image.id.page());
    if (image.leftHalfRemoved != image.rightHalfRemoved) {
      // Both are not supposed to be removed.
      imageEl.setAttribute("removed", image.leftHalfRemoved ? "L" : "R");
    }
    writeImageMetadata(doc, imageEl, image.id);
    imagesEl.appendChild(imageEl);
  }
  return imagesEl;
}

void ProjectWriter::writeImageMetadata(QDomDocument& doc, QDomElement& imageEl, const ImageId& imageId) const {
  auto it(m_metadataByImage.find(imageId));
  assert(it != m_metadataByImage.end());
  const ImageMetadata& metadata = it->second;

  QDomElement sizeEl(doc.createElement("size"));
  sizeEl.setAttribute("width", metadata.size().width());
  sizeEl.setAttribute("height", metadata.size().height());
  imageEl.appendChild(sizeEl);

  QDomElement dpiEl(doc.createElement("dpi"));
  dpiEl.setAttribute("horizontal", metadata.dpi().horizontal());
  dpiEl.setAttribute("vertical", metadata.dpi().vertical());
  imageEl.appendChild(dpiEl);
}

QDomElement ProjectWriter::processPages(QDomDocument& doc) const {
  QDomElement pagesEl(doc.createElement("pages"));

  const PageId selOpt1(m_selectedPage.get(IMAGE_VIEW));
  const PageId selOpt2(m_selectedPage.get(PAGE_VIEW));

  PageId pageLeft;
  PageId pageRight;
  if (selOpt2.subPage() == PageId::SINGLE_PAGE) {
    // In case it was split later select first of its pages found
    pageLeft = PageId(selOpt2.imageId(), PageId::LEFT_PAGE);
    pageRight = PageId(selOpt2.imageId(), PageId::RIGHT_PAGE);
  }


  for (const PageInfo& page : m_pageSequence) {
    const PageId& pageId = page.id();
    QDomElement pageEl(doc.createElement("page"));
    pageEl.setAttribute("id", this->pageId(pageId));
    pageEl.setAttribute("imageId", imageId(pageId.imageId()));
    pageEl.setAttribute("subPage", pageId.subPageAsString());
    if ((pageId == selOpt1) || (pageId == selOpt2) || (pageId == pageLeft) || (pageId == pageRight)) {
      pageEl.setAttribute("selected", "selected");
      pageLeft = pageRight = PageId();  // if one of these match other shouldn't
    }
    pagesEl.appendChild(pageEl);
  }
  return pagesEl;
}  // ProjectWriter::processPages

int ProjectWriter::dirId(const std::filesystem::path& dirPath) const {
  const Directories::const_iterator it(m_dirs.find(dirPath));
  assert(it != m_dirs.end());
  return it->numericId;
}

int ProjectWriter::fileId(const std::filesystem::path& filePath) const {
  const Files::const_iterator it(m_files.find(filePath));
  if (it != m_files.end()) {
    return it->numericId;
  } else {
    return -1;
  }
}

QString ProjectWriter::packFilePath(const std::filesystem::path& filePath) const {
  const Files::const_iterator it(m_files.find(filePath));
  if (it != m_files.end()) {
    return QString::number(it->numericId);
  } else {
    return QString();
  }
}

int ProjectWriter::imageId(const ImageId& imageId) const {
  const Images::const_iterator it(m_images.find(imageId));
  assert(it != m_images.end());
  return it->numericId;
}

int ProjectWriter::pageId(const PageId& pageId) const {
  const Pages::const_iterator it(m_pages.find(pageId));
  assert(it != m_pages.end());
  return it->numericId;
}

void ProjectWriter::enumImagesImpl(const VirtualFunction<void, const ImageId&, int>& out) const {
  for (const Image& image : m_images) {
    out(image.id, image.numericId);
  }
}

void ProjectWriter::enumPagesImpl(const VirtualFunction<void, const PageId&, int>& out) const {
  for (const Page& page : m_pages) {
    out(page.id, page.numericId);
  }
}

/*======================== ProjectWriter::Image =========================*/

ProjectWriter::Image::Image(const PageInfo& page, int numericId)
    : id(page.imageId()),
      numericId(numericId),
      numSubPages(page.imageSubPages()),
      leftHalfRemoved(page.leftHalfRemoved()),
      rightHalfRemoved(page.rightHalfRemoved()) {}
