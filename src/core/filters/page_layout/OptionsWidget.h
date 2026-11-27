// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <UnitsListener.h>
#include <core/ConnectionManager.h>

#include <QIcon>
#include <QPalette>
#include <QSize>
#include <QWidget>
#include <memory>
#include <set>
#include <unordered_map>

#include "Alignment.h"
#include "FilterOptionsWidget.h"
#include "ImageMetadata.h"
#include "Margins.h"
#include "PageId.h"
#include "PageSelectionAccessor.h"
#include "ui_OptionsWidget.h"

class QToolButton;
class ProjectPages;
class QLineEdit;

namespace page_layout {
class Settings;

class OptionsWidget : public FilterOptionsWidget, public UnitsListener, private Ui::OptionsWidget {
  Q_OBJECT
 public:
  OptionsWidget(std::shared_ptr<Settings> settings,
                std::shared_ptr<ProjectPages> pages,
                const PageSelectionAccessor& pageSelectionAccessor);

  ~OptionsWidget() override;

  void preUpdateUI(const PageInfo& pageInfo, const Margins& marginsMm, const Alignment& alignment);

  void postUpdateUI();

  bool leftRightLinked() const;

  bool topBottomLinked() const;

  const Margins& marginsMM() const;

  const Alignment& alignment() const;

  void onUnitsChanged(Units units) override;

 signals:

  void leftRightLinkToggled(bool linked);

  void topBottomLinkToggled(bool linked);

  void alignmentChanged(const Alignment& alignment);

  void marginsSetLocally(const Margins& marginsMm);

  void aggregateHardSizeChanged();

 public slots:

  void marginsSetExternally(const Margins& marginsMm);

 private slots:

  void horMarginsChanged(double val);

  void vertMarginsChanged(double val);

  void autoMarginsToggled(bool checked);

  void horizontalAlignmentModeChanged(int idx);

  void verticalAlignmentModeChanged(int idx);

  void topBottomLinkClicked();

  void leftRightLinkClicked();

  void zeroMarginsClicked();

  void alignWithOthersToggled();

  void alignmentButtonClicked();

  void showApplySettingsDialog();

  void showApplyAlignmentDialog();

  void freezeAggregateHardSizeToggled(bool checked);

  void sourceDpiComboActivated(int index);

  void sourceDpiEditingFinished();

  void applySettings(const std::set<PageId>& pages,
                     bool applyLeft = true,
                     bool applyRight = true,
                     bool applyTop = true,
                     bool applyBottom = true,
                     bool applyDpiX = true,
                     bool applyDpiY = true);

  void applyAlignment(const std::set<PageId>& pages, bool copyCommonSize, bool copyAlignment);

 private:
  using AlignmentByButton = std::unordered_map<QToolButton*, Alignment>;

  void updateMarginsDisplay();

  void updateLinkDisplay(QToolButton* button, bool linked);

  void updateAlignmentButtonsEnabled();

  void updateMarginsControlsEnabled();

  void updateAutoModeButtons();

  void updateAlignmentModeEnabled();

  QToolButton* getCheckedAlignmentButton() const;

  void setupUiConnections();

  void setupIcons();

  void setupSourceDpiControls();

  void updateSourceDpiDisplay();

  void commitSourceDpiIfValid();

  void decorateSourceDpiField(QLineEdit* field, ImageMetadata::DpiStatus dpiStatus);

  void updateSourceDpiComboFromFields();

  void keepSourceDpiFieldsEnabled();

  bool isSourceDpiFieldFocused() const;

  std::shared_ptr<Settings> m_settings;
  std::shared_ptr<ProjectPages> m_pages;
  PageSelectionAccessor m_pageSelectionAccessor;
  QIcon m_chainIcon;
  QIcon m_brokenChainIcon;
  AlignmentByButton m_alignmentByButton;
  PageId m_pageId;
  Dpi m_dpi;
  QSize m_sourceImagePixelSize;
  QPalette m_sourceDpiNormalPalette;
  QPalette m_sourceDpiErrorPalette;
  QWidget* m_sourceDpiFocusWidget;
  Margins m_marginsMM;
  Alignment m_alignment;
  bool m_leftRightLinked;
  bool m_topBottomLinked;
  QButtonGroup* m_alignmentButtonGroup;
  QToolButton* m_zeroMarginsBtn;

  ConnectionManager m_connectionManager;
};
}  // namespace page_layout
