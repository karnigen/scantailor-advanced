// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QImage>
#include <QPointF>
#include <QTransform>
#include <vector>

class QLineF;
class QImage;
class ImageTransformation;
class DebugImages;

namespace imageproc {
class GrayImage;
}

namespace page_split {
class VertLineFinder {
 public:
  static std::vector<QLineF> findLines(const QImage& image,
                                       const ImageTransformation& xform,
                                       int maxLines,
                                       DebugImages* dbg = nullptr,
                                       imageproc::GrayImage* grayDownscaled = nullptr,
                                       QTransform* outToDownscaled = nullptr);

 private:
  class QualityLine {
   public:
    QualityLine(const QPointF& top, const QPointF& bottom, unsigned quality);

    const QPointF& left() const;

    const QPointF& right() const;

    unsigned quality() const;

    QLineF toQLine() const;

   private:
    QPointF m_left;
    QPointF m_right;
    unsigned m_quality;
  };


  class LineGroup {
   public:
    explicit LineGroup(const QualityLine& line);

    bool belongsHere(const QualityLine& line) const;

    void add(const QualityLine& line);

    void merge(const LineGroup& other);

    const QualityLine& leader() const;

   private:
    QualityLine m_leader;
    double m_left;
    double m_right;
  };


  static imageproc::GrayImage removeDarkVertBorders(const imageproc::GrayImage& src);

  static void selectVertBorders(imageproc::GrayImage& image);

  static void buildWeightTable(unsigned weight_table[]);
};
}  // namespace page_split
