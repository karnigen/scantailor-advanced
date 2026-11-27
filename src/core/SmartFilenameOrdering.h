// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_SMARTFILENAMEORDERING_H_
#define SCANTAILOR_CORE_SMARTFILENAMEORDERING_H_

class QFileInfo;
class QString;

class SmartFilenameOrdering {
 public:
  SmartFilenameOrdering() = default;

  /**
   * \brief Compare filenames using a set of heuristic rules.
   *
   * This function tries to mimic the way humans would order filenames.
   * For example, "2.png" will go before "12.png".  While doing so,
   * it still provides the usual guarantees of a comparison predicate,
   * such as two different file paths will never be ruled equivalent.
   *
   * \return true if \p lhs should go before \p rhs.
   */
  bool operator()(const QFileInfo& lhs, const QFileInfo& rhs) const;
  bool operator()(QString const& lhs, QString const& rhs) const;
};


#endif
