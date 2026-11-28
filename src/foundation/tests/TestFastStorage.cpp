// SPDX-FileCopyrightText: 2026 project contributors
// SPDX-License-Identifier: GPL-3.0-only

#include <DynamicPool.h>
#include <FastQueue.h>

#include <catch2/catch_test_macros.hpp>


TEST_CASE("FastStorageTestSuite / test_fast_queue_across_chunks_and_copy") {
  FastQueue<int> queue;
  for (int i = 0; i < 1500; ++i) {
    queue.push(i);
  }

  FastQueue<int> copy(queue);
  for (int i = 0; i < 1100; ++i) {
    CHECK(queue.front() == i);
    queue.pop();
  }
  CHECK(copy.front() == 0);

  FastQueue<int> other;
  other.push(-1);
  copy.swap(other);
  CHECK(copy.front() == -1);
  CHECK(other.front() == 0);
}

TEST_CASE("FastStorageTestSuite / test_dynamic_pool_allocations") {
  DynamicPool<int> pool;
  int* first = pool.alloc(3);
  int* second = pool.alloc(9);
  int* third = pool.alloc(1);

  CHECK(second == first + 3);
  first[0] = 10;
  second[8] = 22;
  third[0] = 33;
  CHECK(first[0] == 10);
  CHECK(second[8] == 22);
  CHECK(third[0] == 33);
}
