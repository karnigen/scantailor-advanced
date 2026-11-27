// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_UNITS_H_
#define SCANTAILOR_CORE_UNITS_H_


#include <QtCore/QString>

enum Units { PIXELS, MILLIMETRES, CENTIMETRES, INCHES };

QString unitsToString(Units units);

Units unitsFromString(const QString& string);

QString unitsToLocalizedString(Units units);

#endif  // SCANTAILOR_CORE_UNITS_H_
