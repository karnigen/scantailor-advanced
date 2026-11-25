// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_DEVIATIONPROVIDER_H_
#define SCANTAILOR_CORE_DEVIATIONPROVIDER_H_

#include <foundation/NonCopyable.h>

#include <cmath>
#include <functional>
#include <limits>
#include <mutex>
#include <unordered_map>

template <typename K, typename Hash = std::hash<K>>
class DeviationProvider {
  DECLARE_NON_COPYABLE(DeviationProvider)
 public:
  DeviationProvider() = default;

  explicit DeviationProvider(const std::function<double(const K&)>& computeValueByKey);

  bool isDeviant(const K& key, double coefficient = 1.0, double threshold = 0.0, bool defaultVal = false) const;

  double getDeviationValue(const K& key) const;

  std::unordered_map<K, double, Hash> getDeviationValuesSnapshot() const;

  void addOrUpdate(const K& key);

  void addOrUpdate(const K& key, double value);

  void remove(const K& key);

  void clear();

  void setComputeValueByKey(const std::function<double(const K&)>& computeValueByKey);

 private:
  // Keeps the statistics in sync with the stored value. Requires m_mutex.
  void store(const K& key, double value);

  // Running statistics over non-NaN values. Requiring m_mutex keeps readers O(1).
  void addToStatistics(double value);
  void removeFromStatistics(double value);
  double meanValueLocked() const;
  double standardDeviationLocked() const;

  mutable std::mutex m_mutex;
  std::function<double(const K&)> m_computeValueByKey;
  std::unordered_map<K, double, Hash> m_keyValueMap;

  int m_valueCount = 0;
  double m_valueSum = 0.0;
  double m_valueSquareSum = 0.0;
};


template <typename K, typename Hash>
DeviationProvider<K, Hash>::DeviationProvider(const std::function<double(const K&)>& computeValueByKey)
    : m_computeValueByKey(computeValueByKey) {}

template <typename K, typename Hash>
bool DeviationProvider<K, Hash>::isDeviant(const K& key, double coefficient, double threshold, bool defaultVal) const {
  const std::lock_guard<std::mutex> lock(m_mutex);

  const auto it = m_keyValueMap.find(key);
  if (it == m_keyValueMap.end()) {
    return false;
  }
  if (m_keyValueMap.size() < 3) {
    return false;
  }

  const double value = it->second;
  if (std::isnan(value)) {
    return defaultVal;
  }

  const double deviation = std::abs(value - meanValueLocked());
  return deviation > std::max((coefficient * standardDeviationLocked()), (threshold / 100) * meanValueLocked());
}

template <typename K, typename Hash>
double DeviationProvider<K, Hash>::getDeviationValue(const K& key) const {
  const std::lock_guard<std::mutex> lock(m_mutex);

  const auto it = m_keyValueMap.find(key);
  if (it == m_keyValueMap.end()) {
    return -1.0;
  }
  if (m_keyValueMap.size() < 2) {
    return .0;
  }

  const double value = it->second;
  if (std::isnan(value)) {
    return -1.0;
  }

  return std::abs(value - meanValueLocked());
}

template <typename K, typename Hash>
std::unordered_map<K, double, Hash> DeviationProvider<K, Hash>::getDeviationValuesSnapshot() const {
  const std::lock_guard<std::mutex> lock(m_mutex);

  const bool tooFewValues = m_keyValueMap.size() < 2;
  const double mean = meanValueLocked();

  std::unordered_map<K, double, Hash> snapshot;
  snapshot.reserve(m_keyValueMap.size());
  for (const auto& [key, value] : m_keyValueMap) {
    const double deviation = tooFewValues ? 0.0 : (std::isnan(value) ? -1.0 : std::abs(value - mean));
    snapshot.emplace(key, deviation);
  }
  return snapshot;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::addOrUpdate(const K& key) {
  std::function<double(const K&)> computeValueByKey;
  {
    const std::lock_guard<std::mutex> lock(m_mutex);
    computeValueByKey = m_computeValueByKey;
  }
  // The callback may take a filter lock, so never invoke it while holding m_mutex.
  const double value = computeValueByKey(key);

  const std::lock_guard<std::mutex> lock(m_mutex);
  store(key, value);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::addOrUpdate(const K& key, const double value) {
  const std::lock_guard<std::mutex> lock(m_mutex);
  store(key, value);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::store(const K& key, const double value) {
  const auto it = m_keyValueMap.find(key);
  if (it == m_keyValueMap.end()) {
    m_keyValueMap.emplace(key, value);
    addToStatistics(value);
    return;
  }
  if ((it->second == value) || (std::isnan(it->second) && std::isnan(value))) {
    return;
  }
  removeFromStatistics(it->second);
  addToStatistics(value);
  it->second = value;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::addToStatistics(const double value) {
  if (std::isnan(value)) {
    return;
  }
  ++m_valueCount;
  m_valueSum += value;
  m_valueSquareSum += value * value;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::removeFromStatistics(const double value) {
  if (std::isnan(value)) {
    return;
  }
  --m_valueCount;
  if (m_valueCount <= 0) {
    m_valueCount = 0;
    m_valueSum = 0.0;
    m_valueSquareSum = 0.0;
    return;
  }
  m_valueSum -= value;
  m_valueSquareSum -= value * value;
  if (m_valueSquareSum < 0.0) {
    m_valueSquareSum = 0.0;
  }
}

template <typename K, typename Hash>
double DeviationProvider<K, Hash>::meanValueLocked() const {
  if (m_valueCount == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return m_valueSum / m_valueCount;
}

template <typename K, typename Hash>
double DeviationProvider<K, Hash>::standardDeviationLocked() const {
  if (m_valueCount < 2) {
    // Matches the historical behaviour of a sample deviation over a single value.
    return std::numeric_limits<double>::quiet_NaN();
  }
  const double variance = (m_valueSquareSum - m_valueSum * (m_valueSum / m_valueCount)) / (m_valueCount - 1);
  return variance > 0.0 ? std::sqrt(variance) : 0.0;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::remove(const K& key) {
  const std::lock_guard<std::mutex> lock(m_mutex);
  const auto it = m_keyValueMap.find(key);
  if (it == m_keyValueMap.end()) {
    return;
  }
  removeFromStatistics(it->second);
  m_keyValueMap.erase(it);
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::setComputeValueByKey(const std::function<double(const K&)>& computeValueByKey) {
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_computeValueByKey = computeValueByKey;
}

template <typename K, typename Hash>
void DeviationProvider<K, Hash>::clear() {
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_keyValueMap.clear();

  m_valueCount = 0;
  m_valueSum = 0.0;
  m_valueSquareSum = 0.0;
}


#endif  // SCANTAILOR_CORE_DEVIATIONPROVIDER_H_
