// Copyright (C) 2020  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "Utils.h"

#include <core/DefaultParams.h>
#include <core/DefaultParamsProvider.h>

#include "Params.h"

using namespace deskew;

Params Utils::buildDefaultParams() {
  const DefaultParams& defaultParams = DefaultParamsProvider::getInstance().getParams();
  return buildDefaultParams(defaultParams.getDeskewParams());
}

Params Utils::buildDefaultParams(const DefaultParams::DeskewParams& deskewParams) {
  const AutoManualMode obliqueMode = deskewParams.isAutoOblique() ? MODE_AUTO : MODE_DISABLED;
  return Params(deskewParams.getDeskewAngleDeg(), 0.0, Dependencies(), deskewParams.getMode(), obliqueMode);
}
