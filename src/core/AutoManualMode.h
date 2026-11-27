// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QtCore/QString>

enum AutoManualMode { MODE_AUTO, MODE_MANUAL, MODE_DISABLED };

QString autoManualModeToString(AutoManualMode mode);

AutoManualMode stringToAutoManualMode(const QString& str);
