// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <cstddef>
#include <stdexcept>

#include "NonCopyable.h"

template <typename T>
class StaticPoolBase {
  DECLARE_NON_COPYABLE(StaticPoolBase)

 public:
  StaticPoolBase(T* buf, size_t size) : m_next(buf), m_sizeRemaining(size) {}

  /**
   * \brief Allocates a sequence of objects.
   *
   * If the pool has enough free space, returns a sequence of requested
   * number of elements, otherwise throws an std::runtime_error.
   * If T is a POD type, the returned objects are uninitialized,
   * otherwise they are default-constructed.
   *
   * This function was moved to the base class in order to have
   * just one instantiation of it for different sized pool of the same type.
   */
  T* alloc(size_t numElements);

 private:
  T* m_next;
  size_t m_sizeRemaining;
};


/**
 * \brief Allocates objects from a statically sized pool.
 *
 * There is no way of releasing the allocated objects
 * besides destroying the whole pool.
 */
template <typename T, size_t S>
class StaticPool : public StaticPoolBase<T> {
  DECLARE_NON_COPYABLE(StaticPool)

 public:
  StaticPool() : StaticPoolBase<T>(m_buf, S) {}

 private:
  T m_buf[S];
};


template <typename T>
T* StaticPoolBase<T>::alloc(size_t numElements) {
  if (numElements > m_sizeRemaining) {
    throw std::runtime_error("StaticPool overflow");
  }

  T* sequence = m_next;
  m_next += numElements;
  m_sizeRemaining -= numElements;
  return sequence;
}
