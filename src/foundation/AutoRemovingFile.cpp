// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "AutoRemovingFile.h"

#include <system_error>
#include <utility>

AutoRemovingFile::AutoRemovingFile() = default;

AutoRemovingFile::AutoRemovingFile(std::filesystem::path filePath) : m_file(std::move(filePath)) {}

AutoRemovingFile::AutoRemovingFile(AutoRemovingFile& other) : m_file(other.release()) {}

AutoRemovingFile::AutoRemovingFile(CopyHelper other) : m_file(other.obj->release()) {}

AutoRemovingFile::~AutoRemovingFile() {
  if (!m_file.empty()) {
    std::error_code error;
    std::filesystem::remove(m_file, error);
  }
}

AutoRemovingFile& AutoRemovingFile::operator=(AutoRemovingFile& other) {
  m_file = other.release();
  return *this;
}

AutoRemovingFile& AutoRemovingFile::operator=(CopyHelper other) {
  m_file = other.obj->release();
  return *this;
}

void AutoRemovingFile::reset(const std::filesystem::path& file) {
  const std::filesystem::path& oldFile(file);

  m_file = file;

  if (!oldFile.empty()) {
    std::error_code error;
    std::filesystem::remove(oldFile, error);
  }
}

std::filesystem::path AutoRemovingFile::release() {
  std::filesystem::path saved(m_file);
  m_file.clear();
  return saved;
}
