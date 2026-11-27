// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_OUTPUT_OUTPUTIMAGEPLAIN_H_
#define SCANTAILOR_OUTPUT_OUTPUTIMAGEPLAIN_H_

#include "OutputImage.h"

namespace output {
class OutputImagePlain : public virtual OutputImage {
 public:
  OutputImagePlain() = default;

  explicit OutputImagePlain(const QImage& image);

  QImage toImage() const override;

  bool isNull() const override;

  void setDpm(const Dpm& dpm) override;

 private:
  QImage m_image;
};
}  // namespace output


#endif  // SCANTAILOR_OUTPUT_OUTPUTIMAGEPLAIN_H_
