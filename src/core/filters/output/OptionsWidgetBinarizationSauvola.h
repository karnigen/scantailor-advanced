// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <core/ConnectionManager.h>

#include <QtCore>
#include <list>
#include <memory>

#include "ColorParams.h"
#include "OptionsWidgetBinarization.h"
#include "Settings.h"
#include "ui_OptionsWidgetBinarizationSauvola.h"

namespace output {
class OptionsWidgetBinarizationSauvola : public OptionsWidgetBinarization,
                                         private Ui::OptionsWidgetBinarizationSauvola {
  Q_OBJECT
 public:
  explicit OptionsWidgetBinarizationSauvola(std::shared_ptr<Settings> settings);

  ~OptionsWidgetBinarizationSauvola() override = default;

  void updateUi(const PageId& m_pageId) override;

 private slots:

  void sauvolaDeltaChanged(double value);

  void windowSizeChanged(int value);

  void sauvolaCoefChanged(double value);

  void sendStateChanged();

 private:
  void updateView();

  void setupUiConnections();

  std::shared_ptr<Settings> m_settings;
  PageId m_pageId;
  ColorParams m_colorParams;
  QTimer m_delayedStateChanger;
  OutputProcessingParams m_outputProcessingParams;

  ConnectionManager m_connectionManager;
};
}  // namespace output
