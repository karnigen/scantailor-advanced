// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_OUTPUT_TABBEDIMAGEVIEW_H_
#define SCANTAILOR_OUTPUT_TABBEDIMAGEVIEW_H_

#include <QScrollBar>
#include <QTabWidget>
#include <QWidget>
#include <memory>
#include <unordered_map>

#include "ImageViewTab.h"

class ImageViewBase;
class QShortcut;

namespace output {
class TabbedImageView : public QTabWidget {
  Q_OBJECT
 private:
  using TabImageRectMap = std::unordered_map<ImageViewTab, QRectF, std::hash<int>>;

 public:
  explicit TabbedImageView(QWidget* parent = nullptr);

  void addTab(QWidget* widget, const QString& label, ImageViewTab tab);

  void setImageRectMap(std::unique_ptr<TabImageRectMap> tabImageRectMap);

 public slots:

  void setCurrentTab(ImageViewTab tab);

 signals:

  void tabChanged(ImageViewTab tab);

 private slots:

  void tabChangedSlot(int idx);

 private:
  void copyViewZoomAndPos(int oldIdx, int newIdx) const;

  QPointF getFocus(const QRectF& rect, const QScrollBar& horBar, const QScrollBar& verBar) const;

  void setFocus(QScrollBar& horBar, QScrollBar& verBar, const QRectF& rect, const QPointF& focal) const;

  std::unordered_map<QWidget*, ImageViewTab> m_registry;
  std::unique_ptr<TabImageRectMap> m_tabImageRectMap;
  int m_prevImageViewTabIndex;

  QShortcut* m_tabShortcuts[5];
};
}  // namespace output
#endif  // ifndef SCANTAILOR_OUTPUT_TABBEDIMAGEVIEW_H_
