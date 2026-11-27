// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_CORE_FILENAMEDISAMBIGUATOR_H_
#define SCANTAILOR_CORE_FILENAMEDISAMBIGUATOR_H_

#include <filesystem>
#include <functional>
#include <memory>
#include <set>

#include "NonCopyable.h"

class AbstractRelinker;
class QString;
class QDomElement;
class QDomDocument;

/**
 * \brief Associates an integer label with each file path so that
 *        files with the same name but from different directories
 *        have distinctive labels.
 *
 * \note This class is thread-safe.
 */
class FileNameDisambiguator {
  DECLARE_NON_COPYABLE(FileNameDisambiguator)

 public:
  FileNameDisambiguator();

  /**
   * \brief Load disambiguation information from XML.
   */
  explicit FileNameDisambiguator(const QDomElement& disambiguatorEl);

  /**
   * \brief Load disambiguation information from XML with file path unpacking.
   *
   * Supplying a file path unpacker allows storing shorthands rather than
   * full paths in XML.  Unpacker is a functor taking a shorthand and
   * returning the full path.  If unpacker returns an empty path,
   * the record will be skipped.
   */
  FileNameDisambiguator(const QDomElement& disambiguatorEl,
                        const std::function<std::filesystem::path(const QString&)>& filePathUnpacker);

  virtual ~FileNameDisambiguator();

  /**
   * \brief Serialize disambiguation information to XML.
   */
  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  /**
   * \brief Serialize disambiguation information to XML with file path packing.
   *
   * Supplying a file path packer allows storing shorthands rather than
   * full paths in XML.  Packer is a functor taking a full file path and
   * returning the corresponding shorthand.  If packer returns an empty string,
   * the record will be skipped.
   */
  QDomElement toXml(QDomDocument& doc,
                    const QString& name,
                    const std::function<QString(const std::filesystem::path&)>& filePathPacker) const;

  int getLabel(const std::filesystem::path& filePath) const;

  int registerFile(const std::filesystem::path& filePath);

  void performRelinking(const AbstractRelinker& relinker);

 private:
  class Impl;

  std::unique_ptr<Impl> m_impl;
};


#endif  // ifndef SCANTAILOR_CORE_FILENAMEDISAMBIGUATOR_H_
