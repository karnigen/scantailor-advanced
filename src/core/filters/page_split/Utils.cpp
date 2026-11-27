// SPDX-FileCopyrightText: 2020 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2020 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "Utils.h"

#include <core/DefaultParams.h>
#include <core/DefaultParamsProvider.h>

using namespace page_split;

Settings::UpdateAction Utils::buildDefaultUpdateAction() {
  const DefaultParams& defaultParams = DefaultParamsProvider::getInstance().getParams();
  const DefaultParams::PageSplitParams& pageSplitParams = defaultParams.getPageSplitParams();

  Settings::UpdateAction update;
  update.setLayoutType(pageSplitParams.getLayoutType());

  return update;
}
