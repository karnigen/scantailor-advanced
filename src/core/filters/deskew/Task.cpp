// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Task.h"

#include <BinaryImage.h>
#include <BlackOnWhiteEstimator.h>
#include <Morphology.h>
#include <OrthogonalRotation.h>
#include <RasterOp.h>
#include <ReduceThreshold.h>
#include <SeedFill.h>
#include <SkewFinder.h>
#include <UnitsProvider.h>
#include <UpscaleIntegerTimes.h>
#include <core/ApplicationSettings.h>
#include <core/DefaultParams.h>
#include <core/DefaultParamsProvider.h>
#include <imageproc/Grayscale.h>
#include <imageproc/OrthogonalRotation.h>
#include <imageproc/PolygonRasterizer.h>
#include <imageproc/Transform.h>

#include <QPolygonF>
#include <QTransform>
#include <utility>

#include "DebugImagesImpl.h"
#include "Dpm.h"
#include "Filter.h"
#include "FilterData.h"
#include "FilterUiInterface.h"
#include "ImageView.h"
#include "ObliqueFinder.h"
#include "OptionsWidget.h"
#include "TaskStatus.h"
#include "filters/select_content/Task.h"

namespace deskew {
using namespace imageproc;

class Task::UiUpdater : public FilterResult {
 public:
  UiUpdater(std::shared_ptr<Filter> filter,
            std::unique_ptr<DebugImages> dbgImg,
            const QImage& image,
            const PageId& pageId,
            const ImageTransformation& xform,
            const OptionsWidget::UiData& uiData,
            bool batchProcessing);

  void updateUI(FilterUiInterface* ui) override;

  std::shared_ptr<AbstractFilter> filter() override { return m_filter; }

