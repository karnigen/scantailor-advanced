// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only


#pragma once

#include <BinaryImage.h>

#include <QImage>
#include <QStackedWidget>
#include <QString>
#include <memory>

#include "DespeckleLevel.h"
#include "DespeckleState.h"
#include "Dpi.h"

class DebugImages;
class ProcessingIndicationWidget;
class ImageViewBase;

namespace output {
class DespeckleVisualization;

class DespeckleView : public QStackedWidget {
  Q_OBJECT
 public:
  /**
   * \param despeckleState Describes a particular despeckling.
   * \param visualization Optional despeckle visualization.
   *        If null, it will be reconstructed from \p despeckleState
   *        when this widget becomes visible.
   * \param debug Indicates whether debugging is turned on.
   */
  DespeckleView(const DespeckleState& despeckleState, const DespeckleVisualization& visualization, bool debug);

  ~DespeckleView() override;

 public slots:

  void despeckleLevelChanged(double level, bool* handled);

 signals:

  void imageViewCreated(ImageViewBase*);

 protected:
  void hideEvent(QHideEvent* evt) override;

  void showEvent(QShowEvent* evt) override;

 private:
  class TaskCancelException;
  class TaskCancelHandle;
  class DespeckleTask;
  class DespeckleResult;

  enum AnimationAction { RESET_ANIMATION, RESUME_ANIMATION };

  void initiateDespeckling(AnimationAction animAction);

  void despeckleDone(const DespeckleState& despeckleState,
                     const DespeckleVisualization& visualization,
                     DebugImages* dbg);

  void despeckleFailed(const std::shared_ptr<TaskCancelHandle>& cancelHandle, const QString& message);

  void cancelBackgroundTask();

  void removeImageViewWidget();

  DespeckleState m_despeckleState;
  std::shared_ptr<TaskCancelHandle> m_cancelHandle;
  ProcessingIndicationWidget* m_processingIndicator;
  double m_despeckleLevel;
  bool m_debug;
};
}  // namespace output
