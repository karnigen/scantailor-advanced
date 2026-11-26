// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "LoadFileTask.h"

#include <imageproc/Grayscale.h>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>
#include <utility>

#include "AbstractFilter.h"
#include "ApplicationSettings.h"
#include "Dpm.h"
#include "ErrorWidget.h"
#include "FilterData.h"
#include "FilterOptionsWidget.h"
#include "FilterUiInterface.h"
#include "ImageLoader.h"
#include "PathUtils.h"
#include "ProjectPages.h"
#include "ThumbnailPixmapCache.h"
#include "filters/fix_orientation/Task.h"

using namespace imageproc;

class LoadFileTask::ErrorResult : public FilterResult {
  Q_DECLARE_TR_FUNCTIONS(LoadFileTask)
 public:
  ErrorResult(const QString& filePath, bool dimensionsKnown);
  ErrorResult(const QString& filePath, const QString& errorDetails);

  void updateUI(FilterUiInterface* ui) override;
  QString failureDescription() const override;

  std::shared_ptr<AbstractFilter> filter() override { return nullptr; }

 private:
  QString m_filePath;
  QString m_errorDetails;
  bool m_fileExists;
  bool m_dimensionsKnown;
  bool m_processingError = false;
};

namespace {

constexpr int kLimitUpdateRetryIntervalMs = 100;
constexpr int kLimitUpdateTimeoutMs = 5000;

bool canOfferAllocationLimitIncrease(const ImageAllocationLimitInfo& limitInfo) {
  return limitInfo.canRaiseLimit() && !ImageAllocationLimit::hasEnvironmentOverride();
}

class LargeImageOptionsWidget : public FilterOptionsWidget {
  Q_DECLARE_TR_FUNCTIONS(LoadFileTask)

 public:
  LargeImageOptionsWidget(QString filePath, ImageAllocationLimitInfo limitInfo)
      : m_filePath(std::move(filePath)), m_limitInfo(limitInfo) {
    auto* layout = new QVBoxLayout(this);
    const bool environmentOverride = ImageAllocationLimit::hasEnvironmentOverride();
    QString explanationText;
    if (!m_limitInfo.canRaiseLimit()) {
      explanationText = tr("This image is too large for the maximum configurable image allocation limit.");
    } else if (environmentOverride) {
      explanationText = tr(
          "QT_IMAGEIO_MAXALLOC overrides the allocation limit. Change this environment variable and "
          "restart the application to load this image.");
    } else {
      explanationText = tr(
          "The image is larger than the current allocation limit. Increasing the limit affects all "
          "image loads in this application. Large or malformed images can exhaust memory, slow down "
          "the system, or crash the application. Choose whether to allow this limit for this run or "
          "remember it in application settings.");
    }
    m_initialExplanation = explanationText;
    m_explanation = new QLabel(explanationText, this);
    m_explanation->setWordWrap(true);
    layout->addWidget(m_explanation);

    if (canOfferAllocationLimitIncrease(m_limitInfo)) {
      m_limitButton = new QPushButton(tr("Review image allocation limit..."), this);
      m_limitButton->setToolTip(
          tr("Review the image allocation limit and optionally increase it for this application."));
      layout->addWidget(m_limitButton);
      connect(m_limitButton, &QPushButton::clicked, this, [this]() {
        if (m_waitingForLimitUpdate) {
          cancelLimitUpdate();
        } else {
          requestLimitIncrease();
        }
      });
    }
    layout->addStretch();
  }

