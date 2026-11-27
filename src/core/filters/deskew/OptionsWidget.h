// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <core/ConnectionManager.h>

#include <list>
#include <memory>
#include <set>

#include <QTimer>

#include "AutoManualMode.h"
#include "Dependencies.h"
#include "FilterOptionsWidget.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "ui_OptionsWidget.h"

namespace deskew {
class Settings;

class OptionsWidget : public FilterOptionsWidget, private Ui::OptionsWidget {
  Q_OBJECT
 public:
  class UiData {
    // Member-wise copying is OK.
   public:
    UiData();

    ~UiData();

    void setEffectiveDeskewAngle(double degrees);

    double effectiveDeskewAngle() const;

    void setEffectiveObliqueAngle(double degrees);

    double effectiveObliqueAngle() const;

    void setDependencies(const Dependencies& deps);

    const Dependencies& dependencies() const;

    void setMode(AutoManualMode mode);

    AutoManualMode mode() const;

    void setObliqueMode(AutoManualMode mode);

    AutoManualMode obliqueMode() const;

   private:
    double m_effDeskewAngle;
    double m_effObliqueAngle;
    Dependencies m_deps;
    AutoManualMode m_mode;
    AutoManualMode m_obliqueMode;
  };


  OptionsWidget(std::shared_ptr<Settings> settings, const PageSelectionAccessor& pageSelectionAccessor);

  ~OptionsWidget() override;

 signals:

  void manualDeskewAngleSet(double degrees);

  void manualObliqueAngleSet(double degrees);

 public slots:

  void manualDeskewAngleSetExternally(double degrees);

  void manualObliqueAngleSetExternally(double degrees);

 public:
  void preUpdateUI(const PageId& pageId);

  void postUpdateUI(const UiData& uiData);

 private slots:

  void spinBoxValueChanged(double skewDegrees);

  void obliqueSpinBoxValueChanged(double obliqueDegrees);

  void modeChanged(bool autoMode);

  void topEdgeToggled(bool checked);

  void showDeskewDialog();

  void reloadAfterRotation();

  void appliedTo(const std::set<PageId>& pages, bool applyDeskew, bool applyOblique);

  void appliedToAllPages(const std::set<PageId>& pages, bool applyDeskew, bool applyOblique);

 private:
  void obliqueModeChanged(AutoManualMode mode);

  void updateModeIndication(AutoManualMode mode);

  void updateObliqueModeIndication(AutoManualMode mode);

  void setSpinBoxUnknownState();

  void setSpinBoxKnownState(double angle);

  void commitCurrentParams();

  void scheduleRotationReload();

  void setupUiConnections();

  static double spinBoxToDegrees(double sbValue);

  static double degreesToSpinBox(double degrees);

  static const double MAX_ANGLE;

  std::shared_ptr<Settings> m_settings;
  PageId m_pageId;
  UiData m_uiData;

  PageSelectionAccessor m_pageSelectionAccessor;

  ConnectionManager m_connectionManager;
  QTimer m_rotationReloadTimer;
};


inline void OptionsWidget::UiData::setEffectiveDeskewAngle(const double degrees) {
  m_effDeskewAngle = degrees;
}

inline double OptionsWidget::UiData::effectiveDeskewAngle() const {
  return m_effDeskewAngle;
}

inline void OptionsWidget::UiData::setEffectiveObliqueAngle(const double degrees) {
  m_effObliqueAngle = degrees;
}

inline double OptionsWidget::UiData::effectiveObliqueAngle() const {
  return m_effObliqueAngle;
}

inline void OptionsWidget::UiData::setDependencies(const Dependencies& deps) {
  m_deps = deps;
}

inline const Dependencies& OptionsWidget::UiData::dependencies() const {
  return m_deps;
}

inline void OptionsWidget::UiData::setMode(const AutoManualMode mode) {
  m_mode = mode;
}

inline AutoManualMode OptionsWidget::UiData::mode() const {
  return m_mode;
}

inline void OptionsWidget::UiData::setObliqueMode(const AutoManualMode mode) {
  m_obliqueMode = mode;
}

inline AutoManualMode OptionsWidget::UiData::obliqueMode() const {
  return m_obliqueMode;
}
}  // namespace deskew
