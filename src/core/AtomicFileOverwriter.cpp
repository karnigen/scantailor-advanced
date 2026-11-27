// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "AtomicFileOverwriter.h"

#include <QFile>
#include <QTemporaryFile>

#include "Utils.h"

using namespace core;

AtomicFileOverwriter::AtomicFileOverwriter() = default;

AtomicFileOverwriter::~AtomicFileOverwriter() {
  abort();
}

QIODevice* AtomicFileOverwriter::startWriting(const QString& filePath) {
  abort();

  m_tempFile = std::make_unique<QTemporaryFile>(filePath);
  m_tempFile->setAutoRemove(false);
  if (!m_tempFile->open()) {
    m_tempFile.reset();
  }
  return m_tempFile.get();
}

bool AtomicFileOverwriter::commit() {
  if (!m_tempFile) {
    return false;
  }

  const QString tempFilePath(m_tempFile->fileName());
  const QString targetPath(m_tempFile->fileTemplate());

  // Yes, we have to destroy this object here, because:
  // 1. Under Windows, open files can't be renamed or deleted.
  // 2. QTemporaryFile::close() doesn't really close it.
  m_tempFile.reset();

  if (!Utils::overwritingRename(tempFilePath, targetPath)) {
    QFile::remove(tempFilePath);
    return false;
  }
  return true;
}

void AtomicFileOverwriter::abort() {
  if (!m_tempFile) {
    return;
  }

  const QString tempFilePath(m_tempFile->fileName());
  m_tempFile.reset();  // See comments in commit()
  QFile::remove(tempFilePath);
}