 private:
  void requestLimitIncrease() {
    if (!m_limitInfo.canRaiseLimit()) {
      m_explanation->setText(tr("The image dimensions exceed the maximum limit that can be configured."));
      return;
    }

    if (ImageAllocationLimit::hasEnvironmentOverride()) {
      m_explanation->setText(
          tr("QT_IMAGEIO_MAXALLOC overrides the allocation limit. Change this environment variable and restart the "
             "application to load this image."));
      return;
    }

    QString message = tr("The image %1 (%2 x %3 pixels) requires an estimated allocation limit of at least %4 MiB. "
                         "The current limit is %5 MiB. This application-wide limit applies to each image. Raising "
                         "it can cause memory exhaustion or make the system unresponsive.")
                          .arg(QDir::toNativeSeparators(m_filePath))
                          .arg(m_limitInfo.imageSize.width())
                          .arg(m_limitInfo.imageSize.height())
                          .arg(m_limitInfo.requiredLimitMb)
                          .arg(m_limitInfo.currentLimitMb);

    auto* const dialog
        = new QMessageBox(QMessageBox::Warning, tr("Image Too Large"), message, QMessageBox::Cancel, window());
    QPushButton* const onceButton = dialog->addButton(tr("Allow for This Run"), QMessageBox::AcceptRole);
    QPushButton* const rememberButton = dialog->addButton(tr("Remember This Limit"), QMessageBox::AcceptRole);
    dialog->setDefaultButton(QMessageBox::Cancel);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(dialog, &QDialog::finished, this, [this, dialog, onceButton, rememberButton](int) {
      m_limitButton->setEnabled(true);
      const QAbstractButton* const clickedButton = dialog->clickedButton();
      if (clickedButton == onceButton || clickedButton == rememberButton) {
        beginLimitUpdate(clickedButton == rememberButton);
      }
    });
    m_limitButton->setEnabled(false);
    dialog->open();
  }

  void beginLimitUpdate(const bool rememberLimit) {
    m_pendingLimitMb = m_limitInfo.requiredLimitMb;
    m_rememberLimit = rememberLimit;
    m_waitingForLimitUpdate = true;
    m_limitUpdateTimer.start();
    m_limitButton->setText(tr("Cancel"));
    m_limitButton->setToolTip(tr("Cancel waiting for active image loads before changing the limit."));
    m_explanation->setText(tr("Waiting for active image loads to finish..."));
    applyLimitWhenAvailable();
  }

  void cancelLimitUpdate() {
    m_waitingForLimitUpdate = false;
    m_limitButton->setText(tr("Review image allocation limit..."));
    m_limitButton->setToolTip(tr("Review the image allocation limit and optionally increase it for this application."));
    m_explanation->setText(m_initialExplanation);
  }

  void finishLimitUpdate(const QString& message) {
    m_waitingForLimitUpdate = false;
    m_limitButton->setText(tr("Review image allocation limit..."));
    m_limitButton->setToolTip(tr("Review the image allocation limit and optionally increase it for this application."));
    m_explanation->setText(message);
  }

  void applyLimitWhenAvailable() {
    if (!m_waitingForLimitUpdate) {
      return;
    }

    if (m_limitUpdateTimer.elapsed() >= kLimitUpdateTimeoutMs) {
      finishLimitUpdate(
          tr("Active image loads are still in progress. The allocation limit was not changed. You can try again."));
      return;
    }

    int effectiveLimitMb = 0;
    const ImageAllocationLimit::UpdateResult result
        = ImageAllocationLimit::trySetLimitMb(m_pendingLimitMb, &effectiveLimitMb);
    if (result == ImageAllocationLimit::UpdateResult::Busy) {
      QTimer::singleShot(kLimitUpdateRetryIntervalMs, this, [this]() { applyLimitWhenAvailable(); });
      return;
    }
    if (result == ImageAllocationLimit::UpdateResult::Invalid) {
      finishLimitUpdate(tr("The requested allocation limit is invalid and was not applied."));
      return;
    }
    if (effectiveLimitMb > 0 && effectiveLimitMb < m_pendingLimitMb) {
      finishLimitUpdate(tr("The effective allocation limit is still below the image's estimated requirement."));
      return;
    }
    QString completionMessage;
    if (!m_rememberLimit) {
      completionMessage = tr("The allocation limit was raised for this run. Reloading the image...");
    } else if (ApplicationSettings::getInstance().setImageAllocationLimitMb(m_pendingLimitMb)) {
      completionMessage = tr("The allocation limit was saved and applied. Reloading the image...");
    } else {
      completionMessage = tr(
          "The allocation limit was applied for this run, but could not be saved to application settings. "
          "Reloading the image...");
    }
    finishLimitUpdate(completionMessage);
    m_limitButton->setEnabled(false);
    emit reloadRequested();
  }

  QString m_filePath;
  ImageAllocationLimitInfo m_limitInfo;
  QString m_initialExplanation;
  QLabel* m_explanation = nullptr;
  QPushButton* m_limitButton = nullptr;
  QElapsedTimer m_limitUpdateTimer;
  int m_pendingLimitMb = 0;
  bool m_rememberLimit = false;
  bool m_waitingForLimitUpdate = false;
};

class LargeImageResult : public FilterResult {
  Q_DECLARE_TR_FUNCTIONS(LoadFileTask)

