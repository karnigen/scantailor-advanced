// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_PROJECTWRITER_H_
#define SCANTAILOR_CORE_PROJECTWRITER_H_

#include <QString>
#include <Qt>
#include <filesystem>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ImageId.h"
#include "OutputFileNameGenerator.h"
#include "PageId.h"
#include "PageSequence.h"
#include "SelectedPage.h"
#include "VirtualFunction.h"

class AbstractFilter;
class ProjectPages;
class PageInfo;
class QDomDocument;
class QDomElement;

class ProjectWriter {
  DECLARE_NON_COPYABLE(ProjectWriter)

 public:
  using FilterPtr = std::shared_ptr<AbstractFilter>;

  ProjectWriter(const std::shared_ptr<ProjectPages>& pageSequence,
                const SelectedPage& selectedPage,
                const OutputFileNameGenerator& outFileNameGen);

  ~ProjectWriter();

  bool write(const std::filesystem::path& filePath,
             const std::vector<FilterPtr>& filters,
             QString* errorMessage = nullptr,
             int backupCount = 0) const;

  /**
   * \p out will be called like this: out(ImageId, numeric_image_id)
   */
  template <typename OutFunc>
  void enumImages(OutFunc out) const;

  /**
   * \p out will be called like this: out(LogicalPageId, numeric_page_id)
   */
  template <typename OutFunc>
  void enumPages(OutFunc out) const;

 private:
  struct Directory {
    std::filesystem::path path;
    int numericId;

    Directory(std::filesystem::path path, int numericId) : path(std::move(path)), numericId(numericId) {}
  };

  struct File {
    std::filesystem::path path;
    int numericId;

    File(std::filesystem::path path, int numericId) : path(std::move(path)), numericId(numericId) {}
  };

  struct Image {
    ImageId id;
    int numericId;
    int numSubPages;
    bool leftHalfRemoved;
    bool rightHalfRemoved;

    Image(const PageInfo& pageInfo, int numericId);
  };

  struct Page {
    PageId id;
    int numericId;

    Page(const PageId& id, int numericId) : id(id), numericId(numericId) {}
  };

  template <typename Item, typename Key, typename Hash>
  class OrderedUniqueContainer {
   public:
    using iterator = typename std::list<Item>::iterator;
    using const_iterator = typename std::list<Item>::const_iterator;

    std::pair<iterator, bool> insert(const Key& key, Item item) {
      auto [it, inserted] = m_index.try_emplace(key, m_items.end());
      if (!inserted) {
        return {it->second, false};
      }

      try {
        it->second = m_items.emplace(m_items.end(), std::move(item));
      } catch (...) {
        m_index.erase(it);
        throw;
      }
      return {it->second, true};
    }

    iterator find(const Key& key) {
      const auto it = m_index.find(key);
      return it == m_index.end() ? m_items.end() : it->second;
    }

    const_iterator find(const Key& key) const {
      const auto it = m_index.find(key);
      return it == m_index.end() ? m_items.end() : it->second;
    }

    iterator begin() { return m_items.begin(); }

    const_iterator begin() const { return m_items.begin(); }

    iterator end() { return m_items.end(); }

    const_iterator end() const { return m_items.end(); }

   private:
    std::list<Item> m_items;
    std::unordered_map<Key, iterator, Hash> m_index;
  };

  using MetadataByImage = std::unordered_map<ImageId, ImageMetadata>;

  using Directories = OrderedUniqueContainer<Directory, std::filesystem::path, std::hash<std::filesystem::path>>;
  using Files = OrderedUniqueContainer<File, std::filesystem::path, std::hash<std::filesystem::path>>;
  using Images = OrderedUniqueContainer<Image, ImageId, std::hash<ImageId>>;
  using Pages = OrderedUniqueContainer<Page, PageId, std::hash<PageId>>;

  QDomElement processDirectories(QDomDocument& doc, const QString& projectDirPath) const;

  QDomElement processFiles(QDomDocument& doc) const;

  QDomElement processImages(QDomDocument& doc) const;

  QDomElement processPages(QDomDocument& doc) const;

  void writeImageMetadata(QDomDocument& doc, QDomElement& imageEl, const ImageId& imageId) const;

  int dirId(const std::filesystem::path& dirPath) const;

  int fileId(const std::filesystem::path& filePath) const;

  QString packFilePath(const std::filesystem::path& filePath) const;

  int imageId(const ImageId& imageId) const;

  int pageId(const PageId& pageId) const;

  void enumImagesImpl(const VirtualFunction<void, const ImageId&, int>& out) const;

  void enumPagesImpl(const VirtualFunction<void, const PageId&, int>& out) const;

  PageSequence m_pageSequence;
  OutputFileNameGenerator m_outFileNameGen;
  SelectedPage m_selectedPage;
  Directories m_dirs;
  Files m_files;
  Images m_images;
  Pages m_pages;
  MetadataByImage m_metadataByImage;
  Qt::LayoutDirection m_layoutDirection;
};


template <typename Callable>
void ProjectWriter::enumImages(Callable out) const {
  enumImagesImpl(ProxyFunction<Callable, void, const ImageId&, int>(out));
}

template <typename Callable>
void ProjectWriter::enumPages(Callable out) const {
  enumPagesImpl(ProxyFunction<Callable, void, const PageId&, int>(out));
}

#endif  // ifndef SCANTAILOR_CORE_PROJECTWRITER_H_
