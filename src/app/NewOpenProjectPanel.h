// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_APP_NEWOPENPROJECTPANEL_H_
#define SCANTAILOR_APP_NEWOPENPROJECTPANEL_H_

#include <QWidget>
#include <memory>

#include "ui_NewOpenProjectPanel.h"

class QString;

class NewOpenProjectPanel : public QWidget, private Ui::NewOpenProjectPanel {
  Q_OBJECT
 public:
  explicit NewOpenProjectPanel(QWidget* parent = nullptr);

 signals:

  void newProject();

  void openProject();

  void openRecentProject(const QString& projectFile);

 protected:
  void paintEvent(QPaintEvent*) override;

 private:
  void addRecentProject(const QString& filePath);
};


#endif  // ifndef SCANTAILOR_APP_NEWOPENPROJECTPANEL_H_
