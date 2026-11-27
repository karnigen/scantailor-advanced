// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

class QString;
class QDomDocument;
class QDomElement;

namespace output {
class OutputProcessingParams {
 public:
  OutputProcessingParams();

  explicit OutputProcessingParams(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool operator==(const OutputProcessingParams& other) const;

  bool operator!=(const OutputProcessingParams& other) const;

  bool isAutoZonesFound() const;

  void setAutoZonesFound(bool autoZonesFound);

  bool isBlackOnWhiteSetManually() const;

  void setBlackOnWhiteSetManually(bool blackOnWhiteSetManually);

 private:
  bool m_autoZonesFound;
  bool m_blackOnWhiteSetManually;
};


inline bool output::OutputProcessingParams::isAutoZonesFound() const {
  return m_autoZonesFound;
}

inline void output::OutputProcessingParams::setAutoZonesFound(bool autoZonesFound) {
  OutputProcessingParams::m_autoZonesFound = autoZonesFound;
}

inline bool OutputProcessingParams::isBlackOnWhiteSetManually() const {
  return m_blackOnWhiteSetManually;
}

inline void OutputProcessingParams::setBlackOnWhiteSetManually(bool blackOnWhiteSetManually) {
  OutputProcessingParams::m_blackOnWhiteSetManually = blackOnWhiteSetManually;
}
}  // namespace output
