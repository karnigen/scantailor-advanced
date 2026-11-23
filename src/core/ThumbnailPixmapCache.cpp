// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ThumbnailPixmapCache.h"

#include <GrayImage.h>
#include <Scale.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QThread>
#include <algorithm>
#include <atomic>
#include <exception>
#include <list>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "AtomicFileOverwriter.h"
#include "ImageId.h"
#include "ImageLoader.h"
#include "OutOfMemoryHandler.h"
#include "RelinkablePath.h"

using namespace imageproc;

namespace {

void handleOutOfMemoryNoThrow() {
  try {
    OutOfMemoryHandler::instance().handleOutOfMemorySituation();
  } catch (const std::exception& e) {
    qCritical("Out-of-memory handler failed: %s", e.what());
  } catch (...) {
    qCritical("Out-of-memory handler failed with an unknown exception.");
  }
}

}  // namespace

class ThumbnailPixmapCache::Item {
 public:
  using Queue = std::list<Item*>;

  enum Status {
    /**
     * The background threaed hasn't touched it yet.
     */
    QUEUED,

    /**
     * The image is currently being loaded by a background
     * thread, or it has been loaded, but the main thread
     * hasn't yet received the loaded image, or it's currently
     * converting it to a pixmap.
     */
    IN_PROGRESS,

    /**
     * The image was loaded and then converted to a pixmap
     * by the main thread.
     */
    LOADED,

    /**
     * The image could not be loaded.
     */
    LOAD_FAILED
  };

  ImageId imageId;

  mutable QPixmap pixmap;

  /**< Guaranteed to be set if status is LOADED */

  mutable std::vector<std::weak_ptr<CompletionHandler>> completionHandlers;
  mutable bool hasPendingLoadResult = false;
  mutable QImage pendingLoadResultImage;
  mutable ThumbnailLoadResult::Status pendingLoadResultStatus = ThumbnailLoadResult::LOAD_FAILED;

  /**
   * The total image loading attempts (of any images) by
   * ThumbnailPixmapCache at the time of the creation of this item.
   * This information is used for request expiration.
   * \see ThumbnailLoadResult::REQUEST_EXPIRED
   */
  int precedingLoadAttempts;

  mutable Status status;

  Queue::iterator loadQueueIterator;
  Queue::iterator removeQueueIterator;

  Item(const ImageId& imageId, int precedingLoadAttempts, Status st);

  Item(const Item& other) = delete;

 private:
  Item& operator=(const Item& other) = delete;  // Assignment is forbidden.
};


class ThumbnailPixmapCache::Impl : public QThread {
 public:
  Impl(const QString& thumbDir, const QSize& maxThumbSize, int maxCachedPixmaps, int expirationThreshold);

  ~Impl() override;

  void setThumbDir(const QString& thumbDir);

  const QSize& getMaxThumbSize() const;

  void setMaxThumbSize(const QSize& maxSize);

  Status request(const ImageId& imageId,
                 QPixmap& pixmap,
                 bool loadNow = false,
                 const std::weak_ptr<CompletionHandler>* completionHandler = nullptr);

  void ensureThumbnailExists(const ImageId& imageId, const QImage& image);

  void recreateThumbnail(const ImageId& imageId, const QImage& image);

 protected:
  void run() override;

  void customEvent(QEvent* e) override;

 private:
  class LoadResultEvent;
  using ItemsByKey = std::unordered_map<ImageId, std::unique_ptr<Item>>;
  using LoadQueue = Item::Queue;
  using RemoveQueue = Item::Queue;

  class BackgroundLoader : public QObject {
   public:
    explicit BackgroundLoader(Impl& owner);

   protected:
    void customEvent(QEvent* e) override;

   private:
    Impl& m_owner;
  };


  void backgroundProcessing();

  static QImage loadSaveThumbnail(const ImageId& imageId, const QString& thumbDir, const QSize& maxThumbSize);

  static QString getThumbFilePath(const ImageId& imageId, const QString& thumbDir, const QSize& maxThumbSize);

  static QImage makeThumbnail(const QImage& image, const QSize& maxThumbSize);

  void queuedToInProgress(const LoadQueue::iterator& lqIt);

  bool postLoadResult(const LoadQueue::iterator& lqIt,
                      const QImage& image,
                      ThumbnailLoadResult::Status status,
                      bool reportFailure = true);

  void deferLoadResult(const LoadQueue::iterator& lqIt, const QImage& image, ThumbnailLoadResult::Status status);

  void processLoadResult(LoadResultEvent* result);

  void removeExcessLocked();

  Item* insertItemLocked(const ImageId& imageId,
                         int precedingLoadAttempts,
                         Item::Status status,
                         LoadQueue::const_iterator loadPosition,
                         RemoveQueue::const_iterator removePosition);

