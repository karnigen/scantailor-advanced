// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "RelinkingModel.h"

#include <core/IconProvider.h>
#include <core/PathUtils.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#include <QWaitCondition>

#include <filesystem>
#include <list>
#include <unordered_map>
#include <utility>

#include "OutOfMemoryHandler.h"
#include "PayloadEvent.h"

class RelinkingModel::StatusUpdateResponse {
 public:
  StatusUpdateResponse(const std::filesystem::path& path, int row, Status status)
      : m_path(path), m_row(row), m_status(status) {}

  const std::filesystem::path& path() const { return m_path; }

  int row() const { return m_row; }

  Status status() const { return m_status; }

 private:
  std::filesystem::path m_path;
  int m_row;
  Status m_status;
};


class RelinkingModel::StatusUpdateThread : private QThread {
 public:
  explicit StatusUpdateThread(RelinkingModel* owner);

  /** This will signal the thread to stop and wait for it to happen. */
  ~StatusUpdateThread() override;

  /**
   * Requests are served from last to first.
   * Requesting the same item multiple times will just move the existing
   * record to the top of the stack.
   */
  void requestStatusUpdate(const std::filesystem::path& path, int row);

 private:
  struct Task {
    std::filesystem::path path;
    int row;

    Task(const std::filesystem::path& p, int r) : path(p), row(r) {}
  };

  using TaskList = std::list<Task>;
  using TasksByPath = std::unordered_map<std::filesystem::path, TaskList::iterator>;

  void run() override;

  RelinkingModel* m_owner;
  TaskList m_tasks;
  TasksByPath m_tasksByPath;
  std::filesystem::path m_pathBeingProcessed;
  QMutex m_mutex;
  QWaitCondition m_cond;
  bool m_exiting;
};

namespace {

bool isPathWithinDirectory(const QString& path, const QString& directory) {
  const QString relativePath = QDir::fromNativeSeparators(QDir(directory).relativeFilePath(path));
  return relativePath == "."
         || (!QDir::isAbsolutePath(relativePath) && relativePath != ".." && !relativePath.startsWith("../"));
}

}  // namespace


/*============================ RelinkingModel =============================*/

RelinkingModel::RelinkingModel()
    : m_fileIcon(IconProvider::getInstance().getIcon("file").pixmap(16, 16)),
      m_folderIcon(IconProvider::getInstance().getIcon("folder").pixmap(16, 16)),
      m_relinker(std::make_shared<Relinker>()),
      m_statusUpdateThread(std::make_unique<StatusUpdateThread>(this)),
      m_haveUncommittedChanges(true) {}

RelinkingModel::~RelinkingModel() = default;

int RelinkingModel::rowCount(const QModelIndex& parent) const {
  if (!parent.isValid()) {
    return static_cast<int>(m_items.size());
  } else {
    return 0;
  }
}

QVariant RelinkingModel::data(const QModelIndex& index, int role) const {
  const Item& item = m_items[index.row()];
  const QString path(core::toQString(item.uncommittedPath));

  switch (role) {
    case TypeRole:
      return item.type;
    case UncommittedStatusRole:
      return item.uncommittedStatus;
    case EntryKindRole:
      return item.entryKind;
    case UncommittedPathRole:
      return path;
    case Qt::ToolTipRole:
      if (item.uncommittedStatus == StatusUpdateFailed) {
        return tr("Could not determine whether this path exists.");
      }
      break;
    case Qt::DisplayRole:
      if (item.entryKind == CommonImageRoot) {
        return tr("Common image root: %1").arg(QDir::toNativeSeparators(path));
      }
      if (item.entryKind == OutputDirectory) {
        const QString root = commonImageRootPath();
        if (!root.isEmpty() && isPathWithinDirectory(path, root)) {
          return tr("Output directory: %1").arg(QDir::toNativeSeparators(QDir(root).relativeFilePath(path)));
        }
        return tr("Output directory (outside common root): %1").arg(QDir::toNativeSeparators(path));
      }
      {
        const QString root = commonImageRootPath();
        if (!root.isEmpty() && isPathWithinDirectory(path, root)) {
          return QDir::toNativeSeparators(QDir(root).relativeFilePath(path));
        }
        const QString relativePath = root.isEmpty() ? QString() : QDir(root).relativeFilePath(path);
        return tr("Outside common root: %1")
            .arg(QDir::toNativeSeparators(relativePath.isEmpty() ? path : relativePath));
      }
    case Qt::DecorationRole:
      return (item.type == RelinkablePath::Dir) ? m_folderIcon : m_fileIcon;
    case Qt::ForegroundRole:
      return foregroundColorForStatus(item.uncommittedStatus);
    default:
      break;
  }
  return QVariant();
}

