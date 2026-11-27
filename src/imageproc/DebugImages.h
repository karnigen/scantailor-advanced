// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>
#include <deque>
#include <functional>

#include "AutoRemovingFile.h"

class QImage;
class QWidget;

namespace imageproc {
class BinaryImage;
}

/**
 * \brief A sequence of image + label pairs.
 */
class DebugImages {
 public:
  virtual ~DebugImages() = default;

  virtual void add(const QImage& image,
                   const QString& label,
                   const std::function<QWidget*(const QImage&)>& imageViewFactory
                   = std::function<QWidget*(const QImage&)>()) = 0;

  virtual void add(const imageproc::BinaryImage& image,
                   const QString& label,
                   const std::function<QWidget*(const QImage&)>& imageViewFactory
                   = std::function<QWidget*(const QImage&)>()) = 0;

  virtual bool empty() const = 0;

  /**
   * \brief Removes and returns the first item in the sequence.
   *
   * The label and viewer widget factory (that may not be bound)
   * are returned by taking pointers to them as arguments.
   * Returns a null AutoRemovingFile if image sequence is empty.
   */
  virtual AutoRemovingFile retrieveNext(QString* label = nullptr,
                                        std::function<QWidget*(const QImage&)>* imageViewFactory = nullptr) = 0;
};