  void removeItemLocked(const RemoveQueue::iterator& it);

  void removeLoadedItemsLocked();

  void cachePixmapUnlocked(const ImageId& imageId, const QPixmap& pixmap);

  bool cachePixmapLocked(const ImageId& imageId, const QPixmap& pixmap, LoadQueue::iterator& resultIterator);

  mutable QMutex m_mutex;
  BackgroundLoader m_backgroundLoader;
  ItemsByKey m_itemsByKey; /**< ImageId => Item mapping */

  /**
   * An "std::list"-like view of QUEUED items in the order they are
   * going to be loaded.  Actually the list contains all kinds of items,
   * but all QUEUED ones precede any others.  New QUEUED items are added
   * to the front of this list for purposes of request expiration.
   * \see ThumbnailLoadResult::REQUEST_EXPIRED
   */
  LoadQueue m_loadQueue;

  /**
   * An "std::list"-like view of LOADED items in the order they are
   * going to be removed. Actually the list contains all kinds of items,
   * but all LOADED ones precede any others.  Note that we don't bother
   * removing items without a pixmap, which would be all except LOADED
   * items.  New LOADED items are added after the last LOADED item
   * already present in the list.
   */
  RemoveQueue m_removeQueue;

  /**
   * An iterator of m_removeQueue that marks the end of LOADED items.
   */
  RemoveQueue::iterator m_endOfLoadedItems;

  QString m_thumbDir;
  QSize m_maxThumbSize;
  int m_maxCachedPixmaps;

  /**
   * \see ThumbnailPixmapCache::ThumbnailPixmapCache()
   */
  int m_expirationThreshold;

  int m_numQueuedItems;
  int m_numLoadedItems;

  /**
   * Total image loading attempts so far.  Used for request expiration.
   * \see ThumbnailLoadResult::REQUEST_EXPIRED
   */
  int m_totalLoadAttempts;

  bool m_threadStarted;
  std::atomic_bool m_shuttingDown;
};


class ThumbnailPixmapCache::Impl::LoadResultEvent : public QEvent {
 public:
  LoadResultEvent(const Impl::LoadQueue::iterator& lqIt, const QImage& image, ThumbnailLoadResult::Status status);

  ~LoadResultEvent() override;

  Impl::LoadQueue::iterator lqIter() const { return m_lqIter; }

  const QImage& image() const { return m_image; }

  void releaseImage() { m_image = QImage(); }

  ThumbnailLoadResult::Status status() const { return m_status; }

 private:
  Impl::LoadQueue::iterator m_lqIter;
  QImage m_image;
  ThumbnailLoadResult::Status m_status;
};


/*========================== ThumbnailPixmapCache ===========================*/

ThumbnailPixmapCache::ThumbnailPixmapCache(const QString& thumbDir,
                                           const QSize& maxThumbSize,
                                           const int maxCachedPixmaps,
                                           const int expirationThreshold)
    : m_impl(std::make_unique<Impl>(RelinkablePath::normalize(thumbDir),
                                    maxThumbSize,
                                    maxCachedPixmaps,
                                    expirationThreshold)) {}

ThumbnailPixmapCache::~ThumbnailPixmapCache() = default;

void ThumbnailPixmapCache::setThumbDir(const QString& thumbDir) {
  m_impl->setThumbDir(RelinkablePath::normalize(thumbDir));
}

ThumbnailPixmapCache::Status ThumbnailPixmapCache::loadFromCache(const ImageId& imageId, QPixmap& pixmap) {
  return m_impl->request(imageId, pixmap);
}

ThumbnailPixmapCache::Status ThumbnailPixmapCache::loadNow(const ImageId& imageId, QPixmap& pixmap) {
  return m_impl->request(imageId, pixmap, true);
}

ThumbnailPixmapCache::Status ThumbnailPixmapCache::loadRequest(
    const ImageId& imageId,
    QPixmap& pixmap,
    const std::weak_ptr<CompletionHandler>& completionHandler) {
  return m_impl->request(imageId, pixmap, false, &completionHandler);
}

void ThumbnailPixmapCache::ensureThumbnailExists(const ImageId& imageId, const QImage& image) {
  m_impl->ensureThumbnailExists(imageId, image);
}

void ThumbnailPixmapCache::recreateThumbnail(const ImageId& imageId, const QImage& image) {
  m_impl->recreateThumbnail(imageId, image);
}

void ThumbnailPixmapCache::setMaxThumbSize(const QSize& maxSize) {
  m_impl->setMaxThumbSize(maxSize);
}

const QSize& ThumbnailPixmapCache::getMaxThumbSize() const {
  return m_impl->getMaxThumbSize();
}

