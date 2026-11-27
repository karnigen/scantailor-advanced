// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_APP_RELINKINGLISTVIEW_H_
#define SCANTAILOR_APP_RELINKINGLISTVIEW_H_

#include <QTreeView>

class RelinkingListView : public QTreeView {
 public:
  explicit RelinkingListView(QWidget* parent = nullptr);
};


#endif  // ifndef SCANTAILOR_APP_RELINKINGLISTVIEW_H_