 private:
  std::shared_ptr<Filter> m_filter;
  std::unique_ptr<DebugImages> m_dbg;
  QImage m_image;
  QImage m_downscaledImage;
  PageId m_pageId;
  ImageTransformation m_xform;
  OptionsWidget::UiData m_uiData;
  bool m_batchProcessing;
};


Task::Task(std::shared_ptr<Filter> filter,
           std::shared_ptr<Settings> settings,
           std::shared_ptr<ImageSettings> imageSettings,
           std::shared_ptr<select_content::Task> nextTask,
           const PageId& pageId,
           const bool batchProcessing,
           const bool debug)
    : m_filter(std::move(filter)),
      m_settings(std::move(settings)),
      m_imageSettings(std::move(imageSettings)),
      m_nextTask(std::move(nextTask)),
      m_pageId(pageId),
      m_batchProcessing(batchProcessing) {
  if (debug) {
    m_dbg = std::make_unique<DebugImagesImpl>();
  }
}

Task::~Task() = default;

FilterResultPtr Task::process(const TaskStatus& status, FilterData data) {
  status.throwIfCancelled();

  const bool algoContentBased = m_settings->algoContentBased();
  const Dependencies deps(data.xform().preCropArea(), data.xform().preRotation(), algoContentBased);

  std::unique_ptr<Params> params(m_settings->getPageParams(m_pageId));
  const Settings::RecomputeRequest pendingRecompute = m_settings->pendingRecompute(m_pageId);
  std::optional<Params> priorParamsBeforeRecompute;
  updateFilterData(status, data, (!params || !deps.matches(params->dependencies())));

  OptionsWidget::UiData uiData;
  uiData.setDependencies(deps);

  bool recomputeDeskew = false;
  bool recomputeOblique = false;
  if (params) {
    if (!deps.matches(params->dependencies())) {
      recomputeDeskew = recomputeDeskew || (params->mode() == MODE_AUTO);
      recomputeOblique = recomputeOblique || (params->obliqueMode() == MODE_AUTO);
    }
    recomputeDeskew = recomputeDeskew || (pendingRecompute.deskew && params->mode() == MODE_AUTO);
    recomputeOblique = recomputeOblique || (pendingRecompute.oblique && params->obliqueMode() == MODE_AUTO);

    if (recomputeDeskew || recomputeOblique) {
      priorParamsBeforeRecompute = *params;
      params.reset();
    } else {
      uiData.setEffectiveDeskewAngle(params->deskewAngle());
      uiData.setEffectiveObliqueAngle(params->obliqueAngle());
      uiData.setMode(params->mode());
      uiData.setObliqueMode(params->obliqueMode());

      status.throwIfCancelled();
      Params newParams(uiData.effectiveDeskewAngle(), uiData.effectiveObliqueAngle(), deps, uiData.mode(),
                       uiData.obliqueMode());
      m_settings->setPageParams(m_pageId, newParams);
      m_settings->clearRecomputeRequest(m_pageId, pendingRecompute.generation);
    }
  }

  if (!params) {
    const DefaultParams::DeskewParams& defaultDeskew
        = DefaultParamsProvider::getInstance().getParams().getDeskewParams();
    double deskewAngle
        = priorParamsBeforeRecompute ? priorParamsBeforeRecompute->deskewAngle() : defaultDeskew.getDeskewAngleDeg();
    AutoManualMode deskewMode
        = priorParamsBeforeRecompute ? priorParamsBeforeRecompute->mode() : defaultDeskew.getMode();
    double obliqueAngle = priorParamsBeforeRecompute ? priorParamsBeforeRecompute->obliqueAngle() : 0.0;
    AutoManualMode obliqueMode = priorParamsBeforeRecompute
                                     ? priorParamsBeforeRecompute->obliqueMode()
                                     : (defaultDeskew.isAutoOblique() ? MODE_AUTO : MODE_DISABLED);

    if (!priorParamsBeforeRecompute) {
      recomputeDeskew = (deskewMode == MODE_AUTO);
      recomputeOblique = (obliqueMode == MODE_AUTO);
    }
    recomputeDeskew = recomputeDeskew || (pendingRecompute.deskew && deskewMode == MODE_AUTO);
    recomputeOblique = recomputeOblique || (pendingRecompute.oblique && obliqueMode == MODE_AUTO);
    if (recomputeDeskew && obliqueMode == MODE_AUTO) {
      recomputeOblique = true;
    }
    if (obliqueMode == MODE_DISABLED) {
      obliqueAngle = 0.0;
      recomputeOblique = false;
    }

    if ((recomputeDeskew && deskewMode == MODE_AUTO) || (recomputeOblique && obliqueMode == MODE_AUTO)) {
      const QRectF imageArea(data.xform().transformBack().mapRect(data.xform().resultingRect()));
      const QRect boundedImageArea(imageArea.toRect().intersected(data.origImage().rect()));

      status.throwIfCancelled();

      // Keep the previous/default angles when no source pixels are available.
      if (boundedImageArea.isValid()) {
        BinaryImage rotatedImage(orthogonalRotation(
            BinaryImage(data.grayImageBlackOnWhite(), boundedImageArea, data.bwThresholdBlackOnWhite()),
            data.xform().preRotation().toDegrees()));
        if (m_dbg) {
          m_dbg->add(rotatedImage, "bw_rotated");
        }

        const QSize unrotatedDpm(Dpm(data.origImage()).toSize());
        const Dpm rotatedDpm(data.xform().preRotation().rotate(unrotatedDpm));
        cleanup(status, rotatedImage, Dpi(rotatedDpm));
        if (m_dbg) {
          m_dbg->add(rotatedImage, "after_cleanup");
        }

        status.throwIfCancelled();

        SkewFinder skewFinder;
        skewFinder.setResolutionRatio((double) rotatedDpm.horizontal() / rotatedDpm.vertical());
        if (recomputeDeskew && deskewMode == MODE_AUTO) {
          const Skew skew(algoContentBased ? skewFinder.findSkew(rotatedImage)
                                           : skewFinder.findSkewFromTopEdge(rotatedImage));
          deskewAngle = skew.confidence() >= Skew::GOOD_CONFIDENCE ? -skew.angle() : 0.0;
        }

        if (recomputeOblique && obliqueMode == MODE_AUTO) {
          // Estimate oblique on the deskewed mask for better accuracy (PR #110 feedback).
          BinaryImage horizontalMask;
          if (std::abs(deskewAngle) < 1e-6) {
            horizontalMask = rotatedImage;
          } else {
            const int w = rotatedImage.width();
            const int h = rotatedImage.height();
            QTransform rotXform;
            const QPointF center(0.5 * w, 0.5 * h);
            rotXform.translate(-center.x(), -center.y());
            rotXform.rotate(deskewAngle);
            rotXform.translate(center.x(), center.y());
            const QRectF srcRectF(0, 0, w, h);
            QPolygonF dstCorners;
            dstCorners << rotXform.map(srcRectF.topLeft()) << rotXform.map(srcRectF.topRight())
                       << rotXform.map(srcRectF.bottomRight()) << rotXform.map(srcRectF.bottomLeft());
            const QRect dstRect = dstCorners.boundingRect().toAlignedRect();
            if (dstRect.isValid()) {
              const QImage srcQ = rotatedImage.toQImage();
              const QImage dstQ = transform(srcQ, rotXform, dstRect, OutsidePixels::assumeWeakColor(Qt::white));
              horizontalMask = BinaryImage(dstQ);
            } else {
              horizontalMask = rotatedImage;
            }
          }
          const std::optional<double> obliqueDeg = findObliqueDegrees(horizontalMask, skewFinder, 5.0);
          obliqueAngle = obliqueDeg ? -*obliqueDeg : 0.0;
        }
      }
    }

    uiData.setEffectiveDeskewAngle(deskewAngle);
    uiData.setEffectiveObliqueAngle(obliqueAngle);
    uiData.setMode(deskewMode);
    uiData.setObliqueMode(obliqueMode);

    status.throwIfCancelled();
    Params newParams(deskewAngle, obliqueAngle, deps, deskewMode, obliqueMode);
    m_settings->setPageParams(m_pageId, newParams);
    m_settings->clearRecomputeRequest(m_pageId, pendingRecompute.generation);
  }

  ImageTransformation newXform(data.xform());
  newXform.setPostRotation(uiData.effectiveDeskewAngle());
  newXform.setPostOblique(uiData.effectiveObliqueAngle());

  if (m_nextTask) {
    return m_nextTask->process(status, FilterData(data, newXform));
  } else {
    return std::make_shared<UiUpdater>(m_filter, std::move(m_dbg), data.origImage(), m_pageId, newXform, uiData,
                                       m_batchProcessing);
  }
}  // Task::process

void Task::cleanup(const TaskStatus& status, BinaryImage& image, const Dpi& dpi) {
  // We don't have to clean up every piece of garbage.
  // The only concern are the horizontal shadows, which we remove here.

  Dpi reducedDpi(dpi);
  BinaryImage reducedImage;

  {
    ReduceThreshold reductor(image);
    while (reducedDpi.horizontal() >= 200 && reducedDpi.vertical() >= 200) {
      reductor.reduce(2);
      reducedDpi = Dpi(reducedDpi.horizontal() / 2, reducedDpi.vertical() / 2);
    }
    reducedImage = reductor.image();
  }

  status.throwIfCancelled();

  const QSize brick(from150dpi(QSize(200, 14), reducedDpi));
  BinaryImage opened(openBrick(reducedImage, brick, BLACK));
  reducedImage.release();

  status.throwIfCancelled();

  BinaryImage seed(upscaleIntegerTimes(opened, image.size(), WHITE));
  opened.release();

  status.throwIfCancelled();

  BinaryImage garbage(seedFill(seed, image, CONN8));
  seed.release();

  status.throwIfCancelled();

  rasterOp<RopSubtract<RopDst, RopSrc>>(image, garbage);
}  // Task::cleanup

int Task::from150dpi(int size, int targetDpi) {
  const int newSize = (size * targetDpi + 75) / 150;
  if (newSize < 1) {
    return 1;
  }
  return newSize;
}

QSize Task::from150dpi(const QSize& size, const Dpi& targetDpi) {
  const int width = from150dpi(size.width(), targetDpi.horizontal());
  const int height = from150dpi(size.height(), targetDpi.vertical());
  return QSize(width, height);
}

void Task::updateFilterData(const TaskStatus& status, FilterData& data, bool needUpdate) {
  const std::unique_ptr<ImageSettings::PageParams> params = m_imageSettings->getPageParams(m_pageId);
  if (!needUpdate && params) {
    data.updateImageParams(*params);
  } else {
    const GrayImage& img = data.grayImage();
    BinaryImage mask(img.size(), BLACK);
    PolygonRasterizer::fillExcept(mask, WHITE, data.xform().resultingPreCropArea(), Qt::WindingFill);
    bool isBlackOnWhite = true;
    if (ApplicationSettings::getInstance().isBlackOnWhiteDetectionEnabled()) {
      isBlackOnWhite = BlackOnWhiteEstimator::isBlackOnWhite(data.grayImage(), data.xform(), status, m_dbg.get());
    }
    ImageSettings::PageParams newParams(BinaryThreshold::otsuThreshold(GrayscaleHistogram(img, mask)), isBlackOnWhite);

    m_imageSettings->setPageParams(m_pageId, newParams);
    data.updateImageParams(newParams);
  }
}

/*============================ Task::UiUpdater ==========================*/

Task::UiUpdater::UiUpdater(std::shared_ptr<Filter> filter,
                           std::unique_ptr<DebugImages> dbgImg,
                           const QImage& image,
                           const PageId& pageId,
                           const ImageTransformation& xform,
                           const OptionsWidget::UiData& uiData,
                           const bool batchProcessing)
    : m_filter(std::move(filter)),
      m_dbg(std::move(dbgImg)),
      m_image(image),
      m_downscaledImage(ImageView::createDownscaledImage(image)),
      m_pageId(pageId),
      m_xform(xform),
      m_uiData(uiData),
      m_batchProcessing(batchProcessing) {}

void Task::UiUpdater::updateUI(FilterUiInterface* ui) {
  // This function is executed from the GUI thread.
  ui->invalidateThumbnail(m_pageId);

  if (m_batchProcessing) {
    return;
  }

  OptionsWidget* const optWidget = m_filter->optionsWidget();
  optWidget->postUpdateUI(m_uiData);
  ui->setOptionsWidget(optWidget, ui->KEEP_OWNERSHIP);

  auto* view = new ImageView(m_image, m_downscaledImage, m_xform);
  ui->setImageWidget(view, ui->TRANSFER_OWNERSHIP, m_dbg.get());

  QObject::connect(view, SIGNAL(manualDeskewAngleSet(double)), optWidget, SLOT(manualDeskewAngleSetExternally(double)));
  QObject::connect(optWidget, SIGNAL(manualDeskewAngleSet(double)), view, SLOT(manualDeskewAngleSetExternally(double)));
  QObject::connect(view, SIGNAL(manualObliqueAngleSet(double)), optWidget,
                   SLOT(manualObliqueAngleSetExternally(double)));
  QObject::connect(optWidget, SIGNAL(manualObliqueAngleSet(double)), view,
                   SLOT(manualObliqueAngleSetExternally(double)));
}
}  // namespace deskew
