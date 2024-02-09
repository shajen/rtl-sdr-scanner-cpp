#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>

template <typename T>
class Notification {
 public:
  Notification() = default;
  Notification(const Notification&) = delete;
  Notification& operator=(const Notification&) = delete;

  void notify(const T& value) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_value = value;
    m_cv.notify_all();
  }

  T wait() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]() { return m_value.has_value(); });
    T value = std::move(m_value.value());
    m_value = std::nullopt;
    return value;
  }

 private:
  std::mutex m_mutex;
  std::condition_variable m_cv;
  std::optional<T> m_value;
};
