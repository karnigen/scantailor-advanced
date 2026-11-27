// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_APP_THUMBNAILVIEWMODE_H_
#define SCANTAILOR_APP_THUMBNAILVIEWMODE_H_

#include <QAbstractButton>

inline void setSingleColumnViewButtonState(QAbstractButton& button, bool enabled) {
  button.setChecked(enabled);
}

#endif  // SCANTAILOR_APP_THUMBNAILVIEWMODE_H_
