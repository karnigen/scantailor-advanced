// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "OutOfMemoryDialog.h"

#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <utility>

#include <core/ApplicationSettings.h>
#include <core/PathUtils.h>
#include <core/SettingsStore.h>
#include "ProjectWriter.h"
#include "RecentProjects.h"

OutOfMemoryDialog::OutOfMemoryDialog(QWidget* parent) : QDialog(parent) {
  ui.setupUi(this);
  if (sizeof(void*) > 4) {
    ui.only_32bit_1->hide();
    ui.only_32bit_2->hide();
  }

  ui.topLevelStack->setCurrentWidget(ui.mainPage);

  connect(ui.saveProjectBtn, SIGNAL(clicked()), SLOT(saveProject()));
  connect(ui.saveProjectAsBtn, SIGNAL(clicked()), SLOT(saveProjectAs()));
  connect(ui.dontSaveBtn, SIGNAL(clicked()), SLOT(reject()));
}

void OutOfMemoryDialog::setParams(const std::filesystem::path& projectFile,
                                  const std::filesystem::path& initialDirectory,
                                  std::shared_ptr<StageSequence> stages,
                                  std::shared_ptr<ProjectPages> pages,
                                  const SelectedPage& selectedPage,
                                  const OutputFileNameGenerator& outFileNameGen) {
  m_projectFile = projectFile;
  m_initialDirectory = initialDirectory;
  m_stages = std::move(stages);
  m_pages = std::move(pages);
  m_selectedPage = selectedPage;
  m_outFileNameGen = outFileNameGen;

  ui.saveProjectBtn->setVisible(!projectFile.empty());
}

void OutOfMemoryDialog::saveProject() {
  if (m_projectFile.empty()) {
    saveProjectAs();
  } else if (saveProjectWithFeedback(m_projectFile)) {
    showSaveSuccessScreen();
  }
}

void OutOfMemoryDialog::saveProjectAs() {
  // XXX: this function is duplicated MainWindow

  QString projectDir;
  if (!m_projectFile.empty()) {
    projectDir = QFileInfo(core::toQString(m_projectFile)).absolutePath();
  } else {
    SettingsStore settings;
    projectDir = core::toQString(m_initialDirectory);
    if (projectDir.isEmpty() || !QDir(projectDir).exists()) {
      projectDir = settings.stringValue("project/lastDir");
    }
  }

  QString projectFile(QFileDialog::getSaveFileName(this, tr("Save Project"), projectDir,
                                                   tr("Scan Tailor Projects") + " (*.ScanTailor)"));
  if (projectFile.isEmpty()) {
    return;
  }

  if (!projectFile.endsWith(".ScanTailor", Qt::CaseInsensitive)) {
    projectFile += ".ScanTailor";
  }

  if (saveProjectWithFeedback(core::toFileSystemPath(projectFile))) {
    m_projectFile = core::toFileSystemPath(projectFile);
    showSaveSuccessScreen();

    SettingsStore settings;
    settings.setValue("project/lastDir", QFileInfo(core::toQString(m_projectFile)).absolutePath());

    RecentProjects rp;
    rp.read();
    rp.setMostRecent(m_projectFile);
    rp.write();
  }
}  // OutOfMemoryDialog::saveProjectAs

bool OutOfMemoryDialog::saveProjectWithFeedback(const std::filesystem::path& projectFile) {
  ProjectWriter writer(m_pages, m_selectedPage, m_outFileNameGen);

  QString errorMessage;
  if (!writer.write(projectFile, m_stages->filters(), &errorMessage,
                    ApplicationSettings::getInstance().getProjectBackupCount())) {
    const QString message = errorMessage.isEmpty() ? tr("Error saving the project file!")
                                                   : tr("Error saving the project file!\n\n%1").arg(errorMessage);
    QMessageBox::warning(this, tr("Error"), message);
    return false;
  }
  return true;
}

void OutOfMemoryDialog::showSaveSuccessScreen() {
  ui.topLevelStack->setCurrentWidget(ui.saveSuccessPage);
}
