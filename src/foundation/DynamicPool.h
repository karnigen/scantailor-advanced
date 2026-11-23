// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_FOUNDATION_DYNAMICPOOL_H_
#define SCANTAILOR_FOUNDATION_DYNAMICPOOL_H_

#include <cstddef>
#include <memory>

#include "NonCopyable.h"

/**
 * \brief Allocates objects from the heap.
 *
 * There is no way of freeing the allocated objects
 * besides destroying the whole pool.
 */
template <typename T>
class DynamicPool {
  DECLARE_NON_COPYABLE(DynamicPool)

 public:
  DynamicPool() {}

  ~DynamicPool();

  /**
   * \brief Allocates a sequence of objects.
   *
   * If T is a POD type, the returned objects are uninitialized,
   * otherwise they are default-constructed.
   */
  T* alloc(size_t numElements);

 private:
  enum { OVERALLOCATION_FACTOR = 3 };

  /**< Allocate 3 times the requested size. */
  enum { OVERALLOCATION_LIMIT = 256 };

  /**< Don't overallocate too much. */

  struct Chunk {
    Chunk* next;
    std::unique_ptr<T[]> storage;
    T* pData;
    size_t remainingElements;

    Chunk() : next(nullptr), pData(nullptr), remainingElements(0) {}

    void init(std::unique_ptr<T[]>& data, size_t size) {
      data.swap(storage);
      pData = storage.get();
      remainingElements = size;
    }
  };

  static size_t adviseChunkSize(size_t numElements);

  Chunk* m_head = nullptr;
  Chunk* m_tail = nullptr;
};


template <typename T>
DynamicPool<T>::~DynamicPool() {
  while (m_head) {
    Chunk* chunk = m_head;
    m_head = chunk->next;
    delete chunk;
  }
}

template <typename T>
T* DynamicPool<T>::alloc(size_t numElements) {
  Chunk* chunk = m_tail;

  if (chunk && chunk->remainingElements < numElements) {
    chunk = nullptr;
  }

  if (!chunk) {
    // Create a new chunk.
    const size_t chunkSize = adviseChunkSize(numElements);
    std::unique_ptr<T[]> data(new T[chunkSize]);
    std::unique_ptr<Chunk> newChunk(new Chunk);
    newChunk->init(data, chunkSize);
    chunk = newChunk.get();
    if (m_tail) {
      m_tail->next = chunk;
    } else {
      m_head = chunk;
    }
    m_tail = newChunk.release();
  }

  // Allocate from chunk.
  T* data = chunk->pData;
  chunk->pData += numElements;
  chunk->remainingElements -= numElements;
  return data;
}

template <typename T>
size_t DynamicPool<T>::adviseChunkSize(size_t numElements) {
  size_t factor = OVERALLOCATION_LIMIT / numElements;
  if (factor > (size_t) OVERALLOCATION_FACTOR) {
    factor = OVERALLOCATION_FACTOR;
  }
  return numElements * (factor + 1);
}

#endif  // ifndef SCANTAILOR_FOUNDATION_DYNAMICPOOL_H_
