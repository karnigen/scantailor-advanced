// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <BinaryThreshold.h>
#include <GrayImage.h>

#include <QImage>

#include "ImageSettings.h"
#include "ImageTransformation.h"

class FilterData {
  // Member-wise copying is OK.
 public:
  explicit FilterData(const QImage& image);

  FilterData(const FilterData& other, const ImageTransformation& xform);

  FilterData(const FilterData& other);

  imageproc::BinaryThreshold bwThreshold() const;

  const ImageTransformation& xform() const;

  const QImage& origImage() const;

  const imageproc::GrayImage& grayImage() const;

  bool isBlackOnWhite() const;

  imageproc::BinaryThreshold bwThresholdBlackOnWhite() const;

  imageproc::GrayImage grayImageBlackOnWhite() const;

  void updateImageParams(const ImageSettings::PageParams& imageParams);

 private:
  QImage m_origImage;
  imageproc::GrayImage m_grayImage;
  ImageTransformation m_xform;
  ImageSettings::PageParams m_imageParams;
};


inline const ImageTransformation& FilterData::xform() const {
  return m_xform;
}

inline const QImage& FilterData::origImage() const {
  return m_origImage;
}

inline const imageproc::GrayImage& FilterData::grayImage() const {
  return m_grayImage;
}

inline void FilterData::updateImageParams(const ImageSettings::PageParams& imageParams) {
  m_imageParams = imageParams;
}
