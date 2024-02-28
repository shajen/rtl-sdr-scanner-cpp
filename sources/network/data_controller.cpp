#include "data_controller.h"

#include <string.h>

#include <cstdlib>
#include <memory>

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

DataController::DataController(Mqtt& mqtt, const std::string& deviceName)
    : m_mqtt(mqtt), m_spectrogramTopic(std::string("sdr/" + deviceName + "/spectrogram")), m_transmissionsTopic(std::string("sdr/" + deviceName + "/transmission")) {}

DataController::~DataController() = default;

void DataController::pushTransmission(const std::chrono::milliseconds time, const Frequency& frequency, const Frequency& sampleRate, const TransmissionData* data, int size) {
  const Frequency start = frequency - sampleRate / 2;
  const Frequency stop = frequency + sampleRate / 2;
  std::vector<uint8_t> payload(sizeof(uint64_t) + 2 * sizeof(Frequency) + sizeof(uint32_t) + sizeof(TransmissionData) * size);
  uint64_t offset = 0;
  add(payload.data(), offset, static_cast<uint64_t>(time.count()));
  add(payload.data(), offset, start);
  add(payload.data(), offset, stop);
  add(payload.data(), offset, static_cast<uint32_t>(size));
  add(payload.data(), offset, data, size);
  m_mqtt.publish(m_transmissionsTopic, std::move(payload));
}

void DataController::pushSpectrogram(const std::chrono::milliseconds time, const Frequency& frequency, const Frequency& sampleRate, const SpectrogramData* data, int size) {
  const Frequency start = frequency - sampleRate / 2;
  const Frequency stop = frequency + sampleRate / 2;
  const Frequency step = sampleRate / size;
  std::vector<uint8_t> payload(sizeof(uint64_t) + 3 * sizeof(Frequency) + sizeof(uint32_t) + sizeof(SpectrogramData) * size);
  uint64_t offset = 0;
  add(payload.data(), offset, static_cast<uint64_t>(time.count()));
  add(payload.data(), offset, start);
  add(payload.data(), offset, stop);
  add(payload.data(), offset, step);
  add(payload.data(), offset, static_cast<uint32_t>(size));
  add(payload.data(), offset, data, size);
  m_mqtt.publish(m_spectrogramTopic, std::move(payload));
}
