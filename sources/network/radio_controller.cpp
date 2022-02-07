#include "radio_controller.h"

#include <logger.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

constexpr auto QUEUE_MAX_SIZE = 1000;

RadioController::RadioController(WebSocketServer& server)
    : m_server(server), m_isRunning(true), m_thread([this]() {
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
  rapidjson::Document document;
  rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

  rapidjson::Value frequencies(rapidjson::kArrayType);
  rapidjson::Value powers(rapidjson::kArrayType);
  frequencies.Reserve(signals.size(), allocator);
  powers.Reserve(signals.size(), allocator);
  for (const auto& signal : signals) {
    frequencies.PushBack(rapidjson::Value(signal.frequency.value), allocator);
    powers.PushBack(rapidjson::Value(signal.power.value), allocator);
  }
  document.SetObject();
  document.AddMember("type", "spectrogram", allocator);
  document.AddMember("frequencies", frequencies, allocator);
  document.AddMember("powers", powers, allocator);
  document.AddMember("timestamp_ms", time.count(), allocator);

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  writer.SetMaxDecimalPlaces(2);
  document.Accept(writer);
  const std::string message(buffer.GetString());
  m_server.send(message);
}

void RadioController::sendSignalsAndClear() {
  for (const auto& signals : m_signals) {
    sendSignals(std::get<0>(signals), std::get<1>(signals));
  }
  m_signals.clear();
}