 public:
  LargeImageResult(const QString& filePath, const ImageAllocationLimitInfo& limitInfo)
      : m_filePath(filePath), m_limitInfo(limitInfo) {}

  void updateUI(FilterUiInterface* ui) override {
    ui->setImageWidget(new ErrorWidget(limitMessage()), ui->TRANSFER_OWNERSHIP);
    ui->setOptionsWidget(new LargeImageOptionsWidget(m_filePath, m_limitInfo), ui->TRANSFER_OWNERSHIP);
  }

  QString failureDescription() const override { return QString(); }

  QString skippedDescription() const override { return tr("The page was skipped: %1").arg(limitMessage()); }

  std::shared_ptr<AbstractFilter> filter() override { return nullptr; }

 private:
  QString limitMessage() const {
    if (!m_limitInfo.canRaiseLimit()) {
      return tr("This image is too large for the maximum configurable image allocation limit.");
    }
    if (ImageAllocationLimit::hasEnvironmentOverride()) {
      return tr(
          "This image exceeds the allocation limit set by QT_IMAGEIO_MAXALLOC. Change the environment "
          "variable and restart the application to load it.");
    }
    return tr(
        "This image is larger than the current image allocation limit. Review the limit in the options "
        "panel to allow loading it.");
  }

  QString m_filePath;
  ImageAllocationLimitInfo m_limitInfo;
};

class ThumbnailInvalidationResult : public FilterResult {
 public:
  ThumbnailInvalidationResult(FilterResultPtr result, PageId pageId)
      : m_result(std::move(result)), m_pageId(std::move(pageId)) {}

  void updateUI(FilterUiInterface* ui) override {
    m_result->updateUI(ui);
    ui->invalidateThumbnail(m_pageId);
  }

  QString failureDescription() const override { return m_result->failureDescription(); }

  QString skippedDescription() const override { return m_result->skippedDescription(); }

  std::shared_ptr<AbstractFilter> filter() override { return m_result->filter(); }

 private:
  FilterResultPtr m_result;
  PageId m_pageId;
};

}  // namespace


LoadFileTask::LoadFileTask(Type type,
                           const PageInfo& page,
                           std::shared_ptr<ThumbnailPixmapCache> thumbnailCache,
                           std::shared_ptr<ProjectPages> pages,
                           std::shared_ptr<fix_orientation::Task> nextTask)
    : BackgroundTask(type),
      m_thumbnailCache(std::move(thumbnailCache)),
      m_imageId(page.imageId()),
      m_pageId(page.id()),
      m_imageMetadata(page.metadata()),
      m_pages(std::move(pages)),
      m_nextTask(std::move(nextTask)) {
  assert(m_nextTask);
}

LoadFileTask::~LoadFileTask() = default;

FilterResultPtr LoadFileTask::operator()() {
  try {
    ImageAllocationLimitInfo limitInfo;
    QImage image = ImageLoader::load(m_imageId, &limitInfo);

    throwIfCancelled();

    if (image.isNull()) {
      if (limitInfo.exceeded) {
        return std::make_shared<LargeImageResult>(core::toQString(m_imageId.filePath()), limitInfo);
      }
      return std::make_shared<ErrorResult>(core::toQString(m_imageId.filePath()), limitInfo.dimensionsKnown);
    } else {
      convertToSupportedFormat(image);
      updateImageSizeIfChanged(image);
      overrideDpi(image);
      m_thumbnailCache->ensureThumbnailExists(m_imageId, image);
      FilterResultPtr result = m_nextTask->process(*this, FilterData(image));
      if (!result) {
        return nullptr;
      }
      return std::make_shared<ThumbnailInvalidationResult>(std::move(result), m_pageId);
    }
  } catch (const CancelledException&) {
    return nullptr;
  }
}

FilterResultPtr LoadFileTask::exceptionResult(const QString& message) {
  return std::make_shared<ErrorResult>(core::toQString(m_imageId.filePath()), message);
}

