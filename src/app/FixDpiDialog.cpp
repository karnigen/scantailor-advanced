// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FixDpiDialog.h"

#include <QAbstractItemView>
#include <QFont>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QTableView>
#include <algorithm>
#include <map>
#include <optional>
#include <utility>

#include <core/PathUtils.h>
#include "ColorSchemeManager.h"
#include "ImageMetadataLoader.h"

// To be able to use it in QVariant
Q_DECLARE_METATYPE(ImageMetadata)

static const int IMAGE_METADATA_ROLE = Qt::UserRole;

class FixDpiDialog::PageTableModel : public QAbstractTableModel {
 public:
  explicit PageTableModel(const std::vector<ImageFileInfo>& files) : m_files(files) {
    const int numFiles = static_cast<int>(m_files.size());
    for (int fileIndex = 0; fileIndex < numFiles; ++fileIndex) {
      std::vector<ImageMetadata> sourceMetadata;
      const auto status = ImageMetadataLoader::load(
          core::toFileSystemPath(m_files[fileIndex].fileInfo().absoluteFilePath()),
          [&sourceMetadata](const ImageMetadata& metadata) { sourceMetadata.push_back(metadata); });
      const auto numImages = static_cast<int>(m_files[fileIndex].imageInfo().size());
      for (int imageIndex = 0; imageIndex < numImages; ++imageIndex) {
        const int sourceIndex = m_files[fileIndex].imageIndexes()[imageIndex];
        std::optional<Dpi> sourceDpi;
        if (status == ImageMetadataLoader::LOADED && sourceIndex >= 0
            && sourceIndex < static_cast<int>(sourceMetadata.size())) {
          sourceDpi = sourceMetadata[sourceIndex].dpi();
        }
        m_pages.push_back({fileIndex, imageIndex, sourceDpi});
      }
    }
  }

  const std::vector<ImageFileInfo>& files() const { return m_files; }

  const std::optional<Dpi>& sourceDpiForRow(const int row) const { return m_pages[row].sourceDpi; }

  Dpi projectDpiForRow(const int row) const { return metadataForRow(row).dpi(); }

  int rowCount(const QModelIndex& parent = QModelIndex()) const override {
    return parent.isValid() ? 0 : static_cast<int>(m_pages.size());
  }

  int columnCount(const QModelIndex& parent = QModelIndex()) const override { return parent.isValid() ? 0 : 3; }

  QVariant data(const QModelIndex& index, const int role) const override {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
      return QVariant();
    }

