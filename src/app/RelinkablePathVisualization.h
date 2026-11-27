// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>
#include <QWidget>

class RelinkablePath;
class QHBoxLayout;
class QAbstractButton;
class QEvent;
class QObject;

class RelinkablePathVisualization : public QWidget {
  Q_OBJECT
 public:
  explicit RelinkablePathVisualization(QWidget* parent = nullptr);

  void clear();

  void setPath(const RelinkablePath& path, bool clickable);

 signals:

  /** \p type is either RelinkablePath::File or RelinkablePath::Dir */
  void clicked(const QString& prefixPath, const QString& suffixPath, int type);

 protected:
  void paintEvent(QPaintEvent* evt) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

 private:
  struct PathComponent;

  class ComponentButton;

  void onClicked(int componentIdx, const QString& prefixPath, const QString& suffixPath, int type);

  void refreshHoverHighlight();

  void stylePathComponentButton(QAbstractButton* btn, bool exists);

  static void checkForExistence(std::vector<PathComponent>& components);

  QHBoxLayout* m_layout;
  bool m_hoverSuppressedUntilMove = false;
};
