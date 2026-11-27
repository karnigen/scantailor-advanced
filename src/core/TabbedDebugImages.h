// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_TABBEDDEBUGIMAGES_H_
#define SCANTAILOR_CORE_TABBEDDEBUGIMAGES_H_

#include <QPointer>
#include <QTabWidget>
#include <list>

#include "DebugImageView.h"

class TabbedDebugImages : public QTabWidget {
  Q_OBJECT
 public:
  explicit TabbedDebugImages(QWidget* parent = nullptr);

 private slots:

  void currentTabChanged(int idx);

 private:
  using DebugViewList = std::list<QPointer<DebugImageView>>;

  enum { MAX_LIVE_VIEWS = 3 };

  void removeExcessLiveViews();

  /**
   * We don't want to keep all the debug images in memory.  We normally keep
   * only a few of them.  This list holds references to them in the order
   * they become live.
   */
  DebugViewList m_liveViews;
};


#endif  // ifndef SCANTAILOR_CORE_TABBEDDEBUGIMAGES_H_
