// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

class QString;

namespace output {
enum DespeckleLevel { DESPECKLE_OFF, DESPECKLE_CAUTIOUS, DESPECKLE_NORMAL, DESPECKLE_AGGRESSIVE };

QString despeckleLevelToString(DespeckleLevel level);

DespeckleLevel despeckleLevelFromString(const QString& str);
}  // namespace output