void LoadFileTask::updateImageSizeIfChanged(const QImage& image) {
  // The user might just replace a file with another one.
  // In that case, we update its size that we store.
  // Note that we don't do the same about DPI, because
  // a DPI mismatch between the image and the stored value
  // may indicate that the DPI was overridden.
  // TODO: do something about DPIs when we have the ability
  // to change DPIs at any point in time (not just when
  // creating a project).
  if (image.size() != m_imageMetadata.size()) {
    m_imageMetadata.setSize(image.size());
    m_pages->updateImageMetadata(m_imageId, m_imageMetadata);
  }
}

void LoadFileTask::overrideDpi(QImage& image) const {
  // Beware: QImage will have a default DPI when loading
  // an image that doesn't specify one.
  const Dpm dpm(m_imageMetadata.dpi());
  image.setDotsPerMeterX(dpm.horizontal());
  image.setDotsPerMeterY(dpm.vertical());
}

void LoadFileTask::convertToSupportedFormat(QImage& image) const {
  if (((image.format() == QImage::Format_Indexed8) && !image.isGrayscale()) || (image.depth() > 8)) {
    const QImage::Format fmt = image.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32;
    image = image.convertToFormat(fmt);
  } else {
    image = toGrayscale(image);
  }
}

/*======================= LoadFileTask::ErrorResult ======================*/

LoadFileTask::ErrorResult::ErrorResult(const QString& filePath, const bool dimensionsKnown)
    : m_filePath(QDir::toNativeSeparators(filePath)),
      m_fileExists(QFile::exists(filePath)),
      m_dimensionsKnown(dimensionsKnown) {}

LoadFileTask::ErrorResult::ErrorResult(const QString& filePath, const QString& errorDetails)
    : m_filePath(QDir::toNativeSeparators(filePath)),
      m_errorDetails(errorDetails),
      m_fileExists(QFile::exists(filePath)),
      m_dimensionsKnown(false),
      m_processingError(true) {}

void LoadFileTask::ErrorResult::updateUI(FilterUiInterface* ui) {
  class ErrWidget : public ErrorWidget {
   public:
    ErrWidget(std::shared_ptr<AbstractCommand<void>> relinkingDialogRequester,
              const QString& text,
              Qt::TextFormat fmt = Qt::AutoText)
        : ErrorWidget(text, fmt), m_relinkingDialogRequester(std::move(relinkingDialogRequester)) {}

   private:
    void linkActivated(const QString&) override { (*m_relinkingDialogRequester)(); }

    std::shared_ptr<AbstractCommand<void>> m_relinkingDialogRequester;
  };


  QString errMsg;
  Qt::TextFormat fmt = Qt::AutoText;
  if (m_processingError) {
    errMsg = failureDescription();
    fmt = Qt::PlainText;
  } else if (m_fileExists) {
    errMsg = failureDescription();
    fmt = Qt::PlainText;
  } else {
    errMsg = tr("The following file doesn't exist:<br>%1<br>"
                "<br>"
                "Use the <a href=\"#relink\">Relinking Tool</a> to locate it.")
                 .arg(m_filePath.toHtmlEscaped());
    fmt = Qt::RichText;
  }
  ui->setImageWidget(new ErrWidget(ui->relinkingDialogRequester(), errMsg, fmt), ui->TRANSFER_OWNERSHIP);
  ui->setOptionsWidget(new FilterOptionsWidget, ui->TRANSFER_OWNERSHIP);
}  // LoadFileTask::ErrorResult::updateUI

QString LoadFileTask::ErrorResult::failureDescription() const {
  if (m_processingError) {
    const QString details = m_errorDetails.isEmpty() ? tr("An unknown error occurred.") : m_errorDetails;
    return tr("An error occurred while processing this page:\n%1\n\nFile: %2").arg(details, m_filePath);
  }
  if (m_fileExists && m_dimensionsKnown) {
    return tr("The following file could not be loaded:\n%1").arg(m_filePath);
  }
  if (m_fileExists) {
    return tr("The image could not be loaded because its dimensions were unavailable for allocation checking:\n%1"
              "\nVerify that the file is valid. If you suspect it exceeds Qt's allocation limit, you can increase "
              "the saved image limit in Settings and restart.")
        .arg(m_filePath);
  }
  return tr("The following file doesn't exist:\n%1").arg(m_filePath);
}
