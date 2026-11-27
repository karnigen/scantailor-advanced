// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_OUTPUT_UTILS_H_
#define SCANTAILOR_OUTPUT_UTILS_H_

#include <filesystem>

class Dpi;
class QTransform;
class QRect;

namespace output {
class Params;

class Utils {
 public:
  Utils() = default;

  static std::filesystem::path automaskDir(const std::filesystem::path& outDir);

  static std::filesystem::path predespeckleDir(const std::filesystem::path& outDir);

  static std::filesystem::path specklesDir(const std::filesystem::path& outDir);

  static std::filesystem::path foregroundDir(const std::filesystem::path& outDir);

  static std::filesystem::path backgroundDir(const std::filesystem::path& outDir);

  static std::filesystem::path originalBackgroundDir(const std::filesystem::path& outDir);

  static QTransform scaleFromToDpi(const Dpi& from, const Dpi& to);

  static QTransform rotate(double degrees, const QRect& imageRect);

  static Params buildDefaultParams();
};
}  // namespace output
#endif
