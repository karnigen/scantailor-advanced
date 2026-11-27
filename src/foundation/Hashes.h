// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_FOUNDATION_HASHES_H_
#define SCANTAILOR_FOUNDATION_HASHES_H_

#include <QtCore/QString>

namespace hashes {
template <typename>
struct hash;

template <>
struct hash<QString> {
  std::size_t operator()(const QString& str) const noexcept {
    const QChar* data = str.constData();
    std::size_t hash = 5381;
    for (int i = 0; i < str.size(); ++i) {
      hash = ((hash << 5) + hash) ^ ((data[i].row() << 8) | data[i].cell());
    }
    return hash;
  }
};
}  // namespace hashes

#endif  // SCANTAILOR_FOUNDATION_HASHES_H_
