// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "NonCopyable.h"

template <typename T>
class SafeDeletingQObjectPtr {
  DECLARE_NON_COPYABLE(SafeDeletingQObjectPtr)

 public:
  explicit SafeDeletingQObjectPtr(T* obj = 0) : m_obj(obj) {}

  ~SafeDeletingQObjectPtr() {
    if (m_obj) {
      m_obj->deleteLater();
    }
  }

  void reset(T* other) { SafeDeletingQObjectPtr(other).swap(*this); }

  T& operator*() const { return *m_obj; }

  T* operator->() const { return m_obj; }

  T* get() const { return m_obj; }

  void swap(SafeDeletingQObjectPtr& other) {
    T* tmp = m_obj;
    m_obj = other.m_obj;
    other.m_obj = tmp;
  }

 private:
  T* m_obj;
};


template <typename T>
void swap(SafeDeletingQObjectPtr<T>& o1, SafeDeletingQObjectPtr<T>& o2) {
  o1.swap(o2);
}
