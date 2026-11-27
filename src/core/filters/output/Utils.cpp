// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "Utils.h"

#include <core/DefaultParams.h>
#include <core/DefaultParamsProvider.h>

#include <QString>
#include <QTransform>

#include "Dpi.h"
#include "Params.h"

namespace output {
std::filesystem::path Utils::automaskDir(const std::filesystem::path& outDir) {
  return std::filesystem::absolute(outDir / "cache" / "automask");
}

std::filesystem::path Utils::predespeckleDir(const std::filesystem::path& outDir) {
  return std::filesystem::absolute(outDir / "cache" / "predespeckle");
}

std::filesystem::path Utils::specklesDir(const std::filesystem::path& outDir) {
  return std::filesystem::absolute(outDir / "cache" / "speckles");
}

QTransform Utils::scaleFromToDpi(const Dpi& from, const Dpi& to) {
  QTransform xform;
  xform.scale((double) to.horizontal() / from.horizontal(), (double) to.vertical() / from.vertical());
  return xform;
}

std::filesystem::path Utils::foregroundDir(const std::filesystem::path& outDir) {
  return std::filesystem::absolute(outDir / "foreground");
}

std::filesystem::path Utils::backgroundDir(const std::filesystem::path& outDir) {
  return std::filesystem::absolute(outDir / "background");
}

std::filesystem::path Utils::originalBackgroundDir(const std::filesystem::path& outDir) {
  return std::filesystem::absolute(outDir / "original_background");
}

QTransform Utils::rotate(double degrees, const QRect& imageRect) {
  if (degrees == 0.0) {
    return QTransform();
  }

  QTransform rotateXform;
  const QPointF origin = QRectF(imageRect).center();
  rotateXform.translate(-origin.x(), -origin.y());
  rotateXform *= QTransform().rotate(degrees);
  rotateXform *= QTransform().translate(origin.x(), origin.y());
  return rotateXform;
}

Params Utils::buildDefaultParams() {
  const DefaultParams& defaultParams = DefaultParamsProvider::getInstance().getParams();
  const DefaultParams::OutputParams& outputParams = defaultParams.getOutputParams();

  return Params(outputParams.getDpi(), outputParams.getColorParams(), outputParams.getSplittingOptions(),
                outputParams.getPictureShapeOptions(), dewarping::DistortionModel(), outputParams.getDepthPerception(),
                outputParams.getDewarpingOptions(), outputParams.getDespeckleLevel());
}
}  // namespace output