#include "data_controller.h"

#include <logger.h>

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

DataController::DataController(Mqtt& mqtt) : m_mqtt(mqtt) {}

DataController::~DataController() = default;

void DataController::sendTransmission(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<std::complex<float>>& samples) {
  std::vector<uint8_t> data(sizeof(uint64_t) + 4 * sizeof(uint32_t) + sizeof(uint8_t) * 2 * samples.size());
  uint64_t offset = 0;
  add(data.data(), offset, static_cast<uint64_t>(time.count()));
  add(data.data(), offset, static_cast<uint32_t>(frequencyRange.start.value));
  add(data.data(), offset, static_cast<uint32_t>(frequencyRange.stop.value));
  add(data.data(), offset, static_cast<uint32_t>(frequencyRange.step.value));
  add(data.data(), offset, static_cast<uint32_t>(2 * samples.size()));
  for (const auto& value : samples) {
    add(data.data(), offset, static_cast<uint8_t>(value.real() * 127.5 + 127.5));
    add(data.data(), offset, static_cast<uint8_t>(value.imag() * 127.5 + 127.5));
  }
  m_mqtt.publish("sdr/transmission", std::move(data));
}

void DataController::sendTransmission(const std::chrono::milliseconds time, const FrequencyRange& frequencyRange, const std::vector<uint8_t>& samples) {
  std::vector<uint8_t> data(sizeof(uint64_t) + 4 * sizeof(uint32_t) + sizeof(uint8_t) * samples.size());
  uint64_t offset = 0;
  add(data.data(), offset, static_cast<uint64_t>(time.count()));
  add(data.data(), offset, static_cast<uint32_t>(frequencyRange.start.value));
  add(data.data(), offset, static_cast<uint32_t>(frequencyRange.stop.value));
  add(data.data(), offset, static_cast<uint32_t>(frequencyRange.step.value));
  add(data.data(), offset, static_cast<uint32_t>(samples.size()));
  add(data.data(), offset, samples.data(), samples.size());
  m_mqtt.publish("sdr/transmission", std::move(data));
}

void DataController::sendSignals(const std::vector<Signal>& signals, const FrequencyRange& frequencyRange, const std::chrono::milliseconds time) {
  const auto samplesSize = (frequencyRange.stop.value - frequencyRange.start.value) / frequencyRange.step.value + 1;
  std::vector<uint8_t> data(sizeof(uint64_t) + 4 * sizeof(uint32_t) + sizeof(int8_t) * samplesSize);
  uint64_t offset = 0;
  add(data.data(), offset, static_cast<uint64_t>(time.count()));
  add(data.data(), offset, static_cast<uint32_t>(frequencyRange.start.value));
  add(data.data(), offset, static_cast<uint32_t>(frequencyRange.stop.value));
  add(data.data(), offset, static_cast<uint32_t>(frequencyRange.step.value));
  add(data.data(), offset, static_cast<uint32_t>(samplesSize));
  for (const auto& signal : signals) {
    if (frequencyRange.start.value <= signal.frequency.value && signal.frequency.value <= frequencyRange.stop.value) {
      add(data.data(), offset, static_cast<int8_t>(signal.power.value));
    }
  }
  m_mqtt.publish("sdr/spectrogram", std::move(data));
}
