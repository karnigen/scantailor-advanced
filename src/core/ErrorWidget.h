// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_ERRORWIDGET_H_
#define SCANTAILOR_CORE_ERRORWIDGET_H_

#include <QWidget>
#include <Qt>
#include <memory>

class QString;
namespace Ui {
class ErrorWidget;
}

class ErrorWidget : public QWidget {
  Q_OBJECT
 public:
  explicit ErrorWidget(const QString& text, Qt::TextFormat fmt = Qt::AutoText);

  virtual ~ErrorWidget();

 private slots:

  /**
   * \see QLabel::linkActivated()
   */
  virtual void linkActivated(const QString& link) {}

 private:
  std::unique_ptr<Ui::ErrorWidget> ui;
};


#endif
