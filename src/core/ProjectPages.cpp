// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ProjectPages.h"

#include <QDebug>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <map>
#include <unordered_map>
#include <utility>

#include "AbstractRelinker.h"
#include "ImageFileInfo.h"
#include "ImageInfo.h"
#include "OrthogonalRotation.h"
#include "PageSequence.h"
#include "PathUtils.h"
#include "RelinkablePath.h"

ProjectPages::ProjectPages(const Qt::LayoutDirection layoutDirection) {
  initSubPagesInOrder(layoutDirection);
}

ProjectPages::ProjectPages(const std::vector<ImageInfo>& info, const Qt::LayoutDirection layoutDirection) {
  initSubPagesInOrder(layoutDirection);

  for (const ImageInfo& image : info) {
    ImageDesc imageDesc(image);
    // Enforce some rules.
    if (imageDesc.numLogicalPages == 2) {
      imageDesc.leftHalfRemoved = false;
      imageDesc.rightHalfRemoved = false;
    } else if (imageDesc.numLogicalPages != 1) {
      continue;
    } else if (imageDesc.leftHalfRemoved && imageDesc.rightHalfRemoved) {
      imageDesc.leftHalfRemoved = false;
      imageDesc.rightHalfRemoved = false;
    }

    m_images.push_back(imageDesc);
  }
}

ProjectPages::ProjectPages(const std::vector<ImageFileInfo>& files,
                           const Pages pages,
                           const Qt::LayoutDirection layoutDirection) {
  initSubPagesInOrder(layoutDirection);

  for (const ImageFileInfo& file : files) {
    const std::filesystem::path filePath = core::toFileSystemPath(file.fileInfo().absoluteFilePath());
    const std::vector<ImageMetadata>& images = file.imageInfo();
    const auto numImages = static_cast<int>(images.size());
    const int multiPageBase = numImages > 1 ? 1 : 0;
    for (int i = 0; i < numImages; ++i) {
      const ImageMetadata& metadata = images[i];
      const ImageId id(filePath, multiPageBase + i);
      m_images.emplace_back(id, metadata, pages);
    }
  }
}

ProjectPages::~ProjectPages() = default;

Qt::LayoutDirection ProjectPages::layoutDirection() const {
  const QMutexLocker locker(&m_mutex);
  return m_layoutDirection;
}

Qt::LayoutDirection ProjectPages::layoutDirectionFor(const ImageId& imageId) const {
  const QMutexLocker locker(&m_mutex);
  for (const ImageDesc& image : m_images) {
    if (image.id == imageId) {
      return image.layoutDirectionOverride.value_or(m_layoutDirection);
    }
  }
  return m_layoutDirection;
}

std::optional<Qt::LayoutDirection> ProjectPages::layoutDirectionOverride(const ImageId& imageId) const {
  const QMutexLocker locker(&m_mutex);
  for (const ImageDesc& image : m_images) {
    if (image.id == imageId) {
      return image.layoutDirectionOverride;
    }
  }
  return std::nullopt;
}

void ProjectPages::setLayoutDirectionFor(const ImageId& imageId, const Qt::LayoutDirection direction) {
  bool wasModified = false;
  {
    const QMutexLocker locker(&m_mutex);
    for (ImageDesc& image : m_images) {
      if (image.id != imageId) {
        continue;
      }

      const Qt::LayoutDirection currentDirection = image.layoutDirectionOverride.value_or(m_layoutDirection);
      if (currentDirection == direction) {
        break;
      }

      if (direction == m_layoutDirection) {
        image.layoutDirectionOverride.reset();
      } else {
        image.layoutDirectionOverride = direction;
      }
      image.layoutRevision = ++m_nextLayoutRevision;
      wasModified = true;
      break;
    }
  }

  if (wasModified) {
    emit modified();
    emit pageOrderChanged();
  }
}

