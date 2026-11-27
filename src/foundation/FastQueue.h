// SPDX-FileCopyrightText: 2019 Joseph Artsimovich <joseph.artsimovich@gmail.com>
// SPDX-FileCopyrightText: 2019 4lex4 <4lex49@zoho.com>
// SPDX-License-Identifier: GPL-3.0-only

#ifndef SCANTAILOR_FOUNDATION_FASTQUEUE_H_
#define SCANTAILOR_FOUNDATION_FASTQUEUE_H_

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <new>

#include "NonCopyable.h"

template <typename T>
class FastQueue {
 public:
  FastQueue() : m_chunkCapacity(defaultChunkCapacity()) {}

  FastQueue(const FastQueue& other);

  ~FastQueue() { clear(); }

  FastQueue& operator=(const FastQueue& other);

  bool empty() const { return m_head == nullptr; }

  T& front() { return *m_head->pBegin; }

  const T& front() const { return *m_head->pBegin; }

  void push(const T& t);

  void pop();

  void swap(FastQueue& other);

 private:
  struct Chunk {
    DECLARE_NON_COPYABLE(Chunk)

   public:
    explicit Chunk(size_t capacity) : next(nullptr) {
      const std::uintptr_t p = (std::uintptr_t) (this + 1);
      const size_t alignment = alignof(T);
      pBegin = (T*) (((p + alignment - 1) / alignment) * alignment);
      pEnd = pBegin;
      pBufferEnd = pBegin + capacity;
      assert(size_t((char*) pBufferEnd - (char*) this) <= storageRequirement(capacity));
    }

    ~Chunk() {
      for (; pBegin != pEnd; ++pBegin) {
        pBegin->~T();
      }
    }

    static size_t storageRequirement(size_t capacity) { return sizeof(Chunk) + alignof(T) - 1 + capacity * sizeof(T); }

    Chunk* next;
    T* pBegin;
    T* pEnd;
    T* pBufferEnd;
    // An implicit array of T follows.
  };

  struct ChunkDisposer {
    void operator()(Chunk* chunk) {
      chunk->~Chunk();
      delete[] (char*) chunk;
    }
  };

  static size_t defaultChunkCapacity() { return (sizeof(T) >= 4096) ? 1 : 4096 / sizeof(T); }

  void clear() {
    while (m_head) {
      Chunk* chunk = m_head;
      m_head = chunk->next;
      ChunkDisposer()(chunk);
    }
    m_tail = nullptr;
  }

  Chunk* m_head = nullptr;
  Chunk* m_tail = nullptr;
  size_t m_chunkCapacity;
};


template <typename T>
FastQueue<T>::FastQueue(const FastQueue& other) : m_chunkCapacity(other.m_chunkCapacity) {
  try {
    for (const Chunk* chunk = other.m_head; chunk; chunk = chunk->next) {
      for (const T* obj = chunk->pBegin; obj != chunk->pEnd; ++obj) {
        push(*obj);
      }
    }
  } catch (...) {
    clear();
    throw;
  }
}

template <typename T>
FastQueue<T>& FastQueue<T>::operator=(const FastQueue& other) {
  FastQueue(other).swap(*this);
  return *this;
}

template <typename T>
void FastQueue<T>::push(const T& t) {
  Chunk* chunk = m_tail;
  Chunk* const previousTail = m_tail;

  if (chunk && chunk->pEnd == chunk->pBufferEnd) {
    chunk = nullptr;
  }

  const bool createdChunk = !chunk;
  if (!chunk) {
    // Create a new chunk.
    char* buf = new char[Chunk::storageRequirement(m_chunkCapacity)];
    chunk = new (buf) Chunk(m_chunkCapacity);
    if (m_tail) {
      m_tail->next = chunk;
    } else {
      m_head = chunk;
    }
    m_tail = chunk;
  }

  try {
    new (chunk->pEnd) T(t);
  } catch (...) {
    if (createdChunk) {
      if (previousTail) {
        previousTail->next = nullptr;
      } else {
        m_head = nullptr;
      }
      m_tail = previousTail;
      ChunkDisposer()(chunk);
    }
    throw;
  }
  ++chunk->pEnd;
}

template <typename T>
void FastQueue<T>::pop() {
  assert(!empty());

  Chunk* chunk = m_head;
  chunk->pBegin->~T();
  ++chunk->pBegin;
  if (chunk->pBegin == chunk->pEnd) {
    m_head = chunk->next;
    if (!m_head) {
      m_tail = nullptr;
    }
    ChunkDisposer()(chunk);
  }
}

template <typename T>
void FastQueue<T>::swap(FastQueue& other) {
  Chunk* const head = m_head;
  m_head = other.m_head;
  other.m_head = head;
  Chunk* const tail = m_tail;
  m_tail = other.m_tail;
  other.m_tail = tail;
  const size_t tmp = m_chunkCapacity;
  m_chunkCapacity = other.m_chunkCapacity;
  other.m_chunkCapacity = tmp;
}

template <typename T>
inline void swap(FastQueue<T>& o1, FastQueue<T>& o2) {
  o1.swap(o2);
}

#endif  // ifndef SCANTAILOR_FOUNDATION_FASTQUEUE_H_
