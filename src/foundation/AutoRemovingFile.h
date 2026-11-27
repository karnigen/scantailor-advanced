// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <filesystem>

/**
 * \brief Provides unique_ptr semantics for files.
 *
 * Just like std::unique_ptr deleting its object when it goes out of scope,
 * this class deletes a file.  unique_ptr's copying semantics is also preserved.
 */
class AutoRemovingFile {
 private:
  struct CopyHelper {
    AutoRemovingFile* obj;

    explicit CopyHelper(AutoRemovingFile* o) : obj(o) {}
  };

 public:
  AutoRemovingFile();

  explicit AutoRemovingFile(std::filesystem::path filePath);

  AutoRemovingFile(AutoRemovingFile& other);

  AutoRemovingFile(CopyHelper other);

  ~AutoRemovingFile();

  AutoRemovingFile& operator=(AutoRemovingFile& other);

  AutoRemovingFile& operator=(CopyHelper other);

  operator CopyHelper() { return CopyHelper(this); }

  const std::filesystem::path& get() const { return m_file; }

  void reset(const std::filesystem::path& file);

  std::filesystem::path release();

 private:
  std::filesystem::path m_file;
};
