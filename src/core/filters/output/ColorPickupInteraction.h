// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QCoreApplication>
#include <QRect>
#include <cstdint>
#include <memory>

#include "EditableZoneSet.h"
#include "FillColorProperty.h"
#include "InteractionHandler.h"
#include "InteractionState.h"

class ZoneInteractionContext;
class QShortcut;

namespace output {
class ColorPickupInteraction : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(ColorPickupInteraction)
 public:
  ColorPickupInteraction(EditableZoneSet& zones, ZoneInteractionContext& context);

  ~ColorPickupInteraction() override;

  void startInteraction(const EditableZoneSet::Zone& zone, InteractionState& interaction);

  bool isActive(const InteractionState& interaction) const;

 protected:
  void onPaint(QPainter& painter, const InteractionState& interaction) override;

  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

 private:
  void takeColor();

  QRect targetBoundingRect() const;

  void switchToDefaultInteraction();

  static uint32_t bitMixColor(uint32_t color);

  static uint32_t bitUnmixColor(uint32_t mixed);

  EditableZoneSet& m_zones;
  ZoneInteractionContext& m_context;
  InteractionState::Captor m_interaction;
  std::shared_ptr<FillColorProperty> m_fillColorProp;
  int m_dontDrawCircle;
  std::unique_ptr<QShortcut> m_cancelShortcut;

  static const uint32_t m_sBitMixingLUT[3][256];
  static const uint32_t m_sBitUnmixingLUT[3][256];
};
}  // namespace output