    const ImageMetadata& metadata = metadataForRow(index.row());
    if (role == IMAGE_METADATA_ROLE) {
      return QVariant::fromValue(metadata);
    }
    if (role == Qt::TextAlignmentRole && index.column() > 0) {
      return Qt::AlignCenter;
    }
    if (role == Qt::DisplayRole) {
      if (index.column() == 0) {
        const ImageFileInfo& file = m_files[m_pages[index.row()].fileIndex];
        const QString& fileName = file.fileInfo().fileName();
        return file.imageInfo().size() == 1
                   ? fileName
                   : FixDpiDialog::tr("%1 (page %2)").arg(fileName).arg(m_pages[index.row()].imageIndex + 1);
      }
      if (index.column() == 1) {
        const std::optional<Dpi>& sourceDpi = m_pages[index.row()].sourceDpi;
        return sourceDpi ? dpiToString(*sourceDpi) : FixDpiDialog::tr("Unavailable");
      }
      if (index.column() == 2) {
        return dpiToString(metadata.dpi());
      }
    }
    if (role == Qt::ForegroundRole && index.column() == 2) {
      if (!metadata.isDpiOK() || metadata.horizontalDpiStatus() != ImageMetadata::DPI_OK
          || metadata.verticalDpiStatus() != ImageMetadata::DPI_OK) {
        return ColorSchemeManager::instance().getColorParam("FixDpiDialogErrorText", QColor(Qt::red));
      }
    }
    if (role == Qt::ToolTipRole && index.column() == 2) {
      return QStringLiteral("%1 x %2 DPI").arg(metadata.dpi().horizontal()).arg(metadata.dpi().vertical());
    }
    if (role == Qt::ToolTipRole && index.column() == 1 && !m_pages[index.row()].sourceDpi) {
      return FixDpiDialog::tr("Could not read DPI from the source image.");
    }
    return QVariant();
  }

  QVariant headerData(const int section, const Qt::Orientation orientation, const int role) const override {
    if (orientation != Qt::Horizontal) {
      return QVariant();
    }
    if (role == Qt::FontRole) {
      QFont font;
      font.setBold(true);
      return font;
    }
    if (role != Qt::DisplayRole) {
      return QVariant();
    }
    if (section == 0) {
      return FixDpiDialog::tr("Name");
    }
    if (section == 1) {
      return FixDpiDialog::tr("Source DPI");
    }
    if (section == 2) {
      return FixDpiDialog::tr("Project DPI");
    }
    return QVariant();
  }

  bool allDpisOK() const {
    for (const Page& page : m_pages) {
      if (!metadataForPage(page).isDpiOK()) {
        return false;
      }
    }
    return true;
  }

  bool wouldChangeDpi(const std::vector<int>& rows, const Dpi& dpi) const {
    for (const int row : rows) {
      if (row < 0 || row >= rowCount()) {
        continue;
      }
      const ImageMetadata& metadata = metadataForRow(row);
      if (!sameDpi(metadata.dpi(), dpi)) {
        return true;
      }
    }
    return false;
  }

  bool wouldChangeAnyDpi(const Dpi& dpi) const {
    for (const Page& page : m_pages) {
      if (!sameDpi(metadataForPage(page).dpi(), dpi)) {
        return true;
      }
    }
    return false;
  }

  void applyDpi(const std::vector<int>& rows, const Dpi& dpi) {
    for (const int row : rows) {
      if (row < 0 || row >= rowCount()) {
        continue;
      }
      ImageMetadata& metadata = metadataForRow(row);
      metadata.setDpi(dpi);
      emit dataChanged(index(row, 0), index(row, 2));
    }
  }

 private:
  struct Page {
    int fileIndex;
    int imageIndex;
    std::optional<Dpi> sourceDpi;
  };

  static bool sameDpi(const Dpi& first, const Dpi& second) {
    return first.horizontal() == second.horizontal() && first.vertical() == second.vertical();
  }

  static QString dpiToString(const Dpi& dpi) {
    const QString xDpi = dpi.horizontal() > 1 ? QString::number(dpi.horizontal()) : FixDpiDialog::tr("Missing");
    const QString yDpi = dpi.vertical() > 1 ? QString::number(dpi.vertical()) : FixDpiDialog::tr("Missing");
    return QStringLiteral("%1 x %2").arg(xDpi, yDpi);
  }

  ImageMetadata& metadataForRow(const int row) { return metadataForPage(m_pages[row]); }

  const ImageMetadata& metadataForRow(const int row) const { return metadataForPage(m_pages[row]); }

  ImageMetadata& metadataForPage(const Page& page) { return m_files[page.fileIndex].imageInfo()[page.imageIndex]; }

  const ImageMetadata& metadataForPage(const Page& page) const {
    return m_files[page.fileIndex].imageInfo()[page.imageIndex];
  }

  std::vector<ImageFileInfo> m_files;
  std::vector<Page> m_pages;
};

class FixDpiDialog::PageTableSortProxyModel : public QSortFilterProxyModel {
 public:
  explicit PageTableSortProxyModel(PageTableModel* pages) : m_pages(pages) {
    setSourceModel(pages);
    setDynamicSortFilter(true);
  }

