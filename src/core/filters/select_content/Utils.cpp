// SPDX-FileCopyrightText: 2020 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2020 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "Utils.h"

#include <core/DefaultParams.h>
#include <core/DefaultParamsProvider.h>
#include <core/UnitsConverter.h>

#include "Params.h"

using namespace select_content;

Params Utils::buildDefaultParams(const Dpi& dpi) {
  const DefaultParams& defaultParams = DefaultParamsProvider::getInstance().getParams();
  const DefaultParams::SelectContentParams& selectContentParams = defaultParams.getSelectContentParams();

  const UnitsConverter unitsConverter(dpi);

  const QSizeF& pageRectSize = selectContentParams.getPageRectSize();
  double pageRectWidth = pageRectSize.width();
  double pageRectHeight = pageRectSize.height();
  unitsConverter.convert(pageRectWidth, pageRectHeight, defaultParams.getUnits(), PIXELS);

  return Params(QRectF(), QSizeF(), QRectF(QPointF(0, 0), QSizeF(pageRectWidth, pageRectHeight)), Dependencies(),
                selectContentParams.isContentDetectEnabled() ? MODE_AUTO : MODE_DISABLED,
                selectContentParams.getPageDetectMode(), selectContentParams.isFineTuneCorners());
}