/*======================= ThumbnailPixmapCache::Impl ========================*/

ThumbnailPixmapCache::Impl::Impl(const QString& thumbDir,
                                 const QSize& maxThumbSize,
                                 const int maxCachedPixmaps,
                                 const int expirationThreshold)
    : m_backgroundLoader(*this),
      m_itemsByKey(),
      m_loadQueue(),
      m_removeQueue(),
      m_endOfLoadedItems(m_removeQueue.end()),
      m_thumbDir(thumbDir),
      m_maxThumbSize(maxThumbSize),
      m_maxCachedPixmaps(maxCachedPixmaps),
      m_expirationThreshold(expirationThreshold),
      m_numQueuedItems(0),
      m_numLoadedItems(0),
      m_totalLoadAttempts(0),
      m_threadStarted(false),
      m_shuttingDown(false) {
  // Note that QDir::mkdir() will fail if the parent directory,
  // that is $OUT/cache doesn't exist. We want that behaviour,
  // as otherwise when loading a project from a different machine,
  // a whole bunch of bogus directories would be created.
  QDir().mkdir(m_thumbDir);

  m_backgroundLoader.moveToThread(this);
}

ThumbnailPixmapCache::Impl::~Impl() {
  m_shuttingDown.store(true);
  {
    const QMutexLocker locker(&m_mutex);

    if (!m_threadStarted) {
      return;
    }
  }

  quit();
  wait();
}

void ThumbnailPixmapCache::Impl::setThumbDir(const QString& thumbDir) {
  QMutexLocker locker(&m_mutex);

  if (thumbDir == m_thumbDir) {
    return;
  }

  m_thumbDir = thumbDir;

  for (const Item* item : m_loadQueue) {
    // This trick will make all queued tasks to expire.
    m_totalLoadAttempts = std::max(m_totalLoadAttempts, item->precedingLoadAttempts + m_expirationThreshold + 1);
  }
}

ThumbnailPixmapCache::Status ThumbnailPixmapCache::Impl::request(
    const ImageId& imageId,
    QPixmap& pixmap,
    const bool loadNow,
    const std::weak_ptr<CompletionHandler>* completionHandler) {
  assert(QCoreApplication::instance()->thread() == QThread::currentThread());

  QMutexLocker locker(&m_mutex);

  if (m_shuttingDown.load()) {
    return LOAD_FAILED;
  }

  const ItemsByKey::iterator kIt(m_itemsByKey.find(imageId));
  if (kIt != m_itemsByKey.end()) {
    Item& item = *kIt->second;
    if (item.status == Item::LOADED) {
      pixmap = item.pixmap;

      // Move it after all other candidates for removal.
      m_removeQueue.splice(m_endOfLoadedItems, m_removeQueue, item.removeQueueIterator);
      return LOADED;
    } else if (item.status == Item::LOAD_FAILED) {
      pixmap = item.pixmap;
      return LOAD_FAILED;
    }
  }

  if (loadNow) {
    const QString thumbDir(m_thumbDir);
    const QSize maxThumbSize(m_maxThumbSize);

    locker.unlock();

    pixmap = QPixmap::fromImage(loadSaveThumbnail(imageId, thumbDir, maxThumbSize));
    if (pixmap.isNull()) {
      return LOAD_FAILED;
    }

    cachePixmapUnlocked(imageId, pixmap);
    return LOADED;
  }

  if (!completionHandler) {
    return LOAD_FAILED;
  }

  if (kIt != m_itemsByKey.end()) {
    Item& item = *kIt->second;
    assert(item.status == Item::QUEUED || item.status == Item::IN_PROGRESS);
    item.completionHandlers.push_back(*completionHandler);

    if (item.status == Item::QUEUED) {
      // Because we've got a new request for this item,
      // we move it to the beginning of the load queue.
      // Note that we don't do it for IN_PROGRESS items,
      // because all QUEUED items must precede any other
      // items in the load queue.
      m_loadQueue.splice(m_loadQueue.begin(), m_loadQueue, item.loadQueueIterator);
    }
    return QUEUED;
  }

  // Create a new item.
  Item* item = insertItemLocked(imageId, m_totalLoadAttempts, Item::QUEUED, m_loadQueue.begin(), m_removeQueue.end());
  assert(item);
  assert(item->status == Item::QUEUED);
  assert(item->completionHandlers.empty());

  if (m_endOfLoadedItems == m_removeQueue.end()) {
    m_endOfLoadedItems = item->removeQueueIterator;
  }
  item->completionHandlers.push_back(*completionHandler);

  if (m_numQueuedItems++ == 0) {
    if (m_threadStarted) {
      // Wake the background thread up.
      QCoreApplication::postEvent(&m_backgroundLoader, new QEvent(QEvent::User));
    } else {
      // Start the background thread.
      start();
      m_threadStarted = true;
    }
  }
  return QUEUED;
}  // ThumbnailPixmapCache::Impl::request