int ProjectPages::numLogicalPagesFor(const ImageId& imageId) const {
  const QMutexLocker locker(&m_mutex);
  for (const ImageDesc& image : m_images) {
    if (image.id == imageId) {
      return image.numLogicalPages;
    }
  }
  return 0;
}

void ProjectPages::setLayoutDirection(const Qt::LayoutDirection dir) {
  bool wasModified = false;
  {
    const QMutexLocker locker(&m_mutex);
    if (m_layoutDirection != dir) {
      initSubPagesInOrder(dir);
      for (ImageDesc& image : m_images) {
        if (!image.layoutDirectionOverride) {
          image.layoutRevision = ++m_nextLayoutRevision;
        }
      }
      wasModified = true;
    }
  }
  if (wasModified) {
    emit modified();
    emit pageOrderChanged();
  }
}

void ProjectPages::initSubPagesInOrder(const Qt::LayoutDirection layoutDirection) {
  m_layoutDirection = layoutDirection;
}

PageSequence ProjectPages::toPageSequence(const PageView view) const {
  PageSequence pages;

  if (view == PAGE_VIEW) {
    QMutexLocker locker(&m_mutex);

    const auto numImages = static_cast<int>(m_images.size());
    for (int i = 0; i < numImages; ++i) {
      const ImageDesc& image = m_images[i];
      assert(image.numLogicalPages >= 1 && image.numLogicalPages <= 2);
      for (int j = 0; j < image.numLogicalPages; ++j) {
        const PageId id(image.id,
                        image.logicalPageToSubPage(j, image.layoutDirectionOverride.value_or(m_layoutDirection)));
        pages.append(
            PageInfo(id, image.metadata, image.numLogicalPages, image.leftHalfRemoved, image.rightHalfRemoved));
      }
    }
  } else {
    assert(view == IMAGE_VIEW);

    QMutexLocker locker(&m_mutex);

    const auto numImages = static_cast<int>(m_images.size());
    for (int i = 0; i < numImages; ++i) {
      const ImageDesc& image = m_images[i];
      const PageId id(image.id, PageId::SINGLE_PAGE);
      pages.append(PageInfo(id, image.metadata, image.numLogicalPages, image.leftHalfRemoved, image.rightHalfRemoved));
    }
  }
  return pages;
}  // ProjectPages::toPageSequence

void ProjectPages::listRelinkablePaths(const VirtualFunction<void, const RelinkablePath&>& sink) const {
  // It's generally a bad idea to do callbacks while holding an internal mutex,
  // so we accumulate results into this vector first.
  std::vector<std::filesystem::path> files;

  {
    QMutexLocker locker(&m_mutex);

    files.reserve(m_images.size());
    for (const ImageDesc& image : m_images) {
      files.push_back(image.id.filePath());
    }
  }

  for (const std::filesystem::path& file : files) {
    sink(RelinkablePath(file, RelinkablePath::File));
  }
}

void ProjectPages::performRelinking(const AbstractRelinker& relinker) {
  QMutexLocker locker(&m_mutex);
  for (ImageDesc& image : m_images) {
    const RelinkablePath oldPath(image.id.filePath(), RelinkablePath::File);
    image.id.setFilePath(relinker.substitutionPathFor(oldPath));
    image.layoutRevision = ++m_nextLayoutRevision;
  }
}

