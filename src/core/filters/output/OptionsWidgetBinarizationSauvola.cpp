// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#include "OptionsWidgetBinarizationSauvola.h"

#include <utility>


namespace output {

OptionsWidgetBinarizationSauvola::OptionsWidgetBinarizationSauvola(std::shared_ptr<Settings> settings)
    : m_settings(std::move(settings)),
      m_connectionManager(std::bind(&OptionsWidgetBinarizationSauvola::setupUiConnections, this)) {
  setupUi(this);

  m_delayedStateChanger.setSingleShot(true);

  setupUiConnections();
}

void OptionsWidgetBinarizationSauvola::updateUi(const PageId& pageId) {
  auto block = m_connectionManager.getScopedBlock();

  const Params params(m_settings->getParams(pageId));
  m_pageId = pageId;
  m_colorParams = params.colorParams();
  m_outputProcessingParams = m_settings->getOutputProcessingParams(pageId);

  updateView();
}

void OptionsWidgetBinarizationSauvola::sauvolaDeltaChanged(double value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setThresholdAdjustment(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedStateChanger.start(750);
}

void OptionsWidgetBinarizationSauvola::windowSizeChanged(int value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setWindowSize(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedStateChanger.start(750);
}

void OptionsWidgetBinarizationSauvola::sauvolaCoefChanged(double value) {
  BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
  opt.setSauvolaCoef(value);
  m_colorParams.setBlackWhiteOptions(opt);
  m_settings->setColorParams(m_pageId, m_colorParams);

  m_delayedStateChanger.start(750);
}

void OptionsWidgetBinarizationSauvola::updateView() {
  BlackWhiteOptions blackWhiteOptions = m_colorParams.blackWhiteOptions();
  sauvolaDelta->setValue(blackWhiteOptions.thresholdAdjustment());
  windowSize->setValue(blackWhiteOptions.getWindowSize());
  sauvolaCoef->setValue(blackWhiteOptions.getSauvolaCoef());
}

void OptionsWidgetBinarizationSauvola::sendStateChanged() {
  emit stateChanged();
}

#define CONNECT(...) m_connectionManager.addConnection(connect(__VA_ARGS__))

void OptionsWidgetBinarizationSauvola::setupUiConnections() {
  CONNECT(sauvolaDelta, SIGNAL(valueChanged(double)), this, SLOT(sauvolaDeltaChanged(double)));
  CONNECT(windowSize, SIGNAL(valueChanged(int)), this, SLOT(windowSizeChanged(int)));
  CONNECT(sauvolaCoef, SIGNAL(valueChanged(double)), this, SLOT(sauvolaCoefChanged(double)));
  CONNECT(&m_delayedStateChanger, SIGNAL(timeout()), this, SLOT(sendStateChanged()));
}

#undef CONNECT
}  // namespace output
