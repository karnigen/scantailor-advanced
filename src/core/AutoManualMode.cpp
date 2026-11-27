// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "AutoManualMode.h"

QString autoManualModeToString(AutoManualMode mode) {
  QString str;
  switch (mode) {
    case MODE_AUTO:
      str = "auto";
      break;
    case MODE_MANUAL:
      str = "manual";
      break;
    case MODE_DISABLED:
      str = "disabled";
      break;
  }
  return str;
}

AutoManualMode stringToAutoManualMode(const QString& str) {
  if (str == "disabled") {
    return MODE_DISABLED;
  } else if (str == "manual") {
    return MODE_MANUAL;
  } else {
    return MODE_AUTO;
  }
}