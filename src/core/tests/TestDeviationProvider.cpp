// Copyright (C) 2026  ScanTailor Advanced contributors
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <DeviationProvider.h>

#include <atomic>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <limits>
#include <thread>
#include <vector>

TEST_CASE("DeviationProviderTestSuite / unknown keys have no deviation") {
  DeviationProvider<int> provider;
  CHECK_FALSE(provider.isDeviant(1));
  CHECK(provider.getDeviationValue(1) == -1.0);
}

TEST_CASE("DeviationProviderTestSuite / detects and measures an outlier") {
  DeviationProvider<int> provider;
  for (int i = 0; i < 9; ++i) {
    provider.addOrUpdate(i, 10.0);
  }
  provider.addOrUpdate(9, 100.0);

  CHECK(provider.isDeviant(9));
  CHECK_FALSE(provider.isDeviant(0));
  CHECK(provider.getDeviationValue(9) == Catch::Approx(81.0));
  CHECK(provider.getDeviationValue(0) == Catch::Approx(9.0));
}

TEST_CASE("DeviationProviderTestSuite / statistics track changed values and removals") {
  DeviationProvider<int> provider;
  for (int i = 0; i < 10; ++i) {
    provider.addOrUpdate(i, 10.0);
  }
  CHECK_FALSE(provider.isDeviant(3));
  CHECK(provider.getDeviationValue(3) == Catch::Approx(0.0).margin(1e-12));

  provider.addOrUpdate(3, 10.0);
  CHECK_FALSE(provider.isDeviant(3));

  provider.addOrUpdate(3, 500.0);
  CHECK(provider.isDeviant(3));
  CHECK(provider.getDeviationValue(3) == Catch::Approx(441.0));

  provider.remove(3);
  CHECK_FALSE(provider.isDeviant(3));
  CHECK(provider.getDeviationValue(0) == Catch::Approx(0.0).margin(1e-12));

  provider.clear();
  CHECK(provider.getDeviationValue(0) == -1.0);
}

TEST_CASE("DeviationProviderTestSuite / NaN values are ignored") {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  DeviationProvider<int> provider;
  provider.addOrUpdate(0, 10.0);
  provider.addOrUpdate(1, 10.0);
  provider.addOrUpdate(2, 10.0);
  provider.addOrUpdate(3, nan);
  provider.addOrUpdate(3, nan);

  CHECK_FALSE(provider.isDeviant(3));
  CHECK(provider.isDeviant(3, 1.0, 0.0, true));
  CHECK(provider.getDeviationValue(3) == -1.0);
  CHECK(provider.getDeviationValue(0) == Catch::Approx(0.0).margin(1e-12));
}

TEST_CASE("DeviationProviderTestSuite / computes values through its callback") {
  DeviationProvider<int> provider([](const int& key) { return key == 7 ? 70.0 : 1.0; });
  for (int i = 0; i < 10; ++i) {
    provider.addOrUpdate(i);
  }

  CHECK(provider.isDeviant(7));
  CHECK_FALSE(provider.isDeviant(1));
}

TEST_CASE("DeviationProviderTestSuite / concurrent updates and queries stay coherent") {
  DeviationProvider<int> provider;
  std::atomic<int> ready{0};
  std::atomic<bool> start{false};
  std::atomic<bool> stop{false};

  std::vector<std::thread> writers;
  for (int thread = 0; thread < 4; ++thread) {
    writers.emplace_back([&provider, &ready, &start, thread] {
      ++ready;
      while (!start.load()) {
        std::this_thread::yield();
      }
      for (int i = 0; i < 10000; ++i) {
        const int key = thread * 10000 + i;
        provider.addOrUpdate(key, static_cast<double>(i % 17));
        if (i % 7 == 0) {
          provider.remove(key);
        }
      }
    });
  }

  std::thread reader([&provider, &ready, &start, &stop] {
    ++ready;
    while (!start.load()) {
      std::this_thread::yield();
    }
    while (!stop.load()) {
      for (int key = 0; key < 40000; key += 997) {
        provider.isDeviant(key, 1.0, 1.0);
        provider.getDeviationValue(key);
      }
    }
  });

  while (ready.load() != 5) {
    std::this_thread::yield();
  }
  start = true;
  for (std::thread& writer : writers) {
    writer.join();
  }
  stop = true;
  reader.join();

  provider.addOrUpdate(-1, 3.0);
  CHECK(provider.getDeviationValue(-1) >= 0.0);
}
