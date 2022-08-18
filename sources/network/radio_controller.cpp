#include "radio_controller.h"

#include <logger.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

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

RadioController::RadioController(Mqtt& mqtt)
    : m_mqtt(mqtt), m_isRunning(true), m_thread([this]() {
        while (m_isRunning) {
          {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock);
            while (m_isRunning && !m_queue.empty()) {
              Logger::debug("Radio", "queue size: {}", m_queue.size());
              const auto data = std::move(m_queue.front());
              m_queue.pop();
              lock.unlock();
              processSignals(data);
              lock.lock();
            }
          }
        }
      }) {}

RadioController::~RadioController() {
  m_isRunning = false;
  m_cv.notify_one();
  m_thread.join();
}

void RadioController::pushSignals(const Signals& signals, const FrequencyRange& frequencyRange, const std::chrono::milliseconds time) {
  std::unique_lock<std::mutex> lock(m_mutex);
  if (m_queue.size() < QUEUE_MAX_SIZE) {
    m_queue.push({signals, frequencyRange, time});
  } else {
    Logger::warn("Radio", "queue size: {}, queue is full", m_queue.size());
  }
  m_cv.notify_one();
}

void RadioController::pushRecording(const std::chrono::milliseconds& time, const Frequency& frequency, Frequency sampleRate, const std::vector<float>& samples) {
  std::vector<uint8_t> data(sizeof(uint64_t) + 3 * sizeof(uint32_t) + sizeof(float) * samples.size());
  uint64_t offset = 0;
  add(data.data(), offset, static_cast<uint64_t>(time.count()));
  add(data.data(), offset, static_cast<uint32_t>(frequency.value));
  add(data.data(), offset, static_cast<uint32_t>(sampleRate.value));
  add(data.data(), offset, static_cast<uint32_t>(samples.size()));
  add(data.data(), offset, samples.data(), samples.size());
  m_mqtt.publish("sdr/recording/continue", std::move(data));
}

void RadioController::finishRecording(const Frequency& frequency) {
  std::vector<uint8_t> data(sizeof(uint32_t));
  uint64_t offset = 0;
  add(data.data(), offset, static_cast<u_int32_t>(frequency.value));
  m_mqtt.publish("sdr/recording/finish", std::move(data));
}

void RadioController::processSignals(const SignalsWithData& signalsWithRange) {
  const Signals& signals = std::get<0>(signalsWithRange);
  const FrequencyRange& frequencyRange = std::get<1>(signalsWithRange);
  const auto time = std::get<2>(signalsWithRange);

  auto start = std::find_if(signals.begin(), signals.end(), [&frequencyRange](const Signal& signal) { return signal.frequency == frequencyRange.start; });
  auto stop = std::find_if(signals.begin(), signals.end(), [&frequencyRange](const Signal& signal) { return signal.frequency == frequencyRange.stop; });

  auto pushNewRange = [this, &time, &start, &stop]() {
    Signals signals;
    std::copy(start, stop + 1, std::back_inserter(signals));
    m_signals.push_back({signals, time});
  };

  if (m_signals.empty()) {  // start working
    pushNewRange();
  } else if (m_signals.front().first.front().frequency == start->frequency) {  // repeated range
    sendSignalsAndClear();
    pushNewRange();
  } else if (m_signals.back().first.back().frequency == start->frequency) {  // extend range
    std::copy(start + 1, stop + 1, std::back_inserter(m_signals.back().first));
  } else if (m_signals.back().first.back().frequency == stop->frequency) {  // recording
    sendSignalsAndClear();
    pushNewRange();
  } else {  // new range
    pushNewRange();
  }
}

void RadioController::sendSignals(const Signals& signals, const std::chrono::milliseconds time) {
  std::vector<uint8_t> data(sizeof(uint64_t) + sizeof(uint32_t)+ (sizeof(uint32_t) + sizeof(float)) * signals.size());
  uint64_t offset = 0;
  add(data.data(), offset, static_cast<uint64_t>(time.count()));
  add(data.data(), offset, static_cast<uint32_t>(signals.size()));
  for (const auto& signal : signals) {
    add(data.data(), offset, static_cast<uint32_t>(signal.frequency.value));
  }
  for (const auto& signal : signals) {
    add(data.data(), offset, signal.power.value);
  }
  m_mqtt.publish("sdr/spectrogram", std::move(data));
}

void RadioController::sendSignalsAndClear() {
  for (const auto& signals : m_signals) {
    sendSignals(std::get<0>(signals), std::get<1>(signals));
  }
  m_signals.clear();
}