void RelinkingModel::addPath(const RelinkablePath& path) {
  addPath(path, ImageFile);
}

void RelinkingModel::addCommonImageRoot() {
  QStringList imagePaths;
  for (const Item& item : m_items) {
    if (item.entryKind == ImageFile) {
      imagePaths.push_back(core::toQString(item.origPath));
    }
  }

  const QString commonDirectory = findCommonImageRoot(imagePaths);
  const RelinkablePath commonRoot(core::toFileSystemPath(commonDirectory), RelinkablePath::Dir);
  if (commonDirectory.isEmpty() || QDir::cleanPath(QDir(commonDirectory).rootPath()) == commonDirectory
      || m_origPathSet.find(std::make_pair(commonRoot.normalizedPath(), CommonImageRoot)) != m_origPathSet.end()) {
    return;
  }

  addPath(commonRoot, CommonImageRoot);
}

void RelinkingModel::addOutputPath(const RelinkablePath& path) {
  addPath(path, OutputDirectory);
}

QString RelinkingModel::findCommonImageRoot(const QStringList& imagePaths) {
  QString commonDirectory;
  for (const QString& imagePath : imagePaths) {
    const QString imageDirectory = QDir::cleanPath(QFileInfo(imagePath).absolutePath());
    if (commonDirectory.isEmpty()) {
      commonDirectory = imageDirectory;
      continue;
    }

    while (!isPathWithinDirectory(imageDirectory, commonDirectory)) {
      QDir parentDirectory(commonDirectory);
      if (!parentDirectory.cdUp()) {
        return {};
      }
      const QString parentPath = QDir::cleanPath(parentDirectory.absolutePath());
      if (parentPath == commonDirectory) {
        return {};
      }
      commonDirectory = parentPath;
    }
  }

  if (commonDirectory.isEmpty() || QDir::cleanPath(QDir(commonDirectory).rootPath()) == commonDirectory) {
    return {};
  }
  return commonDirectory;
}

RelinkingModel::Summary RelinkingModel::summary() const {
  Summary result;
  const QString root = commonImageRootPath();
  for (const Item& item : m_items) {
    if (item.entryKind == CommonImageRoot) {
      continue;
    }

    if (item.uncommittedStatus == Missing) {
      ++result.missingPaths;
    } else if (item.uncommittedStatus == StatusUpdateFailed) {
      ++result.failedPaths;
    } else if (item.uncommittedStatus == StatusUpdatePending) {
      ++result.pendingPaths;
    }

    const QString path(core::toQString(item.uncommittedPath));
    if (item.entryKind == OutputDirectory) {
      result.hasOutput = true;
      result.outputInsideRoot = !root.isEmpty() && isPathWithinDirectory(path, root);
      result.outputStatus = item.uncommittedStatus;
    } else if (!root.isEmpty() && isPathWithinDirectory(path, root)) {
      ++result.imagesInsideRoot;
    } else {
      ++result.imagesOutsideRoot;
    }
  }
  return result;
}