void ThumbnailPixmapCache::Impl::ensureThumbnailExists(const ImageId& imageId, const QImage& image) {
  if (m_shuttingDown.load()) {
    return;
  }

  if (image.isNull()) {
    return;
  }

  QMutexLocker locker(&m_mutex);
  const QString thumbDir(m_thumbDir);
  const QSize maxThumbSize(m_maxThumbSize);
  locker.unlock();

  const QString thumbFilePath(getThumbFilePath(imageId, thumbDir, m_maxThumbSize));
  if (QFile::exists(thumbFilePath)) {
    return;
  }

  const QImage thumbnail(makeThumbnail(image, maxThumbSize));

  AtomicFileOverwriter overwriter;
  QIODevice* iodev = overwriter.startWriting(thumbFilePath);
  if (iodev && thumbnail.save(iodev, "PNG")) {
    overwriter.commit();
  }
}

void ThumbnailPixmapCache::Impl::recreateThumbnail(const ImageId& imageId, const QImage& image) {
  if (m_shuttingDown.load()) {
    return;
  }

  if (image.isNull()) {
    return;
  }

  QMutexLocker locker(&m_mutex);
  const QString thumbDir(m_thumbDir);
  const QSize maxThumbSize(m_maxThumbSize);
  locker.unlock();

  const QString thumbFilePath(getThumbFilePath(imageId, thumbDir, m_maxThumbSize));
  const QImage thumbnail(makeThumbnail(image, maxThumbSize));
  bool thumbWritten = false;

  // Note that we may be called from multiple threads at the same time.
  AtomicFileOverwriter overwriter;
  QIODevice* iodev = overwriter.startWriting(thumbFilePath);
  if (iodev && thumbnail.save(iodev, "PNG")) {
    thumbWritten = overwriter.commit();
  } else {
    overwriter.abort();
  }

  if (!thumbWritten) {
    return;
  }

  const QMutexLocker locker2(&m_mutex);

  const ItemsByKey::iterator kIt(m_itemsByKey.find(imageId));
  if (kIt == m_itemsByKey.end()) {
    return;
  }

  Item& item = *kIt->second;
  switch (item.status) {
    case Item::LOADED:
    case Item::LOAD_FAILED:
      removeItemLocked(item.removeQueueIterator);
      break;
    case Item::QUEUED:
      break;
    case Item::IN_PROGRESS:
      // We have a small race condition in this case.
      // We don't know if the other thread has already loaded
      // the thumbnail or not.  In case it did, again we
      // don't know if it loaded the old or new version.
      // Well, let's just pretend the thumnail was loaded
      // (or failed to load) before we wrote the new version.
      break;
  }
}  // ThumbnailPixmapCache::Impl::recreateThumbnail

void ThumbnailPixmapCache::Impl::run() {
  try {
    backgroundProcessing();
  } catch (const std::bad_alloc&) {
    handleOutOfMemoryNoThrow();
  } catch (const std::exception& e) {
    qCritical("Thumbnail background thread failed: %s", e.what());
  } catch (...) {
    qCritical("Thumbnail background thread failed with an unknown exception.");
  }
  exec();  // Wait for further processing requests (via custom events).
}

void ThumbnailPixmapCache::Impl::customEvent(QEvent* e) {
  processLoadResult(dynamic_cast<LoadResultEvent*>(e));
}

