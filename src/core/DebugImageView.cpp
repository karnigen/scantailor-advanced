// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "DebugImageView.h"

#include <QPointer>
#include <utility>

#include "AbstractCommand.h"
#include "BackgroundExecutor.h"
#include "BasicImageView.h"
#include "ErrorWidget.h"
#include "ImageViewBase.h"
#include "PathUtils.h"
#include "ProcessingIndicationWidget.h"

class DebugImageView::ImageLoadResult : public AbstractCommand<void> {
 public:
  ImageLoadResult(QPointer<DebugImageView> owner, const quint64 generation, const QImage& image)
      : m_owner(std::move(owner)), m_generation(generation), m_image(image) {}

  // This method is called from the main thread.
  void operator()() override {
    if (DebugImageView* owner = m_owner) {
      owner->imageLoaded(m_generation, m_image);
    }
  }

 private:
  QPointer<DebugImageView> m_owner;
  quint64 m_generation;
  QImage m_image;
};


class DebugImageView::ImageLoader : public BackgroundExecutor::Task {
 public:
  ImageLoader(DebugImageView* owner, const quint64 generation, const std::filesystem::path& filePath)
      : m_owner(owner), m_generation(generation), m_filePath(filePath) {}

  BackgroundExecutor::TaskResultPtr operator()() override {
    QImage image(core::toQString(m_filePath));
    return std::make_shared<ImageLoadResult>(m_owner, m_generation, image);
  }

  void handleException(const QString& message) override {
    if (DebugImageView* owner = m_owner) {
      owner->imageLoadFailed(m_generation, message);
    }
  }

 private:
  QPointer<DebugImageView> m_owner;
  quint64 m_generation;
  std::filesystem::path m_filePath;
};


DebugImageView::DebugImageView(AutoRemovingFile file,
                               const std::function<QWidget*(const QImage&)>& imageViewFactory,
                               QWidget* parent)
    : QStackedWidget(parent),
      m_file(file),
      m_imageViewFactory(imageViewFactory),
      m_placeholderWidget(new ProcessingIndicationWidget(this)),
      m_isLive(false) {
  addWidget(m_placeholderWidget);
}

void DebugImageView::setLive(const bool live) {
  if (live && !m_isLive) {
    ++m_loadGeneration;
    ImageViewBase::backgroundExecutor().enqueueTask(
        std::make_shared<ImageLoader>(this, m_loadGeneration, m_file.get()));
  } else if (!live && m_isLive) {
    ++m_loadGeneration;
    if (QWidget* wgt = currentWidget()) {
      if (wgt != m_placeholderWidget) {
        removeWidget(wgt);
        delete wgt;
      }
    }
  }

  m_isLive = live;
}

void DebugImageView::imageLoaded(const quint64 generation, const QImage& image) {
  if (!m_isLive || generation != m_loadGeneration) {
    return;
  }

  if (currentWidget() == m_placeholderWidget) {
    std::unique_ptr<QWidget> imageView;
    if (!m_imageViewFactory) {
      imageView = std::make_unique<BasicImageView>(image);
    } else {
      imageView.reset(m_imageViewFactory(image));
    }
    setCurrentIndex(addWidget(imageView.release()));
  }
}

void DebugImageView::imageLoadFailed(const quint64 generation, const QString& message) {
  if (!m_isLive || generation != m_loadGeneration || currentWidget() != m_placeholderWidget) {
    return;
  }

  const QString details = message.isEmpty() ? tr("An unknown error occurred.") : message;
  auto* const errorWidget = new ErrorWidget(tr("Failed to load debug image:\n%1").arg(details), Qt::PlainText);
  setCurrentIndex(addWidget(errorWidget));
}
