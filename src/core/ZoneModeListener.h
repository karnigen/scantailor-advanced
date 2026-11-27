// SPDX-FileCopyrightText: 2020 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2020 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <core/zones/ZoneCreationMode.h>

class ZoneModeListener {
 public:
  virtual ~ZoneModeListener() = default;

  virtual void onZoneModeChanged(ZoneCreationMode mode) = 0;

  virtual void onZoneModeProviderStopped() = 0;
};
