// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

class QString;
class QDomElement;
class QSize;
class QSizeF;
class QPointF;
class QLineF;
class QRect;
class QRectF;
class QPolygonF;

class XmlUnmarshaller {
 public:
  static QString string(const QDomElement& el);

  static QSize size(const QDomElement& el);

  static QSizeF sizeF(const QDomElement& el);

  static QPointF pointF(const QDomElement& el);

  static QLineF lineF(const QDomElement& el);

  static QRect rect(const QDomElement& el);

  static QRectF rectF(const QDomElement& el);

  static QPolygonF polygonF(const QDomElement& el);
};
