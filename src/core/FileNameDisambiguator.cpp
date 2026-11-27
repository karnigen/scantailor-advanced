// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "FileNameDisambiguator.h"

#include <QDomDocument>
#include <QFileInfo>
#include <QMutex>

#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <utility>

#include "AbstractRelinker.h"
#include "PathUtils.h"
#include "RelinkablePath.h"

class FileNameDisambiguator::Impl {
 public:
  Impl();

  Impl(const QDomElement& disambiguatorEl,
       const std::function<std::filesystem::path(const QString&)>& filePathUnpacker);

  QDomElement toXml(QDomDocument& doc,
                    const QString& name,
                    const std::function<QString(const std::filesystem::path&)>& filePathPacker) const;

  int getLabel(const std::filesystem::path& filePath) const;

  int registerFile(const std::filesystem::path& filePath);

  void performRelinking(const AbstractRelinker& relinker);

 private:
  struct Item {
    std::filesystem::path filePath;
    QString fileName;
    int label;

    Item(std::filesystem::path filePath, int lbl);

    Item(std::filesystem::path filePath, const QString& fileName, int lbl);
  };

  using ItemsInOrder = std::list<Item>;
  using ItemsByFilePath = std::map<std::filesystem::path, ItemsInOrder::iterator>;
  using FileNameLabel = std::pair<QString, int>;
  using ItemsByFileNameLabel = std::map<FileNameLabel, ItemsInOrder::iterator>;

  static bool insertItem(ItemsInOrder& items,
                         ItemsByFilePath& itemsByFilePath,
                         ItemsByFileNameLabel& itemsByFileNameLabel,
                         const Item& item);

  mutable QMutex m_mutex;
  ItemsInOrder m_items;
  ItemsByFilePath m_itemsByFilePath;
  ItemsByFileNameLabel m_itemsByFileNameLabel;
};


/*====================== FileNameDisambiguator =========================*/

FileNameDisambiguator::FileNameDisambiguator() : m_impl(std::make_unique<Impl>()) {}

FileNameDisambiguator::FileNameDisambiguator(const QDomElement& disambiguatorEl)
    : m_impl(
          std::make_unique<Impl>(disambiguatorEl, [](const QString& path) { return core::toFileSystemPath(path); })) {}

FileNameDisambiguator::FileNameDisambiguator(
    const QDomElement& disambiguatorEl,
    const std::function<std::filesystem::path(const QString&)>& filePathUnpacker)
    : m_impl(std::make_unique<Impl>(disambiguatorEl, filePathUnpacker)) {}

FileNameDisambiguator::~FileNameDisambiguator() = default;

QDomElement FileNameDisambiguator::toXml(QDomDocument& doc, const QString& name) const {
  return m_impl->toXml(doc, name, [](const std::filesystem::path& path) { return core::toQString(path); });
}

QDomElement FileNameDisambiguator::toXml(
    QDomDocument& doc,
    const QString& name,
    const std::function<QString(const std::filesystem::path&)>& filePathPacker) const {
  return m_impl->toXml(doc, name, filePathPacker);
}

int FileNameDisambiguator::getLabel(const std::filesystem::path& filePath) const {
  return m_impl->getLabel(filePath);
}

int FileNameDisambiguator::registerFile(const std::filesystem::path& filePath) {
  return m_impl->registerFile(filePath);
}

void FileNameDisambiguator::performRelinking(const AbstractRelinker& relinker) {
  m_impl->performRelinking(relinker);
}

/*==================== FileNameDisambiguator::Impl ====================*/

FileNameDisambiguator::Impl::Impl() = default;

FileNameDisambiguator::Impl::Impl(const QDomElement& disambiguatorEl,
                                  const std::function<std::filesystem::path(const QString&)>& filePathUnpacker) {
  QDomNode node(disambiguatorEl.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != "mapping") {
      continue;
    }
    const QDomElement fileEl(node.toElement());

    const QString filePathShorthand(fileEl.attribute("file"));
    const std::filesystem::path filePath = filePathUnpacker(filePathShorthand);
    if (filePath.empty()) {
      // Unresolved shorthand - skipping this record.
      continue;
    }

    const int label = fileEl.attribute("label").toInt();
    insertItem(m_items, m_itemsByFilePath, m_itemsByFileNameLabel, Item(filePath, label));
  }
}