QColor RelinkingModel::foregroundColorForStatus(const Status status) {
  const bool darkPalette = QApplication::palette().color(QPalette::Window).lightness() < 128;
  switch (status) {
    case Exists:
      return darkPalette ? QColor(129, 199, 132) : QColor(27, 94, 32);
    case Missing:
      return darkPalette ? QColor(239, 154, 154) : QColor(183, 28, 28);
    case StatusUpdatePending:
      return darkPalette ? QColor(238, 238, 238) : QColor(50, 50, 50);
    case StatusUpdateFailed:
      return darkPalette ? QColor(255, 213, 79) : QColor(109, 76, 0);
  }
  return QApplication::palette().color(QPalette::Text);
}

QString RelinkingModel::commonImageRootPath() const {
  for (const Item& item : m_items) {
    if (item.entryKind == CommonImageRoot) {
      return core::toQString(item.uncommittedPath);
    }
  }
  return {};
}

void RelinkingModel::addPath(const RelinkablePath& path, const EntryKind entryKind) {
  const std::filesystem::path& normalizedPath(path.normalizedPath());

  const std::pair<std::set<std::pair<std::filesystem::path, EntryKind>>::iterator, bool> ins(
      m_origPathSet.insert(std::make_pair(normalizedPath, entryKind)));
  if (!ins.second) {
    // Not inserted because identical path is already there.
    return;
  }

  beginInsertRows(QModelIndex(), static_cast<int>(m_items.size()), static_cast<int>(m_items.size()));
  m_items.emplace_back(path, entryKind);
  endInsertRows();

  requestStatusUpdate(index(static_cast<int>(m_items.size() - 1)));
}

void RelinkingModel::replacePrefix(const QString& prefix,
                                   const QString& replacement,
                                   const RelinkablePath::Type type,
                                   const EntryKind entryKind) {
  QString slashTerminatedPrefix(prefix);
  ensureEndsWithSlash(slashTerminatedPrefix);

  int modifiedRowspanBegin = -1;

  int row = -1;
  for (Item& item : m_items) {
    ++row;
    bool modified = false;

    const bool entryInScope = entryKind == CommonImageRoot || item.entryKind == entryKind;
    if (entryInScope) {
      QString currentPath(core::toQString(item.uncommittedPath));
      if (type == RelinkablePath::File) {
        if ((item.type == RelinkablePath::File) && (currentPath == prefix)) {
          item.uncommittedPath = core::toFileSystemPath(replacement);
          modified = true;
        }
      } else {
        assert(type == RelinkablePath::Dir);
        if (currentPath.startsWith(slashTerminatedPrefix)) {
          const int suffixLen = currentPath.length() - slashTerminatedPrefix.length() + 1;
          item.uncommittedPath = core::toFileSystemPath(replacement + currentPath.right(suffixLen));
          modified = true;
        } else if (currentPath == prefix) {
          item.uncommittedPath = core::toFileSystemPath(replacement);
          modified = true;
        }
      }
    }

    if (modified) {
      m_haveUncommittedChanges = true;
      if (modifiedRowspanBegin == -1) {
        modifiedRowspanBegin = row;
      }
      emit dataChanged(index(modifiedRowspanBegin), index(row));
      requestStatusUpdate(index(row));  // This sets item.changedStatus to StatusUpdatePending.
    } else {
      if (modifiedRowspanBegin != -1) {
        emit dataChanged(index(modifiedRowspanBegin), index(row));
        modifiedRowspanBegin = -1;
      }
    }
  }

  if (modifiedRowspanBegin != -1) {
    emit dataChanged(index(modifiedRowspanBegin), index(row));
  }
}  // RelinkingModel::replacePrefix

bool RelinkingModel::checkForMerges() const {
  std::vector<std::pair<bool, std::filesystem::path>> newPaths;
  newPaths.reserve(m_items.size());

  for (const Item& item : m_items) {
    if (item.entryKind != CommonImageRoot) {
      newPaths.emplace_back(item.entryKind == OutputDirectory, item.uncommittedPath);
    }
  }

  std::sort(newPaths.begin(), newPaths.end());
  return std::adjacent_find(newPaths.begin(), newPaths.end()) != newPaths.end();
}

