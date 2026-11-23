// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "TabbedDebugImages.h"

#include "DebugImageView.h"

TabbedDebugImages::TabbedDebugImages(QWidget* parent) : QTabWidget(parent) {
  setDocumentMode(true);
  connect(this, SIGNAL(currentChanged(int)), SLOT(currentTabChanged(int)));
}

void TabbedDebugImages::currentTabChanged(const int idx) {
  if (auto* div = dynamic_cast<DebugImageView*>(widget(idx))) {
    m_liveViews.remove_if([div](const QPointer<DebugImageView>& view) { return view.isNull() || view.data() == div; });
    m_liveViews.emplace_back(div);
    removeExcessLiveViews();
    div->setLive(true);
  }
}

void TabbedDebugImages::removeExcessLiveViews() {
  m_liveViews.remove_if([](const QPointer<DebugImageView>& view) { return view.isNull(); });
  while (static_cast<int>(m_liveViews.size()) > MAX_LIVE_VIEWS) {
    m_liveViews.front()->setLive(false);
    m_liveViews.pop_front();
  }
}
