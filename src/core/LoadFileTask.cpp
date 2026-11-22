// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "LoadFileTask.h"

#include <imageproc/Grayscale.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
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
#include "ProjectPages.h"
#include "ThumbnailPixmapCache.h"
#include "filters/fix_orientation/Task.h"

using namespace imageproc;

class LoadFileTask::ErrorResult : public FilterResult {
  Q_DECLARE_TR_FUNCTIONS(LoadFileTask)
 public:
  ErrorResult(const QString& filePath, bool dimensionsKnown);

  void updateUI(FilterUiInterface* ui) override;

  std::shared_ptr<AbstractFilter> filter() override { return nullptr; }

 private:
  QString m_filePath;
  bool m_fileExists;
  bool m_dimensionsKnown;
};

namespace {

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
      explanationText = tr("The image is too large for the maximum configurable allocation limit.");
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
    m_explanation = new QLabel(explanationText, this);
    m_explanation->setWordWrap(true);
    layout->addWidget(m_explanation);

    if (canOfferAllocationLimitIncrease(m_limitInfo)) {
      auto* button = new QPushButton(tr("Review image allocation limit..."), this);
      layout->addWidget(button);
      connect(button, &QPushButton::clicked, this, [this]() { requestLimitIncrease(); });
    }
    layout->addStretch();
  }

 private:
  void requestLimitIncrease() {
    if (!m_limitInfo.canRaiseLimit()) {
      QMessageBox::warning(window(), tr("Image Too Large"),
                           tr("The image dimensions exceed the maximum limit that can be configured."));
      return;
    }

    if (ImageAllocationLimit::hasEnvironmentOverride()) {
      QMessageBox::warning(
          window(), tr("Image Too Large"),
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

    QPointer<LargeImageOptionsWidget> self(this);
    QWidget* const dialogParent = window();
    QMessageBox dialog(QMessageBox::Warning, tr("Image Too Large"), message, QMessageBox::Cancel, dialogParent);
    QPushButton* const onceButton = dialog.addButton(tr("Allow for This Run"), QMessageBox::AcceptRole);
    QPushButton* const rememberButton = dialog.addButton(tr("Remember This Limit"), QMessageBox::AcceptRole);
    dialog.setDefaultButton(QMessageBox::Cancel);
    dialog.exec();

    if (!self) {
      return;
    }

    if (dialog.clickedButton() != onceButton && dialog.clickedButton() != rememberButton) {
      return;
    }

    const int requiredLimitMb = m_limitInfo.requiredLimitMb;
    const bool rememberLimit = dialog.clickedButton() == rememberButton;
    m_explanation->setText(tr("Waiting for active image loads to finish..."));
    setEnabled(false);
    applyLimitWhenAvailable(requiredLimitMb, rememberLimit);
  }

  void applyLimitWhenAvailable(const int requiredLimitMb, const bool rememberLimit) {
    int effectiveLimitMb = 0;
    if (!ImageAllocationLimit::trySetLimitMb(requiredLimitMb, &effectiveLimitMb)) {
      QTimer::singleShot(50, this, [this, requiredLimitMb, rememberLimit]() {
        applyLimitWhenAvailable(requiredLimitMb, rememberLimit);
      });
      return;
    }

    setEnabled(true);
    if (effectiveLimitMb > 0 && effectiveLimitMb < requiredLimitMb) {
      m_explanation->setText(
          tr("The requested allocation limit could not be applied. QT_IMAGEIO_MAXALLOC may override the limit."));
      return;
    }
    if (rememberLimit) {
      ApplicationSettings::getInstance().setImageAllocationLimitMb(requiredLimitMb);
    }
    emit reloadRequested();
  }

  QString m_filePath;
  ImageAllocationLimitInfo m_limitInfo;
  QLabel* m_explanation = nullptr;
};

class LargeImageResult : public FilterResult {
  Q_DECLARE_TR_FUNCTIONS(LoadFileTask)

 public:
  LargeImageResult(const QString& filePath, const ImageAllocationLimitInfo& limitInfo)
      : m_filePath(filePath), m_limitInfo(limitInfo) {}

  void updateUI(FilterUiInterface* ui) override {
    QString message;
    if (!m_limitInfo.canRaiseLimit()) {
      message = tr("This image is too large for the configured allocation limit.");
    } else if (ImageAllocationLimit::hasEnvironmentOverride()) {
      message = tr(
          "This image exceeds the allocation limit set by QT_IMAGEIO_MAXALLOC. Change the environment "
          "variable and restart the application to load it.");
    } else {
      message = tr(
          "This image is larger than the current image allocation limit. Review the limit in the options "
          "panel to allow loading it.");
    }
    ui->setImageWidget(new ErrorWidget(message), ui->TRANSFER_OWNERSHIP);
    ui->setOptionsWidget(new LargeImageOptionsWidget(m_filePath, m_limitInfo), ui->TRANSFER_OWNERSHIP);
  }

  std::shared_ptr<AbstractFilter> filter() override { return nullptr; }

 private:
  QString m_filePath;
  ImageAllocationLimitInfo m_limitInfo;
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
      m_imageMetadata(page.metadata()),
      m_pages(std::move(pages)),
      m_nextTask(std::move(nextTask)) {
  assert(m_nextTask);
}

LoadFileTask::~LoadFileTask() = default;

FilterResultPtr LoadFileTask::operator()() {
  ImageAllocationLimitInfo limitInfo;
  QImage image = ImageLoader::load(m_imageId, &limitInfo);

  try {
    throwIfCancelled();

    if (image.isNull()) {
      if (limitInfo.exceeded) {
        return std::make_shared<LargeImageResult>(m_imageId.filePath(), limitInfo);
      }
      return std::make_shared<ErrorResult>(m_imageId.filePath(), limitInfo.dimensionsKnown);
    } else {
      convertToSupportedFormat(image);
      updateImageSizeIfChanged(image);
      overrideDpi(image);
      m_thumbnailCache->ensureThumbnailExists(m_imageId, image);
      return m_nextTask->process(*this, FilterData(image));
    }
  } catch (const CancelledException&) {
    return nullptr;
  }
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
  if (m_fileExists) {
    if (m_dimensionsKnown) {
      errMsg = tr("The following file could not be loaded:\n%1").arg(m_filePath);
    } else {
      errMsg = tr("The image could not be loaded because its dimensions were unavailable for allocation checking:\n%1"
                  "\nIf you trust this file, you can increase the saved image limit in Settings and restart.")
                   .arg(m_filePath);
    }
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
