#pragma once

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>

template <typename T>
int getMaxIndex(const T* data, const int size, const int index, const int groupSize) {
  const auto min = std::max(0, index - groupSize / 2);
  const auto max = std::min(size, index + groupSize / 2 + 1);
  const auto it = std::max_element(data + min, data + max);
  return std::distance(data, it);
}

template <typename T>
std::optional<int> containsWithMargin(const std::map<int, T>& indexes, const int index, const int margin) {
  const auto submargin = margin % 2 == 0 ? margin / 2 : margin / 2 + 1;
  const auto left = index - submargin;
  const auto right = index + submargin;
  auto it = indexes.lower_bound(left);
  if (it != indexes.end() && it->first <= right) {
    return it->first;
  } else {
    return std::nullopt;
  }
}

template <typename T>
T mostFrequentValue(const std::vector<T>& data) {
  auto f = [](const auto& v1, const auto& v2) {
    if (v1.second == v2.second) {
      return v1.first < v2.first;
    } else {
      return v1.second < v2.second;
    }
  };

  std::unordered_map<T, int> counter;
  for (const auto& value : data) {
    counter[value]++;
  }

  std::vector<std::pair<T, int>> buffer;
  std::transform(counter.begin(), counter.end(), std::back_inserter(buffer), [](auto& kv) { return kv; });
  const auto it = std::max_element(buffer.begin(), buffer.end(), f);
  buffer.erase(std::remove_if(buffer.begin(), buffer.end(), [it](const auto& kv) { return kv.second != it->second; }), buffer.end());
  std::sort(buffer.begin(), buffer.end(), f);
  return buffer[buffer.size() / 2].first;
}

template <typename T>
T getNearestElement(const std::set<T>& data, const T& value) {
  const auto next = data.lower_bound(value);
  if (next == data.end()) {
    return *data.rbegin();
  } else if (next == data.begin()) {
    return *next;
  } else {
    const auto prev = std::prev(next);
    if (*next - value <= value - *prev) {
      return *next;
    } else {
      return *prev;
    }
  }
}