 protected:
  bool lessThan(const QModelIndex& left, const QModelIndex& right) const override {
    if (left.column() == 1) {
      const std::optional<Dpi>& leftDpi = m_pages->sourceDpiForRow(left.row());
      const std::optional<Dpi>& rightDpi = m_pages->sourceDpiForRow(right.row());
      if (!leftDpi || !rightDpi) {
        return !leftDpi && rightDpi.has_value();
      }
      return std::pair(leftDpi->horizontal(), leftDpi->vertical())
             < std::pair(rightDpi->horizontal(), rightDpi->vertical());
    }
    if (left.column() == 2) {
      const Dpi leftDpi = m_pages->projectDpiForRow(left.row());
      const Dpi rightDpi = m_pages->projectDpiForRow(right.row());
      return std::pair(leftDpi.horizontal(), leftDpi.vertical())
             < std::pair(rightDpi.horizontal(), rightDpi.vertical());
    }
    return QString::localeAwareCompare(left.data(Qt::DisplayRole).toString(), right.data(Qt::DisplayRole).toString())
           < 0;
  }

 private:
  PageTableModel* m_pages;
};

FixDpiDialog::FixDpiDialog(const std::vector<ImageFileInfo>& files, QWidget* parent)
    : QDialog(parent),
      m_pages(std::make_unique<PageTableModel>(files)),
      m_sortProxy(std::make_unique<PageTableSortProxyModel>(m_pages.get())) {
  setupUi(this);

  m_normalPalette = xDpi->palette();
  m_errorPalette = m_normalPalette;
  const QColor errorTextColor = ColorSchemeManager::instance().getColorParam("FixDpiDialogErrorText", QColor(Qt::red));
  m_errorPalette.setColor(QPalette::Text, errorTextColor);

  dpiCombo->addItem("300 x 300", QSize(300, 300));
  dpiCombo->addItem("400 x 400", QSize(400, 400));
  dpiCombo->addItem("600 x 600", QSize(600, 600));
  dpiCombo->addItem("1200 x 1200", QSize(1200, 1200));

  allPagesTable->setModel(m_sortProxy.get());
  allPagesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  allPagesTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
  allPagesTable->verticalHeader()->setVisible(false);
  allPagesTable->verticalHeader()->setDefaultSectionSize(22);
  QHeaderView* horizontalHeader = allPagesTable->horizontalHeader();
  horizontalHeader->setSectionsClickable(true);
  horizontalHeader->setSortIndicatorShown(false);
  QFont headerFont(horizontalHeader->font());
  headerFont.setBold(true);
  horizontalHeader->setFont(headerFont);
  horizontalHeader->setSectionResizeMode(0, QHeaderView::Stretch);
  horizontalHeader->setSectionResizeMode(1, QHeaderView::Fixed);
  horizontalHeader->setSectionResizeMode(2, QHeaderView::Fixed);
  allPagesTable->setColumnWidth(1, 112);
  allPagesTable->setColumnWidth(2, 112);
  connect(horizontalHeader, &QHeaderView::sectionClicked, this, [this, horizontalHeader](const int section) {
    if (m_sortColumn == section) {
      m_sortOrder = m_sortOrder == Qt::AscendingOrder ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
      m_sortColumn = section;
      m_sortOrder = Qt::AscendingOrder;
    }
    horizontalHeader->setSortIndicator(section, m_sortOrder);
    horizontalHeader->setSortIndicatorShown(true);
    m_sortProxy->sort(section, m_sortOrder);
  });

  xDpi->setMaxLength(4);
  yDpi->setMaxLength(4);
  xDpi->setValidator(new QIntValidator(xDpi));
  yDpi->setValidator(new QIntValidator(yDpi));

  connect(allPagesTable->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
          SLOT(selectionChanged(const QItemSelection&)));

  connect(dpiCombo, SIGNAL(activated(int)), this, SLOT(dpiComboChangedByUser(int)));

  connect(xDpi, SIGNAL(textEdited(const QString&)), this, SLOT(dpiValueChanged()));
  connect(yDpi, SIGNAL(textEdited(const QString&)), this, SLOT(dpiValueChanged()));

  connect(applyBtn, SIGNAL(clicked()), this, SLOT(applyClicked()));
  connect(applyAllBtn, SIGNAL(clicked()), this, SLOT(applyAllClicked()));

  const bool hasPages = m_pages->rowCount() > 0;
  dpiCombo->setEnabled(hasPages);
  xDpi->setEnabled(hasPages);
  yDpi->setEnabled(hasPages);
  dpiValueChanged();
  updateDpiStatistics();
  enableDisableOkButton();
}