void RelinkingModel::commitChanges() {
  if (!m_haveUncommittedChanges) {
    return;
  }

  Relinker newRelinker;
  int modifiedRowspanBegin = -1;

  int row = -1;
  for (Item& item : m_items) {
    ++row;

    if (item.committedPath != item.uncommittedPath) {
      item.committedPath = item.uncommittedPath;
      item.committedStatus = item.uncommittedStatus;
      if (modifiedRowspanBegin == -1) {
        modifiedRowspanBegin = row;
      }
    } else {
      if (modifiedRowspanBegin != -1) {
        emit dataChanged(index(modifiedRowspanBegin), index(row));
        modifiedRowspanBegin = -1;
      }
    }

    if (item.entryKind != CommonImageRoot && item.committedPath != item.origPath) {
      newRelinker.addMapping(item.origPath, item.committedPath);
    }
  }

  if (modifiedRowspanBegin != -1) {
    emit dataChanged(index(modifiedRowspanBegin), index(row));
  }

  m_relinker->swap(newRelinker);
  m_haveUncommittedChanges = false;
}  // RelinkingModel::commitChanges

void RelinkingModel::rollbackChanges() {
  if (!m_haveUncommittedChanges) {
    return;
  }

  int modifiedRowspanBegin = -1;

  int row = -1;
  for (Item& item : m_items) {
    ++row;

    if (item.uncommittedPath != item.committedPath) {
      item.uncommittedPath = item.committedPath;
      item.uncommittedStatus = item.committedStatus;
      if (modifiedRowspanBegin == -1) {
        modifiedRowspanBegin = row;
      }
    } else {
      if (modifiedRowspanBegin != -1) {
        emit dataChanged(index(modifiedRowspanBegin), index(row));
        modifiedRowspanBegin = -1;
      }
    }
  }

  if (modifiedRowspanBegin != -1) {
    emit dataChanged(index(modifiedRowspanBegin), index(row));
  }

  m_haveUncommittedChanges = false;
}  // RelinkingModel::rollbackChanges

void RelinkingModel::ensureEndsWithSlash(QString& str) {
  if (!str.endsWith(QChar('/'))) {
    str += QChar('/');
  }
}

void RelinkingModel::requestStatusUpdate(const QModelIndex& index) {
  assert(index.isValid());

  Item& item = m_items[index.row()];
  item.uncommittedStatus = StatusUpdatePending;

  m_statusUpdateThread->requestStatusUpdate(item.uncommittedPath, index.row());
}

void RelinkingModel::customEvent(QEvent* event) {
  using ResponseEvent = PayloadEvent<StatusUpdateResponse>;
  auto* evt = dynamic_cast<ResponseEvent*>(event);
  assert(evt);

  const StatusUpdateResponse& response = evt->payload();
  if ((response.row() < 0) || (response.row() >= int(m_items.size()))) {
    return;
  }

  Item& item = m_items[response.row()];
  if (item.uncommittedPath == response.path()) {
    item.uncommittedStatus = response.status();
  }
  if (item.committedPath == response.path()) {
    item.committedStatus = response.status();
  }

  emit dataChanged(index(response.row()), index(response.row()));
}

/*========================== StatusUpdateThread =========================*/

RelinkingModel::StatusUpdateThread::StatusUpdateThread(RelinkingModel* owner)
    : QThread(owner), m_owner(owner), m_tasks(), m_tasksByPath(), m_exiting(false) {}

RelinkingModel::StatusUpdateThread::~StatusUpdateThread() {
  {
    QMutexLocker locker(&m_mutex);
    m_exiting = true;
  }

  m_cond.wakeAll();
  wait();
}

