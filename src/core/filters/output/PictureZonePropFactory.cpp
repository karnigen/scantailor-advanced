// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "PictureZonePropFactory.h"

#include "PictureLayerProperty.h"
#include "ZoneCategoryProperty.h"

namespace output {
PictureZonePropFactory::PictureZonePropFactory() {
  PictureLayerProperty::registerIn(*this);
  ZoneCategoryProperty::registerIn(*this);
}
}  // namespace output