void ThumbnailPixmapCache::Impl::backgroundProcessing() {
  // This method is called from a background thread.
  assert(QCoreApplication::instance()->thread() != QThread::currentThread());

  while (true) {
    LoadQueue::iterator lqIt;
    bool loadInProgress = false;
    bool requestExpired = false;
    bool pendingResult = false;
    QImage pendingImage;
    ThumbnailLoadResult::Status pendingStatus = ThumbnailLoadResult::LOAD_FAILED;
    try {
      // We are going to initialize these while holding the mutex.
      ImageId imageId;
      QString thumbDir;
      QSize maxThumbSize;

      {
        const QMutexLocker locker(&m_mutex);

        if (m_shuttingDown.load() || m_itemsByKey.empty()) {
          break;
        }

        if (m_numQueuedItems > 0) {
          lqIt = m_loadQueue.begin();
          assert((*lqIt)->status == Item::QUEUED);
          imageId = (*lqIt)->imageId;

          // By marking the item as IN_PROGRESS, we prevent it
          // from being processed again before the GUI thread
          // receives our LoadResultEvent.
          queuedToInProgress(lqIt);
          loadInProgress = true;

          if (m_totalLoadAttempts - (*lqIt)->precedingLoadAttempts > m_expirationThreshold) {
            // Expire this request.  The reasoning behind
            // request expiration is described in
            // ThumbnailLoadResult::REQUEST_EXPIRED
            // documentation.
            requestExpired = true;
          } else {
            // Expired requests don't count as load attempts.
            ++m_totalLoadAttempts;

            // Copy those while holding the mutex.
            thumbDir = m_thumbDir;
            maxThumbSize = m_maxThumbSize;
          }
        } else {
          const auto pendingIt = std::find_if(m_loadQueue.begin(), m_loadQueue.end(),
                                              [](const Item* item) { return item->hasPendingLoadResult; });
          if (pendingIt == m_loadQueue.end()) {
            break;
          }

          lqIt = pendingIt;
          Item& item = **lqIt;
          pendingImage = item.pendingLoadResultImage;
          pendingStatus = item.pendingLoadResultStatus;
          item.pendingLoadResultImage = QImage();
          item.hasPendingLoadResult = false;
          pendingResult = true;
        }
      }  // mutex scope

      if (pendingResult) {
        if (!postLoadResult(lqIt, pendingImage, pendingStatus, false)) {
          deferLoadResult(lqIt, pendingImage, pendingStatus);

          bool queuedWork = false;
          {
            const QMutexLocker locker(&m_mutex);
            queuedWork = m_numQueuedItems > 0;
          }
          if (!queuedWork && !m_shuttingDown.load()) {
            QThread::msleep(250);
          }
        }
        continue;
      }

      if (requestExpired) {
        const QImage noImage;
        if (!postLoadResult(lqIt, noImage, ThumbnailLoadResult::REQUEST_EXPIRED)) {
          deferLoadResult(lqIt, noImage, ThumbnailLoadResult::REQUEST_EXPIRED);
        }
        continue;
      }

      const QImage image(loadSaveThumbnail(imageId, thumbDir, maxThumbSize));

      const ThumbnailLoadResult::Status status
          = image.isNull() ? ThumbnailLoadResult::LOAD_FAILED : ThumbnailLoadResult::LOADED;
      if (!postLoadResult(lqIt, image, status)) {
        deferLoadResult(lqIt, image, status);
      }
    } catch (const std::bad_alloc&) {
      handleOutOfMemoryNoThrow();
      if (!loadInProgress) {
        break;
      }
      const QImage noImage;
      if (!postLoadResult(lqIt, noImage, ThumbnailLoadResult::LOAD_FAILED)) {
        deferLoadResult(lqIt, noImage, ThumbnailLoadResult::LOAD_FAILED);
      }
    } catch (const std::exception& e) {
      qWarning("Thumbnail loading failed: %s", e.what());
      if (!loadInProgress) {
        break;
      }
      const QImage noImage;
      if (!postLoadResult(lqIt, noImage, ThumbnailLoadResult::LOAD_FAILED)) {
        deferLoadResult(lqIt, noImage, ThumbnailLoadResult::LOAD_FAILED);
      }
    } catch (...) {
      qWarning("Thumbnail loading failed with an unknown exception.");
      if (!loadInProgress) {
        break;
      }
      const QImage noImage;
      if (!postLoadResult(lqIt, noImage, ThumbnailLoadResult::LOAD_FAILED)) {
        deferLoadResult(lqIt, noImage, ThumbnailLoadResult::LOAD_FAILED);
      }
    }
  }
}  // ThumbnailPixmapCache::Impl::backgroundProcessing

QImage ThumbnailPixmapCache::Impl::loadSaveThumbnail(const ImageId& imageId,
                                                     const QString& thumbDir,
                                                     const QSize& maxThumbSize) {
  const QString thumbFilePath(getThumbFilePath(imageId, thumbDir, maxThumbSize));

  QImage image(ImageLoader::load(thumbFilePath, 0));
  if (!image.isNull()) {
    return image;
  }

  image = ImageLoader::load(imageId);
  if (image.isNull()) {
    return QImage();
  }

  const QImage thumbnail(makeThumbnail(image, maxThumbSize));
  thumbnail.save(thumbFilePath, "PNG");
  return thumbnail;
}

