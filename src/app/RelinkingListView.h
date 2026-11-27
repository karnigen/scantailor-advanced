// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_APP_RELINKINGLISTVIEW_H_
#define SCANTAILOR_APP_RELINKINGLISTVIEW_H_

#include <QTreeView>

class RelinkingListView : public QTreeView {
 public:
  explicit RelinkingListView(QWidget* parent = nullptr);
};


#endif  // ifndef SCANTAILOR_APP_RELINKINGLISTVIEW_H_
