// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_APP_RELINKINGMODEL_H_
#define SCANTAILOR_APP_RELINKINGMODEL_H_

#include <QAbstractListModel>
#include <QColor>
#include <QIcon>
#include <QModelIndex>
#include <QString>
#include <QStringList>
#include <QThread>
#include <Qt>
#include <filesystem>
#include <memory>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "AbstractRelinker.h"
#include "NonCopyable.h"
#include "RelinkablePath.h"
#include "VirtualFunction.h"

class RelinkingModel : public QAbstractListModel {
  DECLARE_NON_COPYABLE(RelinkingModel)

 public:
  enum Status { Exists, Missing, StatusUpdatePending, StatusUpdateFailed };

  enum EntryKind { CommonImageRoot, ImageFile, OutputDirectory };

  enum {
    TypeRole = Qt::UserRole,
    UncommittedPathRole,
    UncommittedStatusRole,
    EntryKindRole,
    TreeGroupRole,
    TreeSourceRowRole
  };

  struct Summary {
    int imagesInsideRoot = 0;
    int imagesOutsideRoot = 0;
    int missingPaths = 0;
    int failedPaths = 0;
    int pendingPaths = 0;
    bool hasOutput = false;
    bool outputInsideRoot = false;
    Status outputStatus = StatusUpdatePending;
  };

  RelinkingModel();

  ~RelinkingModel() override;

  int rowCount(const QModelIndex& parent = QModelIndex()) const override;

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  /**
   * This method guarantees that
   * \code
   * model.relinker().release() == model.relinker().release()
   * \endcode
   * will hold true for the lifetime of RelinkingModel object.
   * This allows you to take the relinker right after construction
   * and then use it when accepted() signal is emitted.
   */
  std::shared_ptr<AbstractRelinker> relinker() const { return m_relinker; }

  void operator()(const RelinkablePath& path) { addPath(path); }

  void addPath(const RelinkablePath& path);

  void addCommonImageRoot();

  void addOutputPath(const RelinkablePath& path);

  static QString findCommonImageRoot(const QStringList& imagePaths);

  Summary summary() const;

  static QColor foregroundColorForStatus(Status status);

  void replacePrefix(const QString& prefix, const QString& replacement, RelinkablePath::Type type, EntryKind entryKind);

  /**
   * Returns true if we have different original paths remapped to the same one.
   * Checking is done on uncommitted paths.
   */
  bool checkForMerges() const;

  void commitChanges();

  void rollbackChanges();

  void requestStatusUpdate(const QModelIndex& index);

 protected:
  void customEvent(QEvent* event) override;

 private:
  void addPath(const RelinkablePath& path, EntryKind entryKind);

  QString commonImageRootPath() const;

  class StatusUpdateThread;
  class StatusUpdateResponse;

  /** Stands for File System Object (file or directory). */
  struct Item {
    std::filesystem::path origPath;

    /**< That's the path passed through addPath(). It never changes. */
    std::filesystem::path committedPath;
    std::filesystem::path uncommittedPath;

    /**< Same as committedPath when m_haveUncommittedChanges == false. */
    RelinkablePath::Type type;
    Status committedStatus;
    Status uncommittedStatus;

    /**< Same as committedStatus when m_haveUncommittedChanges == false. */

    EntryKind entryKind;

    Item(const RelinkablePath& path, EntryKind entryKind);
  };

  class Relinker : public AbstractRelinker {
   public:
    void addMapping(const std::filesystem::path& from, const std::filesystem::path& to);

    /** Returns path.normalizedPath() if there is no mapping for it. */
    std::filesystem::path substitutionPathFor(const RelinkablePath& path) const override;

    void swap(Relinker& other);

   private:
    std::unordered_map<std::filesystem::path, std::filesystem::path> m_mappings;
  };


  static void ensureEndsWithSlash(QString& str);

  QPixmap m_fileIcon;
  QPixmap m_folderIcon;
  std::vector<Item> m_items;
  std::set<std::pair<std::filesystem::path, EntryKind>> m_origPathSet;
  const std::shared_ptr<Relinker> m_relinker;
  std::unique_ptr<StatusUpdateThread> m_statusUpdateThread;
  bool m_haveUncommittedChanges;
};


#endif  // ifndef SCANTAILOR_APP_RELINKINGMODEL_H_