FixDpiDialog::~FixDpiDialog() = default;

const std::vector<ImageFileInfo>& FixDpiDialog::files() const {
  return m_pages->files();
}

void FixDpiDialog::selectionChanged(const QItemSelection& selection) {
  updateDpiFromSelection(selection);
}

std::vector<int> FixDpiDialog::selectedRows() const {
  const QModelIndexList selectedIndexes = allPagesTable->selectionModel()->selectedRows(0);
  std::vector<int> rows;
  rows.reserve(static_cast<size_t>(selectedIndexes.size()));
  for (const QModelIndex& selectedIndex : selectedIndexes) {
    rows.push_back(m_sortProxy->mapToSource(selectedIndex).row());
  }
  return rows;
}

void FixDpiDialog::dpiComboChangedByUser(const int index) {
  const QVariant data(dpiCombo->itemData(index));
  if (data.isValid()) {
    const QSize dpi(data.toSize());
    xDpi->setText(QString::number(dpi.width()));
    yDpi->setText(QString::number(dpi.height()));
    dpiValueChanged();
  }
}

void FixDpiDialog::dpiValueChanged() {
  updateDpiCombo();

  bool xOk = false;
  bool yOk = false;
  const Dpi dpi(xDpi->text().toInt(&xOk), yDpi->text().toInt(&yOk));
  const ImageMetadata metadata(QSize(), dpi);

  const auto statusForAxis = [this, &metadata, &dpi](const bool horizontal) {
    if (m_selectedItemPixelSizes.empty()) {
      return horizontal ? metadata.horizontalDpiStatus() : metadata.verticalDpiStatus();
    }
    for (const QSize& size : m_selectedItemPixelSizes) {
      const ImageMetadata selectedMetadata(size, dpi);
      const ImageMetadata::DpiStatus status
          = horizontal ? selectedMetadata.horizontalDpiStatus() : selectedMetadata.verticalDpiStatus();
      if (status != ImageMetadata::DPI_OK) {
        return status;
      }
    }
    return ImageMetadata::DPI_OK;
  };

  decorateDpiInputField(xDpi, statusForAxis(true));
  decorateDpiInputField(yDpi, statusForAxis(false));

  const bool validDpi = xOk && yOk && metadata.isDpiOK();
  applyBtn->setEnabled(validDpi && m_pages->wouldChangeDpi(selectedRows(), dpi));
  applyAllBtn->setEnabled(validDpi && m_pages->wouldChangeAnyDpi(dpi));
}

void FixDpiDialog::applyClicked() {
  const Dpi dpi(xDpi->text().toInt(), yDpi->text().toInt());
  m_pages->applyDpi(selectedRows(), dpi);
  updateDpiFromSelection(allPagesTable->selectionModel()->selection());
  updateDpiStatistics();
  enableDisableOkButton();
}

void FixDpiDialog::applyAllClicked() {
  const Dpi dpi(xDpi->text().toInt(), yDpi->text().toInt());
  std::vector<int> rows;
  rows.reserve(static_cast<size_t>(m_pages->rowCount()));
  for (int row = 0; row < m_pages->rowCount(); ++row) {
    rows.push_back(row);
  }
  m_pages->applyDpi(rows, dpi);
  updateDpiFromSelection(allPagesTable->selectionModel()->selection());
  updateDpiStatistics();
  enableDisableOkButton();
}

void FixDpiDialog::updateDpiStatistics() {
  std::map<std::pair<int, int>, int> dpiCounts;
  for (int row = 0; row < m_pages->rowCount(); ++row) {
    const ImageMetadata metadata = m_pages->index(row, 0).data(IMAGE_METADATA_ROLE).value<ImageMetadata>();
    ++dpiCounts[{metadata.dpi().horizontal(), metadata.dpi().vertical()}];
  }

  QStringList summaries;
  for (const auto& [dpi, count] : dpiCounts) {
    const QString dpiText = dpi.first > 1 && dpi.second > 1 ? QStringLiteral("%1 x %2").arg(dpi.first).arg(dpi.second)
                                                            : tr("Missing DPI");
    const QString pageLabel = count == 1 ? tr("page") : tr("pages");
    summaries.push_back(QStringLiteral("%1: %2 %3").arg(dpiText).arg(count).arg(pageLabel));
  }
  dpiStatsLabel->setText(tr("Distinct DPI values: %1 (%2)").arg(dpiCounts.size()).arg(summaries.join("; ")));
}

