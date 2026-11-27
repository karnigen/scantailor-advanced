// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "DraggableObject.h"

namespace page_split {
class SplitLineObject : public DraggableObject {
 protected:
  virtual Proximity lineProximity(const QPointF& widgetMousePos, const InteractionState& interaction) const = 0;

  virtual QPointF linePosition(const InteractionState& interaction) const = 0;

  virtual void lineMoveRequest(const QPointF& widgetPos) = 0;

  virtual Proximity proximity(const QPointF& widgetMousePos, const InteractionState& interaction) {
    return lineProximity(widgetMousePos, interaction);
  }

  virtual QPointF position(const InteractionState& interaction) const { return linePosition(interaction); }

  virtual void moveRequest(const QPointF& widgetPos) { return lineMoveRequest(widgetPos); }
};
}  // namespace page_split
