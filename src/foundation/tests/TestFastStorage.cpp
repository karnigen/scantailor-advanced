// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <DynamicPool.h>
#include <FastQueue.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(FastStorageTestSuite)

BOOST_AUTO_TEST_CASE(test_fast_queue_across_chunks_and_copy) {
  FastQueue<int> queue;
  for (int i = 0; i < 1500; ++i) {
    queue.push(i);
  }

  FastQueue<int> copy(queue);
  for (int i = 0; i < 1100; ++i) {
    BOOST_CHECK(queue.front() == i);
    queue.pop();
  }
  BOOST_CHECK(copy.front() == 0);

  FastQueue<int> other;
  other.push(-1);
  copy.swap(other);
  BOOST_CHECK(copy.front() == -1);
  BOOST_CHECK(other.front() == 0);
}

BOOST_AUTO_TEST_CASE(test_dynamic_pool_allocations) {
  DynamicPool<int> pool;
  int* first = pool.alloc(3);
  int* second = pool.alloc(9);
  int* third = pool.alloc(1);

  BOOST_CHECK(second == first + 3);
  first[0] = 10;
  second[8] = 22;
  third[0] = 33;
  BOOST_CHECK(first[0] == 10);
  BOOST_CHECK(second[8] == 22);
  BOOST_CHECK(third[0] == 33);
}

BOOST_AUTO_TEST_SUITE_END()
