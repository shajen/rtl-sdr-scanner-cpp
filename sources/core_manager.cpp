#include "core_manager.h"

#include <logger.h>

CoreManager::Core::Core(std::atomic_uint32_t &activeCores) : m_activeCores(activeCores) {
  m_activeCores++;
  Logger::debug("Corer", "reserve core, total: {}", m_activeCores);
}

CoreManager::Core::~Core() {
  m_activeCores--;
  Logger::debug("Core", "release core, total: {}", m_activeCores);
}

CoreManager::CoreManager(const uint32_t availableCores) : m_availableCores(availableCores), m_activeCores(0) {}

CoreManager::~CoreManager() = default;

std::unique_ptr<CoreManager::Core> CoreManager::getCore() {
  std::unique_lock<std::mutex> lock(m_mutex);
  if (m_activeCores < m_availableCores) {
    return std::make_unique<Core>(m_activeCores);
  } else {
    return nullptr;
  }
}