QString ThumbnailPixmapCache::Impl::getThumbFilePath(const ImageId& imageId,
                                                     const QString& thumbDir,
                                                     const QSize& maxThumbSize) {
  // Because a project may have several files with the same name (from
  // different directories), we add a hash of the original image path
  // to the thumbnail file name.
  const QByteArray origPathHash = QCryptographicHash::hash(imageId.filePath().toUtf8(), QCryptographicHash::Md5)
                                      .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
  const QString origPathHashStr = QString::fromLatin1(origPathHash.data(), origPathHash.size());
  const QString thumbnailQualityStr = QChar('q') + QString::number(maxThumbSize.width());

  const QFileInfo origImgPath(imageId.filePath());
  QString thumbFilePath(thumbDir);
  thumbFilePath += QChar('/');
  thumbFilePath += origImgPath.completeBaseName();
  thumbFilePath += QChar('_');
  thumbFilePath += QString::number(imageId.zeroBasedPage());
  thumbFilePath += QChar('_');
  thumbFilePath += origPathHashStr;
  thumbFilePath += QChar('_');
  thumbFilePath += thumbnailQualityStr;
  thumbFilePath += QString::fromLatin1(".png");
  return thumbFilePath;
}

QImage ThumbnailPixmapCache::Impl::makeThumbnail(const QImage& image, const QSize& maxThumbSize) {
  if ((image.width() < maxThumbSize.width()) && (image.height() < maxThumbSize.height())) {
    return image;
  }

  QSize toSize(image.size());
  toSize.scale(maxThumbSize, Qt::KeepAspectRatio);

  if ((image.format() == QImage::Format_Indexed8) && image.isGrayscale()) {
    // This will be faster than QImage::scale().
    return scaleToGray(GrayImage(image), toSize);
  }
  return image.scaled(toSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void ThumbnailPixmapCache::Impl::queuedToInProgress(const LoadQueue::iterator& lqIt) {
  Item& item = **lqIt;
  assert(item.status == Item::QUEUED);
  item.status = Item::IN_PROGRESS;

  assert(m_numQueuedItems > 0);
  --m_numQueuedItems;

  // Move it item to the end of load queue.
  // The point is to keep QUEUED items before any others.
  m_loadQueue.splice(m_loadQueue.end(), m_loadQueue, lqIt);

  // Going from QUEUED to IN_PROGRESS doesn't require
  // moving it in the remove queue, as we only remove
  // LOADED items.
}

bool ThumbnailPixmapCache::Impl::postLoadResult(const LoadQueue::iterator& lqIt,
                                                const QImage& image,
                                                const ThumbnailLoadResult::Status status,
                                                const bool reportFailure) {
  if (m_shuttingDown.load()) {
    return false;
  }

  try {
    auto event = std::make_unique<LoadResultEvent>(lqIt, image, status);
    QCoreApplication::postEvent(this, event.release());
    return true;
  } catch (const std::bad_alloc&) {
    if (reportFailure) {
      qWarning("Failed to post a thumbnail result because memory is exhausted; it will be retried.");
      handleOutOfMemoryNoThrow();
    }
  } catch (const std::exception& e) {
    if (reportFailure) {
      qWarning("Failed to post a thumbnail result: %s; it will be retried.", e.what());
    }
  } catch (...) {
    if (reportFailure) {
      qWarning("Failed to post a thumbnail result with an unknown exception; it will be retried.");
    }
  }
  return false;
}

void ThumbnailPixmapCache::Impl::deferLoadResult(const LoadQueue::iterator& lqIt,
                                                 const QImage& image,
                                                 const ThumbnailLoadResult::Status status) {
  const QMutexLocker locker(&m_mutex);
  if (m_shuttingDown.load()) {
    return;
  }

  Item& item = **lqIt;
  if (item.status != Item::IN_PROGRESS) {
    return;
  }
  item.pendingLoadResultImage = image;
  item.pendingLoadResultStatus = status;
  item.hasPendingLoadResult = true;
}

void ThumbnailPixmapCache::Impl::processLoadResult(LoadResultEvent* result) {
  assert(QCoreApplication::instance()->thread() == QThread::currentThread());

  ThumbnailLoadResult::Status status = result->status();
  QPixmap pixmap;
  try {
    pixmap = QPixmap::fromImage(result->image());
  } catch (const std::bad_alloc&) {
    handleOutOfMemoryNoThrow();
    status = ThumbnailLoadResult::LOAD_FAILED;
  } catch (const std::exception& e) {
    qWarning("Failed to create a thumbnail pixmap: %s", e.what());
    status = ThumbnailLoadResult::LOAD_FAILED;
  } catch (...) {
    qWarning("Failed to create a thumbnail pixmap: unknown exception.");
    status = ThumbnailLoadResult::LOAD_FAILED;
  }
  result->releaseImage();

  std::vector<std::weak_ptr<CompletionHandler>> completionHandlers;

  {
    const QMutexLocker locker(&m_mutex);

    if (m_shuttingDown.load()) {
      return;
    }

    const LoadQueue::iterator lqIt(result->lqIter());
    Item& item = **lqIt;
    const RemoveQueue::iterator rqIt(item.removeQueueIterator);

    if ((status == ThumbnailLoadResult::LOADED) && pixmap.isNull()) {
      // That's a special case caused by cachePixmapLocked().
      assert(!item.pixmap.isNull());
    } else {
      item.pixmap = pixmap;
    }
    item.completionHandlers.swap(completionHandlers);

    if (status == ThumbnailLoadResult::LOADED) {
      // Maybe remove an older item.
      removeExcessLocked();

      item.status = Item::LOADED;
      ++m_numLoadedItems;

      // Move this item after all other LOADED items in
      // the remove queue.
      m_removeQueue.splice(m_endOfLoadedItems, m_removeQueue, rqIt);

      // Move to the end of load queue.
      m_loadQueue.splice(m_loadQueue.end(), m_loadQueue, lqIt);
    } else if (status == ThumbnailLoadResult::LOAD_FAILED) {
      // We keep items that failed to load, as they are cheap
      // to keep and helps us avoid trying to load them
      // again and again.

      item.status = Item::LOAD_FAILED;

      // Move to the end of load queue.
      m_loadQueue.splice(m_loadQueue.end(), m_loadQueue, lqIt);
    } else {
      assert(result->status() == ThumbnailLoadResult::REQUEST_EXPIRED);

      // Just remove it.
      removeItemLocked(rqIt);
    }
  }  // mutex scope
  // Notify listeners.
  const ThumbnailLoadResult loadResult(status, pixmap);
  using WeakHandler = std::weak_ptr<CompletionHandler>;
  for (const WeakHandler& wh : completionHandlers) {
    const std::shared_ptr<CompletionHandler> sh(wh.lock());
    if (sh) {
      try {
        (*sh)(loadResult);
      } catch (const std::bad_alloc&) {
        handleOutOfMemoryNoThrow();
      } catch (const std::exception& e) {
        qWarning("Thumbnail completion handler failed: %s", e.what());
      } catch (...) {
        qWarning("Thumbnail completion handler failed with an unknown exception.");
      }
    }
  }
}  // ThumbnailPixmapCache::Impl::processLoadResult

void ThumbnailPixmapCache::Impl::removeExcessLocked() {
  if (m_numLoadedItems >= m_maxCachedPixmaps) {
    assert(m_numLoadedItems > 0);
    assert(!m_removeQueue.empty());
    assert(m_removeQueue.front()->status == Item::LOADED);
    removeItemLocked(m_removeQueue.begin());
  }
}

ThumbnailPixmapCache::Item* ThumbnailPixmapCache::Impl::insertItemLocked(
    const ImageId& imageId,
    const int precedingLoadAttempts,
    const Item::Status status,
    const LoadQueue::const_iterator loadPosition,
    const RemoveQueue::const_iterator removePosition) {
  auto item = std::make_unique<Item>(imageId, precedingLoadAttempts, status);
  Item* itemPtr = item.get();
  const auto [keyIt, inserted] = m_itemsByKey.emplace(imageId, std::move(item));
  if (!inserted) {
    throw std::logic_error("ThumbnailPixmapCache attempted to insert a duplicate ImageId.");
  }

  LoadQueue::iterator loadIt;
  bool insertedIntoLoadQueue = false;
  try {
    loadIt = m_loadQueue.insert(loadPosition, itemPtr);
    itemPtr->loadQueueIterator = loadIt;
    insertedIntoLoadQueue = true;
    itemPtr->removeQueueIterator = m_removeQueue.insert(removePosition, itemPtr);
  } catch (...) {
    if (insertedIntoLoadQueue) {
      m_loadQueue.erase(loadIt);
    }
    m_itemsByKey.erase(keyIt);
    throw;
  }
  return itemPtr;
}

void ThumbnailPixmapCache::Impl::removeItemLocked(const RemoveQueue::iterator& it) {
  Item* item = *it;
  const auto keyIt = m_itemsByKey.find(item->imageId);
  assert(keyIt != m_itemsByKey.end());
  assert(keyIt->second.get() == item);

  switch (item->status) {
    case Item::QUEUED:
      assert(m_numQueuedItems > 0);
      --m_numQueuedItems;
      break;
    case Item::LOADED:
      assert(m_numLoadedItems > 0);
      --m_numLoadedItems;
      break;
    default:;
  }

  if (m_endOfLoadedItems == it) {
    ++m_endOfLoadedItems;
  }

  m_loadQueue.erase(item->loadQueueIterator);
  m_removeQueue.erase(it);
  m_itemsByKey.erase(keyIt);
}

void ThumbnailPixmapCache::Impl::removeLoadedItemsLocked() {
  if (m_numLoadedItems == 0) {
    return;
  }

  for (auto it = m_removeQueue.begin(); it != m_removeQueue.end();) {
    const auto current = it++;
    if ((*current)->status == Item::LOADED) {
      removeItemLocked(current);
    } else {
      continue;
    }
  }

  m_numLoadedItems = 0;
  m_endOfLoadedItems = m_removeQueue.end();
}

void ThumbnailPixmapCache::Impl::cachePixmapUnlocked(const ImageId& imageId, const QPixmap& pixmap) {
  LoadQueue::iterator resultIterator;
  bool needsResultPost = false;
  {
    const QMutexLocker locker(&m_mutex);
    needsResultPost = cachePixmapLocked(imageId, pixmap, resultIterator);
  }

  if (needsResultPost && !postLoadResult(resultIterator, QImage(), ThumbnailLoadResult::LOADED)) {
    LoadResultEvent fallback(resultIterator, QImage(), ThumbnailLoadResult::LOADED);
    processLoadResult(&fallback);
  }
}

bool ThumbnailPixmapCache::Impl::cachePixmapLocked(const ImageId& imageId,
                                                   const QPixmap& pixmap,
                                                   LoadQueue::iterator& resultIterator) {
  if (m_shuttingDown.load()) {
    return false;
  }

  const Item::Status newStatus = pixmap.isNull() ? Item::LOAD_FAILED : Item::LOADED;

  // Check if such item already exists.
  const ItemsByKey::iterator kIt(m_itemsByKey.find(imageId));
  if (kIt == m_itemsByKey.end()) {
    // Existing item not found.

    // Maybe remove an older item.
    removeExcessLocked();

    // Insert our new item.
    Item* item = insertItemLocked(imageId, m_totalLoadAttempts, newStatus, m_loadQueue.end(), m_endOfLoadedItems);
    // Our new item is now after all LOADED items in the
    // remove queue and at the end of the load queue.
    if (newStatus == Item::LOAD_FAILED) {
      --m_endOfLoadedItems;
    }

    item->pixmap = pixmap;

    assert(item->completionHandlers.empty());
    return false;
  }

  Item& item = *kIt->second;
  switch (item.status) {
    case Item::LOADED:
      // There is no point in replacing LOADED items.
    case Item::IN_PROGRESS:
      // It's unsafe to touch IN_PROGRESS items.
      return false;
    default:
      break;
  }

  if ((newStatus == Item::LOADED) && (item.status == Item::QUEUED)) {
    // Not so fast.  We can't go from QUEUED to LOADED directly.
    // Well, maybe we can, but we'd have to invoke the completion
    // handlers right now.  We'd rather do it asynchronously,
    // so let's transition it to IN_PROGRESS and send
    // a LoadResultEvent asynchronously.

    assert(!item.completionHandlers.empty());

    const LoadQueue::iterator lqIt(item.loadQueueIterator);

    item.pixmap = pixmap;
    queuedToInProgress(lqIt);
    resultIterator = lqIt;
    return true;
  }

  assert(item.status == Item::LOAD_FAILED);

  item.status = newStatus;
  item.pixmap = pixmap;

  if (newStatus == Item::LOADED) {
    m_removeQueue.splice(m_endOfLoadedItems, m_removeQueue, item.removeQueueIterator);
    ++m_numLoadedItems;
  }
  return false;
}  // ThumbnailPixmapCache::Impl::cachePixmapLocked

const QSize& ThumbnailPixmapCache::Impl::getMaxThumbSize() const {
  return m_maxThumbSize;
}

void ThumbnailPixmapCache::Impl::setMaxThumbSize(const QSize& maxSize) {
  const QMutexLocker locker(&m_mutex);

  removeLoadedItemsLocked();
  m_maxThumbSize = maxSize;
}

/*====================== ThumbnailPixmapCache::Item =========================*/

ThumbnailPixmapCache::Item::Item(const ImageId& imageId, const int precedingLoadAttempts, const Status st)
    : imageId(imageId), precedingLoadAttempts(precedingLoadAttempts), status(st) {}

/*=============== ThumbnailPixmapCache::Impl::LoadResultEvent ===============*/

ThumbnailPixmapCache::Impl::LoadResultEvent::LoadResultEvent(const Impl::LoadQueue::iterator& lqIt,
                                                             const QImage& image,
                                                             const ThumbnailLoadResult::Status status)
    : QEvent(QEvent::User), m_lqIter(lqIt), m_image(image), m_status(status) {}

ThumbnailPixmapCache::Impl::LoadResultEvent::~LoadResultEvent() = default;

/*================== ThumbnailPixmapCache::BackgroundLoader =================*/

ThumbnailPixmapCache::Impl::BackgroundLoader::BackgroundLoader(Impl& owner) : m_owner(owner) {}

void ThumbnailPixmapCache::Impl::BackgroundLoader::customEvent(QEvent*) {
  m_owner.backgroundProcessing();
}
