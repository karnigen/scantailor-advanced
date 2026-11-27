// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

class ZoneSet;
class Zone;
class PropertySet;

namespace output {
class PictureZoneComparator {
 public:
  static bool equal(const ZoneSet& lhs, const ZoneSet& rhs);

  static bool equal(const Zone& lhs, const Zone& rhs);

  static bool equal(const PropertySet& lhs, const PropertySet& rhs);
};
}  // namespace output
