// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProjectOpeningContext.h"

#include <QMessageBox>
#include <QStringList>
#include <cassert>

#include "FixDpiDialog.h"
#include "ProjectPages.h"
#include "version.h"

ProjectOpeningContext::ProjectOpeningContext(QWidget* parent, const QString& projectFile, const QDomDocument& doc)
    : m_projectFile(projectFile), m_reader(doc, projectFile), m_parent(parent) {}

ProjectOpeningContext::~ProjectOpeningContext() {
  // Deleting a null pointer is OK.
  delete m_fixDpiDialog;
}

void ProjectOpeningContext::proceed() {
  if (!m_reader.success()) {
    deleteLater();
    if (!m_reader.getVersion().isNull() && (m_reader.getVersion().toInt() != PROJECT_VERSION)) {
      QMessageBox::warning(m_parent, tr("Error"),
                           tr("The project file is not compatible with the current application version."));
      return;
    }

    QMessageBox::critical(m_parent, tr("Error"), tr("Unable to interpret the project file."));
    return;
  }

  const ProjectMigrationReport& migrationReport = m_reader.migrationReport();
  if (migrationReport.sourceVersion == 3) {
    QStringList details;
    if (migrationReport.imagesWithDefaultTrim > 0) {
      details << tr("%1 image(s) have no <trim> data; defaults are enabled=0, left=0, top=0, right=0, bottom=0.")
                     .arg(migrationReport.imagesWithDefaultTrim);
    }
    if (migrationReport.colorSettingsWithDefaultFillOutsidePageBox > 0) {
      details << tr("%1 <color-or-grayscale> setting(s) have no fillOutsidePageBox value; the default is 0.")
                     .arg(migrationReport.colorSettingsWithDefaultFillOutsidePageBox);
    }
    if (migrationReport.obliqueModesDisabledForMissingAttributes > 0) {
      details << tr("%1 deskew page(s) have no oblique, oblique-mode, or autoOblique attribute; oblique correction "
                    "will remain off at angle 0.")
                     .arg(migrationReport.obliqueModesDisabledForMissingAttributes);
    }
    if (migrationReport.obliqueModesFromAutoOblique > 0) {
      details
          << tr("%1 oblique-mode value(s) will be derived from autoOblique: 1 means auto (%2), 0 means manual (%3).")
                 .arg(migrationReport.obliqueModesFromAutoOblique)
                 .arg(migrationReport.obliqueAutoModesFromAutoOblique)
                 .arg(migrationReport.obliqueManualModesFromAutoOblique);
    }
    if (migrationReport.obliqueModesFromDeskewMode > 0) {
      details << tr("%1 oblique-mode value(s) will be derived from mode: manual means manual (%2); all other or "
                    "missing values mean auto (%3).")
                     .arg(migrationReport.obliqueModesFromDeskewMode)
                     .arg(migrationReport.obliqueManualModesFromDeskewMode)
                     .arg(migrationReport.obliqueAutoModesFromDeskewMode);
    }
    if (migrationReport.pathsWrittenRelative > 0) {
      details << tr("%1 output/source path value(s) will be written as relative paths where possible; resolved "
                    "locations are preserved.")
                     .arg(migrationReport.pathsWrittenRelative);
    }
    if (details.isEmpty()) {
      details << tr("No missing format-4-specific fields with known defaults were found.");
    }

    const QString message(
        tr("This project uses format 3. Opening does not modify it, but saving will rewrite it as format %1. "
           "Before an existing format-3 file is overwritten, the original will be preserved as "
           "\"project-name.v3.bak\" (or \"project-name.v3.1.bak\", etc.). Use Save As to leave the original "
           "project untouched.\n\n%2\n\nContinue opening this project?")
            .arg(PROJECT_VERSION)
            .arg(details.join('\n')));
    QMessageBox warning(QMessageBox::Warning, tr("Project Format Upgrade"), message,
                        QMessageBox::Ok | QMessageBox::Cancel, m_parent);
    warning.setDefaultButton(QMessageBox::Cancel);
    warning.setEscapeButton(QMessageBox::Cancel);
    if (warning.exec() != QMessageBox::Ok) {
      deleteLater();
      return;
    }
  }

  if (m_reader.pages()->validateDpis()) {
    deleteLater();
    emit done(this);
    return;
  }

  showFixDpiDialog();
}

void ProjectOpeningContext::fixedDpiSubmitted() {
  m_reader.pages()->updateMetadataFrom(m_fixDpiDialog->files());
  emit done(this);
}

void ProjectOpeningContext::fixDpiDialogDestroyed() {
  deleteLater();
}

void ProjectOpeningContext::showFixDpiDialog() {
  assert(!m_fixDpiDialog);
  m_fixDpiDialog = new FixDpiDialog(m_reader.pages()->toImageFileInfo(), m_parent);
  m_fixDpiDialog->setAttribute(Qt::WA_DeleteOnClose);
  m_fixDpiDialog->setAttribute(Qt::WA_QuitOnClose, false);
  if (m_parent) {
    m_fixDpiDialog->setWindowModality(Qt::WindowModal);
  }
  connect(m_fixDpiDialog, SIGNAL(accepted()), this, SLOT(fixedDpiSubmitted()));
  connect(m_fixDpiDialog, SIGNAL(destroyed(QObject*)), this, SLOT(fixDpiDialogDestroyed()));
  m_fixDpiDialog->show();
}
