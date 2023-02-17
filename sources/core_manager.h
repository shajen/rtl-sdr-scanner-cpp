#pragma once

#include <atomic>
#include <memory>
#include <mutex>

class CoreManager {
 public:
  class Core {
   public:
    Core(std::atomic_uint32_t& activeCores);
    ~Core();

   private:
    std::atomic_uint32_t& m_activeCores;
  };

  CoreManager(const uint32_t availableCores);
  ~CoreManager();

  std::unique_ptr<Core> getCore();

 private:
  const uint32_t m_availableCores;
  std::mutex m_mutex;
  std::atomic_uint32_t m_activeCores;
};
