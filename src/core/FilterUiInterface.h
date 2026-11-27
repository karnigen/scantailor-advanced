// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <memory>

#include "AbstractCommand.h"
#include "PageId.h"

class DebugImages;
class FilterOptionsWidget;
class QWidget;

/**
 * \brief A reduced interface to MainWindow to allow filters to manupulate the UI.
 */
class FilterUiInterface {
 public:
  enum Ownership { KEEP_OWNERSHIP, TRANSFER_OWNERSHIP };

  virtual ~FilterUiInterface() = default;

  virtual void setOptionsWidget(FilterOptionsWidget* widget, Ownership ownership) = 0;

  virtual void setImageWidget(QWidget* widget,
                              Ownership ownership,
                              DebugImages* debugImages = nullptr,
                              bool overlay = false) = 0;

  virtual void invalidateThumbnail(const PageId& pageId) = 0;

  virtual void invalidateAllThumbnails() = 0;

  /**
   * Returns a callable object that when called will open a relinking dialog.
   */
  virtual std::shared_ptr<AbstractCommand<void>> relinkingDialogRequester() = 0;
};
