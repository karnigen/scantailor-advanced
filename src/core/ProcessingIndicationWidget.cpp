// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "ProcessingIndicationWidget.h"

#include <ColorInterpolation.h>

#include <QPaintEvent>
#include <QPainter>
#include <QTimerEvent>

#include "ColorSchemeManager.h"

using namespace imageproc;

static const double distinctionIncrease = 1.0 / 5.0;
static const double distinctionDecrease = -1.0 / 3.0;

ProcessingIndicationWidget::ProcessingIndicationWidget(QWidget* parent)
    : QWidget(parent),
      m_animation(10),
      m_backgroundFadeEnabled(true),
      m_distinction(1.0),
      m_distinctionDelta(distinctionIncrease),
      m_timerId(0) {
  setAttribute(Qt::WA_TranslucentBackground);
  m_headColor = ColorSchemeManager::instance().getColorParam("ProcessingIndicationHead",
                                                             palette().color(QPalette::Window).darker(200));
  m_tailColor = ColorSchemeManager::instance().getColorParam("ProcessingIndicationTail",
                                                             palette().color(QPalette::Window).darker(130));
}

void ProcessingIndicationWidget::setBackgroundFadeEnabled(const bool enabled) {
  if (m_backgroundFadeEnabled != enabled) {
    m_backgroundFadeEnabled = enabled;
    update();
  }
}

void ProcessingIndicationWidget::resetAnimation() {
  m_distinction = 1.0;
  m_distinctionDelta = distinctionIncrease;
}

void ProcessingIndicationWidget::processingRestartedEffect() {
  m_distinction = 1.0;
  m_distinctionDelta = distinctionDecrease;
}

void ProcessingIndicationWidget::paintEvent(QPaintEvent* event) {
  QRect animationRect(this->animationRect());
  if (!event->rect().contains(animationRect)) {
    update(animationRect);
    return;
  }

  QColor headColor(colorInterpolation(m_tailColor, m_headColor, m_distinction));

  m_distinction += m_distinctionDelta;
  if (m_distinction > 1.0) {
    m_distinction = 1.0;
  } else if (m_distinction <= 0.0) {
    m_distinction = 0.0;
    m_distinctionDelta = distinctionIncrease;
  }

  QPainter painter(this);

  if (m_backgroundFadeEnabled) {
    QColor fadeColor
        = ColorSchemeManager::instance().getColorParam("ProcessingIndicationFade", palette().window().color());
    fadeColor.setAlpha(127);
    painter.fillRect(rect(), fadeColor);
  }

  m_animation.nextFrame(headColor, m_tailColor, &painter, animationRect);

  if (m_timerId == 0) {
    m_timerId = startTimer(180);
  }
}

void ProcessingIndicationWidget::timerEvent(QTimerEvent* event) {
  killTimer(event->timerId());
  m_timerId = 0;
  update(animationRect());
}

QRect ProcessingIndicationWidget::animationRect() const {
  QRect r(0, 0, 80, 80);
  r.moveCenter(rect().center());
  r &= rect();
  return r;
}