bool FileNameDisambiguator::Impl::insertItem(ItemsInOrder& items,
                                             ItemsByFilePath& itemsByFilePath,
                                             ItemsByFileNameLabel& itemsByFileNameLabel,
                                             const Item& item) {
  if (itemsByFilePath.find(item.filePath) != itemsByFilePath.end()) {
    return false;
  }

  const FileNameLabel fileNameLabel(item.fileName, item.label);
  if (itemsByFileNameLabel.find(fileNameLabel) != itemsByFileNameLabel.end()) {
    return false;
  }

  items.push_back(item);
  const auto itemIt = std::prev(items.end());
  ItemsByFilePath::iterator pathIt = itemsByFilePath.end();
  try {
    const auto pathResult = itemsByFilePath.emplace(itemIt->filePath, itemIt);
    if (!pathResult.second) {
      items.erase(itemIt);
      return false;
    }
    pathIt = pathResult.first;

    const auto nameLabelResult = itemsByFileNameLabel.emplace(fileNameLabel, itemIt);
    if (!nameLabelResult.second) {
      itemsByFilePath.erase(pathIt);
      items.erase(itemIt);
      return false;
    }
  } catch (...) {
    if (pathIt != itemsByFilePath.end()) {
      itemsByFilePath.erase(pathIt);
    }
    items.erase(itemIt);
    throw;
  }

  return true;
}

QDomElement FileNameDisambiguator::Impl::toXml(
    QDomDocument& doc,
    const QString& name,
    const std::function<QString(const std::filesystem::path&)>& filePathPacker) const {
  const QMutexLocker locker(&m_mutex);

  QDomElement el(doc.createElement(name));

  for (const Item& item : m_items) {
    const QString filePathShorthand = filePathPacker(item.filePath);
    if (filePathShorthand.isEmpty()) {
      // Unrepresentable file path - skipping this record.
      continue;
    }

    QDomElement fileEl(doc.createElement("mapping"));
    fileEl.setAttribute("file", filePathShorthand);
    fileEl.setAttribute("label", item.label);
    el.appendChild(fileEl);
  }
  return el;
}

int FileNameDisambiguator::Impl::getLabel(const std::filesystem::path& filePath) const {
  const QMutexLocker locker(&m_mutex);

  const auto fpIt(m_itemsByFilePath.find(filePath));
  if (fpIt != m_itemsByFilePath.end()) {
    return fpIt->second->label;
  }
  return 0;
}

int FileNameDisambiguator::Impl::registerFile(const std::filesystem::path& filePath) {
  const QMutexLocker locker(&m_mutex);

  const auto fpIt(m_itemsByFilePath.find(filePath));
  if (fpIt != m_itemsByFilePath.end()) {
    return fpIt->second->label;
  }

  int label = 0;

  const QString fileName(QFileInfo(core::toQString(filePath)).fileName());
  const auto fnIt(m_itemsByFileNameLabel.upper_bound(FileNameLabel(fileName, std::numeric_limits<int>::max())));
  // If the item preceeding fnIt has the same file name,
  // the new file belongs to the same disambiguation group.
  if (fnIt != m_itemsByFileNameLabel.begin()) {
    auto prev(fnIt);
    --prev;
    if (prev->first.first == fileName) {
      label = prev->first.second + 1;
    }
  }  // Otherwise, label remains 0.
  const Item newItem(filePath, fileName, label);
  insertItem(m_items, m_itemsByFilePath, m_itemsByFileNameLabel, newItem);
  return label;
}

void FileNameDisambiguator::Impl::performRelinking(const AbstractRelinker& relinker) {
  const QMutexLocker locker(&m_mutex);
  ItemsInOrder newItems;
  ItemsByFilePath newItemsByFilePath;
  ItemsByFileNameLabel newItemsByFileNameLabel;

  for (const Item& item : m_items) {
    const RelinkablePath oldPath(item.filePath, RelinkablePath::File);
    Item newItem(relinker.substitutionPathFor(oldPath), item.label);
    insertItem(newItems, newItemsByFilePath, newItemsByFileNameLabel, newItem);
  }

  m_items.swap(newItems);
  m_itemsByFilePath.swap(newItemsByFilePath);
  m_itemsByFileNameLabel.swap(newItemsByFileNameLabel);
}

/*============================ Impl::Item =============================*/

FileNameDisambiguator::Impl::Item::Item(std::filesystem::path filePath, int lbl)
    : filePath(std::move(filePath)), fileName(QFileInfo(core::toQString(this->filePath)).fileName()), label(lbl) {}

FileNameDisambiguator::Impl::Item::Item(std::filesystem::path filePath, const QString& fileName, int lbl)
    : filePath(std::move(filePath)), fileName(fileName), label(lbl) {}
