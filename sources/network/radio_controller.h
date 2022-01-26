#pragma once

#include <network/websocket_server.h>
#include <radio/help_structures.h>

#include <condition_variable>
#include <mutex>
#include <vector>

class RadioController {
 private:
  using Signals = std::vector<Signal>;
  using SignalsWithData = std::tuple<Signals, FrequencyRange, std::chrono::milliseconds>;
  using SignalsWithTime = std::pair<Signals, std::chrono::milliseconds>;

 public:
  RadioController(WebSocketServer& server);
  ~RadioController();

  void pushSignals(const Signals& signals, const FrequencyRange& frequencyRange, const std::chrono::milliseconds time);

 private:
  void processSignals(const SignalsWithData& signals);
  void sendSignals(const Signals& signals, const std::chrono::milliseconds time);
  void sendSignalsAndClear();

  WebSocketServer& m_server;
  std::queue<SignalsWithData> m_queue;
  std::vector<SignalsWithTime> m_signals;

  std::atomic_bool m_isRunning;
  std::thread m_thread;
  std::condition_variable m_cv;
  std::mutex m_mutex;
};