void ProjectPages::setLayoutTypeFor(const ImageId& imageId, const LayoutType layout) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    setLayoutTypeForImpl(imageId, layout, &wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

std::optional<std::uint64_t> ProjectPages::layoutRevision(const PageInfo& pageInfo) const {
  const QMutexLocker locker(&m_mutex);
  for (const ImageDesc& image : m_images) {
    if (image.id == pageInfo.imageId() && image.metadata == pageInfo.metadata()) {
      return image.layoutRevision;
    }
  }
  return std::nullopt;
}

bool ProjectPages::setLayoutTypeForIfUnchanged(const ImageId& imageId,
                                               const LayoutType layout,
                                               const std::uint64_t revision) {
  bool wasModified = false;
  {
    const QMutexLocker locker(&m_mutex);
    const auto it = std::find_if(m_images.begin(), m_images.end(),
                                 [&imageId](const ImageDesc& image) { return image.id == imageId; });
    if (it == m_images.end() || revision != it->layoutRevision) {
      return false;
    }
    setLayoutTypeForImpl(imageId, layout, &wasModified);
  }
  if (wasModified) {
    emit modified();
  }
  return true;
}

void ProjectPages::setLayoutTypeForAllPages(const LayoutType layout) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    setLayoutTypeForAllPagesImpl(layout, &wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

void ProjectPages::autoSetLayoutTypeFor(const ImageId& imageId, const OrthogonalRotation rotation) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    autoSetLayoutTypeForImpl(imageId, rotation, &wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

void ProjectPages::updateImageMetadata(const ImageId& imageId, const ImageMetadata& metadata) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    updateImageMetadataImpl(imageId, metadata, &wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

int ProjectPages::adviseNumberOfLogicalPages(const ImageMetadata& metadata, const OrthogonalRotation rotation) {
  const QSize size(rotation.rotate(metadata.size()));
  const QSize dpi(rotation.rotate(metadata.dpi().toSize()));

  if (size.width() * dpi.height() > size.height() * dpi.width()) {
    return 2;
  } else {
    return 1;
  }
}

int ProjectPages::numImages() const {
  QMutexLocker locker(&m_mutex);
  return static_cast<int>(m_images.size());
}

std::vector<PageInfo> ProjectPages::insertImage(const ImageInfo& newImage,
                                                BeforeOrAfter beforeOrAfter,
                                                const ImageId& existing,
                                                const PageView view) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    return insertImageImpl(newImage, beforeOrAfter, existing, view, wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

void ProjectPages::removePages(const std::set<PageId>& pages) {
  bool wasModified = false;

  {
    QMutexLocker locker(&m_mutex);
    removePagesImpl(pages, wasModified);
  }

  if (wasModified) {
    emit modified();
  }
}

PageInfo ProjectPages::unremovePage(const PageId& pageId) {
  bool wasModified = false;

  PageInfo pageInfo;

  {
    QMutexLocker locker(&m_mutex);
    pageInfo = unremovePageImpl(pageId, wasModified);
  }

  if (wasModified) {
    emit modified();
  }
  return pageInfo;
}

bool ProjectPages::validateDpis() const {
  QMutexLocker locker(&m_mutex);

  for (const ImageDesc& image : m_images) {
    if (!image.metadata.isDpiOK()) {
      return false;
    }
  }
  return true;
}

namespace {
struct File {
  std::filesystem::path fileName;
  mutable std::vector<ImageMetadata> metadata;
  mutable std::vector<int> imageIndexes;

  explicit File(std::filesystem::path fname) : fileName(std::move(fname)) {}

  explicit operator ImageFileInfo() const {
    return ImageFileInfo(QFileInfo(core::toQString(fileName)), metadata, imageIndexes);
  }
};
}  // anonymous namespace

std::vector<ImageFileInfo> ProjectPages::toImageFileInfo() const {
  std::vector<File> files;
  std::map<std::filesystem::path, std::size_t> filesByName;

  {
    QMutexLocker locker(&m_mutex);

    for (const ImageDesc& image : m_images) {
      const std::filesystem::path& filePath = image.id.filePath();
      const auto [it, inserted] = filesByName.emplace(filePath, files.size());
      if (inserted) {
        files.emplace_back(filePath);
      }
      files[it->second].metadata.push_back(image.metadata);
      files[it->second].imageIndexes.push_back(image.id.page());
    }
  }

  std::vector<ImageFileInfo> imageFiles;
  imageFiles.reserve(files.size());
  for (const File& file : files) {
    imageFiles.push_back(static_cast<ImageFileInfo>(file));
  }
  return imageFiles;
}

void ProjectPages::updateMetadataFrom(const std::vector<ImageFileInfo>& files) {
  using MetadataMap = std::unordered_map<ImageId, ImageMetadata>;
  MetadataMap metadataMap;

  for (const ImageFileInfo& file : files) {
    const std::filesystem::path filePath = core::toFileSystemPath(file.fileInfo().filePath());
    for (std::size_t imageIndex = 0; imageIndex < file.imageInfo().size(); ++imageIndex) {
      metadataMap[ImageId(filePath, file.imageIndexes()[imageIndex])] = file.imageInfo()[imageIndex];
    }
  }

  bool wasModified = false;
  {
    QMutexLocker locker(&m_mutex);
    for (ImageDesc& image : m_images) {
      const MetadataMap::const_iterator it(metadataMap.find(image.id));
      if (it != metadataMap.end() && image.metadata != it->second) {
        image.metadata = it->second;
        image.layoutRevision = ++m_nextLayoutRevision;
        wasModified = true;
      }
    }
  }

  if (wasModified) {
    emit modified();
  }
}

void ProjectPages::setLayoutTypeForImpl(const ImageId& imageId, const LayoutType layout, bool* modified) {
  const int numPages = (layout == TWO_PAGE_LAYOUT ? 2 : 1);
  const auto numImages = static_cast<int>(m_images.size());
  for (int i = 0; i < numImages; ++i) {
    ImageDesc& image = m_images[i];
    if (image.id == imageId) {
      image.layoutRevision = ++m_nextLayoutRevision;
      int adjustedNumPages = numPages;
      if ((numPages == 2) && (image.leftHalfRemoved != image.rightHalfRemoved)) {
        // Both can't be removed, but we handle that case anyway
        // by treating it like none are removed.
        --adjustedNumPages;
      }

      const int delta = adjustedNumPages - image.numLogicalPages;
      if (delta == 0) {
        break;
      }

      image.numLogicalPages = adjustedNumPages;

      *modified = true;
      break;
    }
  }
}

void ProjectPages::setLayoutTypeForAllPagesImpl(const LayoutType layout, bool* modified) {
  const int numPages = (layout == TWO_PAGE_LAYOUT ? 2 : 1);
  const auto numImages = static_cast<int>(m_images.size());
  for (int i = 0; i < numImages; ++i) {
    ImageDesc& image = m_images[i];
    image.layoutRevision = ++m_nextLayoutRevision;

    int adjustedNumPages = numPages;
    if ((numPages == 2) && (image.leftHalfRemoved != image.rightHalfRemoved)) {
      // Both can't be removed, but we handle that case anyway
      // by treating it like none are removed.
      --adjustedNumPages;
    }

    const int delta = adjustedNumPages - image.numLogicalPages;
    if (delta == 0) {
      continue;
    }

    image.numLogicalPages = adjustedNumPages;
    *modified = true;
  }
}

void ProjectPages::autoSetLayoutTypeForImpl(const ImageId& imageId, const OrthogonalRotation rotation, bool* modified) {
  const auto numImages = static_cast<int>(m_images.size());
  for (int i = 0; i < numImages; ++i) {
    ImageDesc& image = m_images[i];
    if (image.id == imageId) {
      image.layoutRevision = ++m_nextLayoutRevision;
      int numPages = adviseNumberOfLogicalPages(image.metadata, rotation);
      if ((numPages == 2) && (image.leftHalfRemoved != image.rightHalfRemoved)) {
        // Both can't be removed, but we handle that case anyway
        // by treating it like none are removed.
        --numPages;
      }

      const int delta = numPages - image.numLogicalPages;
      if (delta == 0) {
        break;
      }

      image.numLogicalPages = numPages;

      *modified = true;
      break;
    }
  }
}

void ProjectPages::updateImageMetadataImpl(const ImageId& imageId, const ImageMetadata& metadata, bool* modified) {
  const auto numImages = static_cast<int>(m_images.size());
  for (int i = 0; i < numImages; ++i) {
    ImageDesc& image = m_images[i];
    if (image.id == imageId) {
      image.layoutRevision = ++m_nextLayoutRevision;
      if (image.metadata != metadata) {
        image.metadata = metadata;
        *modified = true;
      }
      break;
    }
  }
}

std::vector<PageInfo> ProjectPages::insertImageImpl(const ImageInfo& newImage,
                                                    BeforeOrAfter beforeOrAfter,
                                                    const ImageId& existing,
                                                    const PageView view,
                                                    bool& modified) {
  std::vector<PageInfo> logicalPages;

  auto it(m_images.begin());
  const auto end(m_images.end());
  for (; it != end && it->id != existing; ++it) {
    // Continue until we find the existing image.
  }
  if (it == end) {
    // Existing image not found.
    if (!((beforeOrAfter == BEFORE) && existing.isNull())) {
      return logicalPages;
    }  // Otherwise we can still handle that case.
  }
  if (beforeOrAfter == AFTER) {
    ++it;
  }

  ImageDesc imageDesc(newImage);
  imageDesc.layoutRevision = ++m_nextLayoutRevision;

  // Enforce some rules.
  if (imageDesc.numLogicalPages == 2) {
    imageDesc.leftHalfRemoved = false;
    imageDesc.rightHalfRemoved = false;
  } else if (imageDesc.numLogicalPages != 1) {
    return logicalPages;
  } else if (imageDesc.leftHalfRemoved && imageDesc.rightHalfRemoved) {
    imageDesc.leftHalfRemoved = false;
    imageDesc.rightHalfRemoved = false;
  }

  m_images.insert(it, imageDesc);

  PageInfo pageInfoTempl(PageId(newImage.id(), PageId::SINGLE_PAGE), imageDesc.metadata, imageDesc.numLogicalPages,
                         imageDesc.leftHalfRemoved, imageDesc.rightHalfRemoved);

  if ((view == IMAGE_VIEW)
      || ((imageDesc.numLogicalPages == 1) && (imageDesc.leftHalfRemoved == imageDesc.rightHalfRemoved))) {
    logicalPages.push_back(pageInfoTempl);
  } else {
    if ((imageDesc.numLogicalPages == 2) || ((imageDesc.numLogicalPages == 1) && imageDesc.rightHalfRemoved)) {
      pageInfoTempl.setId(PageId(newImage.id(), imageDesc.logicalPageToSubPage(0, m_layoutDirection)));
      logicalPages.push_back(pageInfoTempl);
    }
    if ((imageDesc.numLogicalPages == 2) || ((imageDesc.numLogicalPages == 1) && imageDesc.leftHalfRemoved)) {
      pageInfoTempl.setId(PageId(newImage.id(), imageDesc.logicalPageToSubPage(1, m_layoutDirection)));
      logicalPages.push_back(pageInfoTempl);
    }
  }
  return logicalPages;
}  // ProjectPages::insertImageImpl

void ProjectPages::removePagesImpl(const std::set<PageId>& toRemove, bool& modified) {
  const auto toRemoveEnd(toRemove.end());

  std::vector<ImageDesc> newImages;
  newImages.reserve(m_images.size());
  int newTotalLogicalPages = 0;

  const auto numOldImages = static_cast<int>(m_images.size());
  for (int i = 0; i < numOldImages; ++i) {
    ImageDesc image(m_images[i]);
    if (toRemove.find(PageId(image.id, PageId::SINGLE_PAGE)) != toRemoveEnd
        || toRemove.find(PageId(image.id, PageId::LEFT_PAGE)) != toRemoveEnd
        || toRemove.find(PageId(image.id, PageId::RIGHT_PAGE)) != toRemoveEnd) {
      image.layoutRevision = ++m_nextLayoutRevision;
    }

    if (toRemove.find(PageId(image.id, PageId::SINGLE_PAGE)) != toRemoveEnd) {
      image.numLogicalPages = 0;
      modified = true;
    } else {
      if (toRemove.find(PageId(image.id, PageId::LEFT_PAGE)) != toRemoveEnd) {
        image.leftHalfRemoved = true;
        --image.numLogicalPages;
        modified = true;
      }
      if (toRemove.find(PageId(image.id, PageId::RIGHT_PAGE)) != toRemoveEnd) {
        image.rightHalfRemoved = true;
        --image.numLogicalPages;
        modified = true;
      }
    }

    if (image.numLogicalPages > 0) {
      newImages.push_back(image);
      newTotalLogicalPages += newImages.back().numLogicalPages;
    }
  }

  newImages.swap(m_images);
}  // ProjectPages::removePagesImpl

PageInfo ProjectPages::unremovePageImpl(const PageId& pageId, bool& modified) {
  if (pageId.subPage() == PageId::SINGLE_PAGE) {
    // These can't be unremoved.
    return PageInfo();
  }

  auto it(m_images.begin());
  const auto end(m_images.end());
  for (; it != end && it->id != pageId.imageId(); ++it) {
    // Continue until we find the corresponding image.
  }
  if (it == end) {
    // The corresponding image wasn't found.
    return PageInfo();
  }

  ImageDesc& image = *it;

  if (image.numLogicalPages != 1) {
    return PageInfo();
  }

  if ((pageId.subPage() == PageId::LEFT_PAGE) && image.leftHalfRemoved) {
    image.leftHalfRemoved = false;
  } else if ((pageId.subPage() == PageId::RIGHT_PAGE) && image.rightHalfRemoved) {
    image.rightHalfRemoved = false;
  } else {
    return PageInfo();
  }

  image.numLogicalPages = 2;
  image.layoutRevision = ++m_nextLayoutRevision;
  return PageInfo(pageId, image.metadata, image.numLogicalPages, image.leftHalfRemoved, image.rightHalfRemoved);
}  // ProjectPages::unremovePageImpl

/*========================= ProjectPages::ImageDesc ======================*/

ProjectPages::ImageDesc::ImageDesc(const ImageInfo& imageInfo)
    : id(imageInfo.id()),
      metadata(imageInfo.metadata()),
      numLogicalPages(imageInfo.numSubPages()),
      leftHalfRemoved(imageInfo.leftHalfRemoved()),
      rightHalfRemoved(imageInfo.rightHalfRemoved()) {}

ProjectPages::ImageDesc::ImageDesc(const ImageId& id, const ImageMetadata& metadata, const Pages pages)
    : id(id), metadata(metadata), leftHalfRemoved(false), rightHalfRemoved(false) {
  switch (pages) {
    case ONE_PAGE:
      numLogicalPages = 1;
      break;
    case TWO_PAGES:
      numLogicalPages = 2;
      break;
    case AUTO_PAGES:
      numLogicalPages = adviseNumberOfLogicalPages(metadata, OrthogonalRotation());
      break;
  }
}

PageId::SubPage ProjectPages::ImageDesc::logicalPageToSubPage(const int logicalPage,
                                                              const Qt::LayoutDirection defaultDirection) const {
  assert(numLogicalPages >= 1 && numLogicalPages <= 2);
  assert(logicalPage >= 0 && logicalPage < numLogicalPages);

  if (numLogicalPages == 1) {
    if (leftHalfRemoved && !rightHalfRemoved) {
      return PageId::RIGHT_PAGE;
    } else if (rightHalfRemoved && !leftHalfRemoved) {
      return PageId::LEFT_PAGE;
    } else {
      return PageId::SINGLE_PAGE;
    }
  } else {
    const Qt::LayoutDirection direction = layoutDirectionOverride.value_or(defaultDirection);
    const bool leftPage = (logicalPage == 0) == (direction == Qt::LeftToRight);
    return leftPage ? PageId::LEFT_PAGE : PageId::RIGHT_PAGE;
  }
}
