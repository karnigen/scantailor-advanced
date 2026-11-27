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
#include "ui_OptionsWidgetBinarizationOtsu.h"

namespace output {
class OptionsWidgetBinarizationOtsu : public OptionsWidgetBinarization, private Ui::OptionsWidgetBinarizationOtsu {
  Q_OBJECT
 public:
  explicit OptionsWidgetBinarizationOtsu(std::shared_ptr<Settings> settings);

  ~OptionsWidgetBinarizationOtsu() override = default;

  void updateUi(const PageId& m_pageId) override;

 private slots:

  void thresholdSliderReleased();

  void thresholdSliderValueChanged(int value);

  void setLighterThreshold();

  void setDarkerThreshold();

  void setNeutralThreshold();

  void sendStateChanged();

 private:
  void updateView();

  void setThresholdAdjustment(int value);

  void setupUiConnections();

  std::shared_ptr<Settings> m_settings;
  PageId m_pageId;
  ColorParams m_colorParams;
  QTimer m_delayedStateChanger;

  ConnectionManager m_connectionManager;
};
}  // namespace output
