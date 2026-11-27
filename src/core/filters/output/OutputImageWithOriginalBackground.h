// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_OUTPUT_OUTPUTIMAGEWITHORIGINALBACKGROUND_H_
#define SCANTAILOR_OUTPUT_OUTPUTIMAGEWITHORIGINALBACKGROUND_H_

#include "OutputImageWithForeground.h"

namespace output {
class OutputImageWithOriginalBackground : public virtual OutputImageWithForeground {
 public:
  virtual QImage getOriginalBackgroundImage() const = 0;
};
}  // namespace output


#endif  // SCANTAILOR_OUTPUT_OUTPUTIMAGEWITHORIGINALBACKGROUND_H_