void RelinkingModel::StatusUpdateThread::requestStatusUpdate(const std::filesystem::path& path, int row) {
  const QMutexLocker locker(&m_mutex);
  if (m_exiting) {
    return;
  }

  if (path == m_pathBeingProcessed) {
    // This task is currently in progress.
    return;
  }

  auto [it, inserted] = m_tasksByPath.try_emplace(path, m_tasks.end());
  if (inserted) {
    try {
      it->second = m_tasks.emplace(m_tasks.end(), path, row);
    } catch (...) {
      m_tasksByPath.erase(it);
      throw;
    }
  }

  // Whether inserted or already there, move it to the back of the priority queue.
  m_tasks.splice(m_tasks.end(), m_tasks, it->second);

  if (!isRunning()) {
    start();
  }

  m_cond.wakeOne();
}

void RelinkingModel::StatusUpdateThread::run() try {
  const QMutexLocker locker(&m_mutex);

  class MutexUnlocker {
   public:
    explicit MutexUnlocker(QMutex* mutex) : m_mutex(mutex) { mutex->unlock(); }

    ~MutexUnlocker() { m_mutex->lock(); }

   private:
    QMutex* const m_mutex;
  };


  while (true) {
    if (m_exiting) {
      break;
    }

    if (m_tasks.empty()) {
      m_cond.wait(&m_mutex);
    }

    if (m_tasks.empty()) {
      continue;
    }

    const Task task(m_tasks.front());
    m_pathBeingProcessed = task.path;
    m_tasksByPath.erase(task.path);
    m_tasks.pop_front();

    {
      const MutexUnlocker unlocker(&m_mutex);

      Status status = StatusUpdateFailed;
      try {
        status = QFile::exists(core::toQString(task.path)) ? Exists : Missing;
      } catch (const std::bad_alloc&) {
        OutOfMemoryHandler::instance().handleOutOfMemorySituation();
      } catch (const std::exception& e) {
        qWarning("Failed to check relinked path '%s': %s", qPrintable(core::toQString(task.path)), e.what());
      } catch (...) {
        qWarning("Failed to check relinked path '%s': unknown exception.", qPrintable(core::toQString(task.path)));
      }

      try {
        const StatusUpdateResponse response(task.path, task.row, status);
        QCoreApplication::postEvent(m_owner, new PayloadEvent<StatusUpdateResponse>(response));
      } catch (const std::bad_alloc&) {
        OutOfMemoryHandler::instance().handleOutOfMemorySituation();
      } catch (const std::exception& e) {
        qWarning("Failed to post relinked path status: %s", e.what());
      } catch (...) {
        qWarning("Failed to post relinked path status: unknown exception.");
      }
    }

    m_pathBeingProcessed.clear();
  }
}  // RelinkingModel::StatusUpdateThread::run

catch (const std::bad_alloc&) {
  OutOfMemoryHandler::instance().handleOutOfMemorySituation();
} catch (const std::exception& e) {
  qCritical("Relinking status thread failed: %s", e.what());
} catch (...) {
  qCritical("Relinking status thread failed with an unknown exception.");
}

/*================================ Item =================================*/

RelinkingModel::Item::Item(const RelinkablePath& path, const EntryKind entryKind)
    : origPath(path.normalizedPath()),
      committedPath(path.normalizedPath()),
      uncommittedPath(path.normalizedPath()),
      type(path.type()),
      committedStatus(StatusUpdatePending),
      uncommittedStatus(StatusUpdatePending),
      entryKind(entryKind) {}

/*============================== Relinker ================================*/

void RelinkingModel::Relinker::addMapping(const std::filesystem::path& from, const std::filesystem::path& to) {
  m_mappings[from] = to;
}

std::filesystem::path RelinkingModel::Relinker::substitutionPathFor(const RelinkablePath& path) const {
  const auto it(m_mappings.find(path.normalizedPath()));
  if (it != m_mappings.end()) {
    return it->second;
  } else {
    return path.normalizedPath();
  }
}

void RelinkingModel::Relinker::swap(Relinker& other) {
  m_mappings.swap(other.m_mappings);
}
