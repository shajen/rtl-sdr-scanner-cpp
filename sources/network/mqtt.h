#pragma once

#include <config.h>
#include <mqtt/client.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <vector>

class Mqtt {
 public:
  Mqtt(const Config& config);
  ~Mqtt();

  void publish(const std::string& topic, const std::string& data, int qos = 0);
  void publish(const std::string& topic, const std::vector<uint8_t>& data, int qos = 0);
  void publish(const std::string& topic, const std::vector<uint8_t>&& data, int qos = 0);
  void setMessageCallback(const std::string& topic, std::function<void(const std::string&)> callback);

 private:
  void connect();
  void onConnected();
  void onDisconnected();
  void onMessage(const std::string& topic, const std::string& data);
  void subscribe(const std::string& topic);

  const Config& m_config;
  mqtt::client m_client;
  std::atomic_bool m_isRunning;
  std::thread m_thread;
  std::mutex m_mutex;
  std::queue<std::tuple<std::string, std::vector<uint8_t>, int>> m_messages;
  std::set<std::string> m_topics;
  std::set<std::string> m_waitingTopics;
  std::vector<std::pair<std::string, std::function<void(const std::string&)>>> m_callbacks;
};
