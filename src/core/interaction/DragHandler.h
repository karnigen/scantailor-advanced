// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QCoreApplication>
#include <QPoint>
#include <functional>

#include "InteractionHandler.h"
#include "InteractionState.h"

class ImageViewBase;

class DragHandler : public InteractionHandler {
  Q_DECLARE_TR_FUNCTIONS(DragHandler)
 public:
  explicit DragHandler(ImageViewBase& imageView);

  DragHandler(ImageViewBase& imageView,
              const std::function<bool(const InteractionState&)>& explicitInteractionPermitter);

  bool isActive() const;

 protected:
  void onMousePressEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseReleaseEvent(QMouseEvent* event, InteractionState& interaction) override;

  void onMouseMoveEvent(QMouseEvent* event, InteractionState& interaction) override;

 private:
  void init();

  ImageViewBase& m_imageView;
  InteractionState::Captor m_interaction;
  QPoint m_lastMousePos;
  std::function<bool(const InteractionState&)> m_interactionPermitter;
};
