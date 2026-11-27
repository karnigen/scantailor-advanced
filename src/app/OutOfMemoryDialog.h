// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_APP_OUTOFMEMORYDIALOG_H_
#define SCANTAILOR_APP_OUTOFMEMORYDIALOG_H_

#include <QDialog>
#include <QString>
#include <filesystem>
#include <memory>

#include "OutputFileNameGenerator.h"
#include "ProjectPages.h"
#include "SelectedPage.h"
#include "StageSequence.h"
#include "ui_OutOfMemoryDialog.h"

class OutOfMemoryDialog : public QDialog {
  Q_OBJECT
 public:
  explicit OutOfMemoryDialog(QWidget* parent = nullptr);

  void setParams(const std::filesystem::path& projectFile,
                 const std::filesystem::path& initialDirectory,
                 // may be empty
                 std::shared_ptr<StageSequence> stages,
                 std::shared_ptr<ProjectPages> pages,
                 const SelectedPage& selectedPage,
                 const OutputFileNameGenerator& outFileNameGen);

 private slots:

  void saveProject();

  void saveProjectAs();

 private:
  bool saveProjectWithFeedback(const std::filesystem::path& projectFile);

  void showSaveSuccessScreen();

  Ui::OutOfMemoryDialog ui;
  std::filesystem::path m_projectFile;
  std::filesystem::path m_initialDirectory;
  std::shared_ptr<StageSequence> m_stages;
  std::shared_ptr<ProjectPages> m_pages;
  SelectedPage m_selectedPage;
  OutputFileNameGenerator m_outFileNameGen;
};


#endif  // ifndef SCANTAILOR_APP_OUTOFMEMORYDIALOG_H_
