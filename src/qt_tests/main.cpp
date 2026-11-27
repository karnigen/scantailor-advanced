// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "tst_core_qt.h"
#include "tst_mainwindow.h"
#include "tst_ux_qt.h"

#include <core/Application.h>
#include <core/SettingsStore.h>

#include <QTemporaryDir>
#include <QtTest/QtTest>

int main(int argc, char* argv[]) {
  Application app(argc, argv);
  QTemporaryDir configDirectory;
  if (!configDirectory.isValid()) {
    qCritical("Unable to create the Qt test configuration directory.");
    return 2;
  }
  QString configError;
  if (!SettingsStore::initialize(configDirectory.path(), true, &configError)) {
    qCritical().noquote() << configError;
    return 2;
  }

  if (qEnvironmentVariableIsSet("SCANTAILOR_BATCH_BENCH_PROJECT")) {
    TestMainWindowAsync benchmark;
    return QTest::qExec(&benchmark, argc, argv);
  }

  int status = 0;

  TestCoreQt tc1;
  status |= QTest::qExec(&tc1, argc, argv);

  TestUxQt tc2;
  status |= QTest::qExec(&tc2, argc, argv);

  TestMainWindowAsync tc3;
  status |= QTest::qExec(&tc3, argc, argv);

  return status;
}