void FixDpiDialog::enableDisableOkButton() {
  const bool enable = m_pages->allDpisOK();
  buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

void FixDpiDialog::updateDpiFromSelection(const QItemSelection& selection) {
  std::vector<ImageMetadata> selectedMetadata;
  for (const QModelIndex& selectedIndex : selection.indexes()) {
    if (selectedIndex.column() != 0) {
      continue;
    }
    const QVariant metadata = selectedIndex.data(IMAGE_METADATA_ROLE);
    if (metadata.isValid()) {
      selectedMetadata.push_back(metadata.value<ImageMetadata>());
    }
  }

  m_selectedItemPixelSizes.clear();
  for (const ImageMetadata& metadata : selectedMetadata) {
    m_selectedItemPixelSizes.push_back(metadata.size());
  }

  const bool hasPages = m_pages->rowCount() > 0;
  dpiCombo->setEnabled(hasPages);
  xDpi->setEnabled(hasPages);
  yDpi->setEnabled(hasPages);

  if (selectedMetadata.empty()) {
    resetDpiForm();
    return;
  }

  const Dpi firstDpi = selectedMetadata.front().dpi();
  const bool allDpisMatch
      = std::all_of(selectedMetadata.begin(), selectedMetadata.end(), [&firstDpi](const auto& item) {
          return item.dpi().horizontal() == firstDpi.horizontal() && item.dpi().vertical() == firstDpi.vertical();
        });
  if (allDpisMatch) {
    setDpiForm(selectedMetadata.front());
  } else {
    resetDpiForm();
  }
}

void FixDpiDialog::resetDpiForm() {
  dpiCombo->setCurrentIndex(0);
  xDpi->clear();
  yDpi->clear();
  dpiValueChanged();
}

void FixDpiDialog::setDpiForm(const ImageMetadata& metadata) {
  const Dpi dpi(metadata.dpi());

  if (dpi.isNull()) {
    resetDpiForm();
    return;
  }

  xDpi->setText(QString::number(dpi.horizontal()));
  yDpi->setText(QString::number(dpi.vertical()));
  dpiValueChanged();
}

void FixDpiDialog::updateDpiCombo() {
  bool xOk = true, y_ok = true;
  const QSize dpi(xDpi->text().toInt(&xOk), yDpi->text().toInt(&y_ok));

  if (xOk && y_ok) {
    const int count = dpiCombo->count();
    for (int i = 0; i < count; ++i) {
      const QVariant data(dpiCombo->itemData(i));
      if (data.isValid()) {
        if (dpi == data.toSize()) {
          dpiCombo->setCurrentIndex(i);
          return;
        }
      }
    }
  }

  dpiCombo->setCurrentIndex(0);
}

void FixDpiDialog::decorateDpiInputField(QLineEdit* field, ImageMetadata::DpiStatus dpiStatus) const {
  if (dpiStatus == ImageMetadata::DPI_OK) {
    field->setPalette(m_normalPalette);
  } else {
    field->setPalette(m_errorPalette);
  }

  switch (dpiStatus) {
    case ImageMetadata::DPI_OK:
    case ImageMetadata::DPI_UNDEFINED:
      field->setToolTip(QString());
      break;
    case ImageMetadata::DPI_TOO_LARGE:
      field->setToolTip(tr("DPI is too large and most likely wrong."));
      break;
    case ImageMetadata::DPI_TOO_SMALL:
      field->setToolTip(
          tr("DPI is too small. Even if it's correct, you are not going to get acceptable results with it."));
      break;
    case ImageMetadata::DPI_TOO_SMALL_FOR_THIS_PIXEL_SIZE:
      field->setToolTip(
          tr("DPI is too small for this pixel size. Such combination would probably lead to out of "
             "memory errors."));
      break;
  }
}
