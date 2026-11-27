// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_AUTOMANUALMODE_H_
#define SCANTAILOR_CORE_AUTOMANUALMODE_H_

#include <QtCore/QString>

enum AutoManualMode { MODE_AUTO, MODE_MANUAL, MODE_DISABLED };

QString autoManualModeToString(AutoManualMode mode);

AutoManualMode stringToAutoManualMode(const QString& str);

#endif  // SCANTAILOR_CORE_AUTOMANUALMODE_H_
