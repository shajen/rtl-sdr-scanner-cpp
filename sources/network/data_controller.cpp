#include "data_controller.h"

#include <logger.h>
#include <utils.h>

#include <cstdlib>
#include <memory>

constexpr auto QUEUE_MAX_SIZE = 1000;

template <typename T>
void add(uint8_t* p, uint64_t& offset, const T& value) {
  auto tmp = reinterpret_cast<T*>(p + offset);
  *tmp = value;
  offset += sizeof(T);
}

template <typename T>
void add(uint8_t* p, uint64_t& offset, const T* value, int size) {
  auto tmp = reinterpret_cast<void*>(p + offset);
  memcpy(tmp, value, sizeof(T) * size);
  offset += sizeof(T) * size;
}

DataController::DataController(const Config& config, Mqtt& mqtt, const std::string& deviceName)
    : m_config(config), m_mqtt(mqtt), m_spectrogramTopic(std::string("sdr/" + deviceName + "/spectrogram")), m_transmissionsTopic(std::string("sdr/" + deviceName + "/transmission")) {}

DataController::~DataController() = default;

void DataController::pushTransmission(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<std::complex<float>>& samples, bool isActive) {
  std::unique_lock lock(m_mutex);
  if (m_transmissions.count(frequencyRange) == 0) {
    if (isActive) {
      Logger::info("DataCtrl", "start transmission {}", frequencyToString(frequencyRange.center()));
      m_transmissions.insert({frequencyRange, {time, time, {}}});
    } else {
      Logger::warn("DataCtrl", "start transmission not active {}", frequencyToString(frequencyRange.center()));
      return;
    }
  }
  auto& container = m_transmissions[frequencyRange];
  if (isActive) {
    container.lastActive = std::max(container.lastActive, time);
  }
  container.queue.push({time, samples, isActive});
  flushTransmission(frequencyRange);
}

void DataController::finishTransmission(const FrequencyRange& frequencyRange) {
  std::unique_lock lock(m_mutex);
  if (m_transmissions.count(frequencyRange) == 0) {
    Logger::warn("DataCtrl", "finish transmission not found {}", frequencyToString(frequencyRange.center()));
    return;
  }

  flushTransmission(frequencyRange);
  auto& container = m_transmissions[frequencyRange];
  const auto isMinimalTime = m_config.minRecordingTime() <= container.lastActive - container.firstActive;
  const auto duration = (container.lastActive - container.firstActive).count() / 1000.0;
  Logger::info("DataCtrl", "finish transmission {}, duration: {:.2f} seconds, reach minimum: {}", frequencyToString(frequencyRange.center()), duration, isMinimalTime);
  m_transmissions.erase(frequencyRange);
}

void DataController::flushTransmission(const FrequencyRange& frequencyRange) {
  if (m_transmissions.count(frequencyRange) == 0) {
    Logger::warn("DataCtrl", "flush transmission not found {}", frequencyToString(frequencyRange.center()));
    return;
  }

  auto& container = m_transmissions[frequencyRange];
  const auto isMinimalTime = m_config.minRecordingTime() <= container.lastActive - container.firstActive;
  if (isMinimalTime) {
    while (!container.queue.empty() && container.queue.front().time <= container.lastActive) {
      sendTransmission(frequencyRange, container.queue.front());
      container.queue.pop();
    }
  }
}

void DataController::sendTransmission(const FrequencyRange& frequencyRange, const Transmission& transmission) {
  std::vector<uint8_t> data(sizeof(uint64_t) + 2 * sizeof(Frequency) + sizeof(uint32_t) + sizeof(uint8_t) * 2 * transmission.samples.size());
  uint64_t offset = 0;
  add(data.data(), offset, static_cast<uint64_t>(transmission.time.count()));
  add(data.data(), offset, frequencyRange.start);
  add(data.data(), offset, frequencyRange.stop);
  add(data.data(), offset, static_cast<uint32_t>(2 * transmission.samples.size()));
  for (const auto& value : transmission.samples) {
    add(data.data(), offset, static_cast<uint8_t>(value.real() * 127.5 + 127.5));
    add(data.data(), offset, static_cast<uint8_t>(value.imag() * 127.5 + 127.5));
  }
  m_mqtt.publish(m_transmissionsTopic, std::move(data));
}

void DataController::sendSignals(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<Signal>& signals) {
  std::unique_lock lock(m_mutex);
  std::vector<uint8_t> data(sizeof(uint64_t) + 3 * sizeof(Frequency) + sizeof(uint32_t) + sizeof(int8_t) * signals.size());
  uint64_t offset = 0;
  add(data.data(), offset, static_cast<uint64_t>(time.count()));
  add(data.data(), offset, frequencyRange.start);
  add(data.data(), offset, frequencyRange.stop);
  add(data.data(), offset, frequencyRange.step);
  add(data.data(), offset, static_cast<uint32_t>(signals.size()));
  for (const auto& signal : signals) {
    add(data.data(), offset, static_cast<int8_t>(signal.power));
  }
  m_mqtt.publish(m_spectrogramTopic, std::move(data));
}
