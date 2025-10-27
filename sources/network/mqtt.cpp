#include "mqtt.h"

#include <logger.h>
#include <utils/utils.h>

constexpr auto LABEL = "mqtt";
constexpr auto QOS_SUB = 2;
constexpr auto QUEUE_MAX_SIZE = 1000;
constexpr auto LOOP_TIMEOUT = std::chrono::milliseconds(10);
constexpr auto RECONNECT_INTERVAL = std::chrono::seconds(5);
constexpr auto CONNECT_TIMEOUT = std::chrono::seconds(5);
constexpr auto KEEP_ALIVE = std::chrono::seconds(60);

Mqtt::Mqtt(const Config& config)
    : m_config(config), m_client(config.mqttUrl(), "sdr-scanner"), m_isRunning(true), m_thread([this, config]() {
        Logger::info(LABEL, "started");
        connect();
        while (m_isRunning) {
          if (m_client.is_connected()) {
            std::shared_ptr<const mqtt::message> message;
            while (m_client.try_consume_message_for(&message, LOOP_TIMEOUT) && message) {
              onMessage(message->get_topic(), message->get_payload());
            }
            while (m_isRunning && !m_messages.empty()) {
              const auto& [topic, data, qos] = m_messages.front();
              m_client.publish(topic, data.data(), data.size(), qos, false);
              std::unique_lock lock(m_mutex);
              m_messages.pop();
            }
          } else {
            onDisconnected();
            while (m_isRunning && !m_client.is_connected()) {
              Logger::info(LABEL, "reconnecting...");
              connect();
              if (!m_client.is_connected()) {
                std::this_thread::sleep_for(RECONNECT_INTERVAL);
              }
            }
          }
        }
        Logger::info(LABEL, "stopped");
      }) {}

Mqtt::~Mqtt() {
  m_isRunning = false;
  m_thread.join();
  if (m_client.is_connected()) {
    m_client.disconnect();
  }
}

void Mqtt::publish(const std::string& topic, const std::string& data, int qos) {
  std::unique_lock lock(m_mutex);
  if (m_messages.size() < QUEUE_MAX_SIZE) {
    m_messages.emplace(topic, std::vector<uint8_t>{data.begin(), data.end()}, qos);
    Logger::trace(LABEL, "queue size: {}", m_messages.size());
  }
}

void Mqtt::publish(const std::string& topic, const std::vector<uint8_t>& data, int qos) {
  std::unique_lock lock(m_mutex);
  if (m_messages.size() < QUEUE_MAX_SIZE) {
    m_messages.emplace(topic, data, qos);
    Logger::trace(LABEL, "queue size: {}", m_messages.size());
  }
}

void Mqtt::publish(const std::string& topic, const std::vector<uint8_t>&& data, int qos) {
  std::unique_lock lock(m_mutex);
  if (m_messages.size() < QUEUE_MAX_SIZE) {
    m_messages.emplace(topic, std::move(data), qos);
    Logger::trace(LABEL, "queue size: {}", m_messages.size());
  }
}

void Mqtt::setMessageCallback(const std::string& topic, std::function<void(const std::string&)> callback) {
  subscribe(topic);
  m_callbacks.emplace_back(topic, callback);
}

void Mqtt::connect() {
  mqtt::ssl_options ssl_options;
  ssl_options.ca_path("/etc/ssl/certs");

  const auto options = mqtt::connect_options_builder()
                           .mqtt_version(MQTTVERSION_3_1_1)
                           .ssl(ssl_options)
                           .user_name(m_config.mqttUsername())
                           .password(m_config.mqttPassword())
                           .keep_alive_interval(KEEP_ALIVE)
                           .connect_timeout(CONNECT_TIMEOUT)
                           .automatic_reconnect(false)
                           .clean_session(true)
                           .finalize();

  try {
    const auto response = m_client.connect(options);
    if (response.is_session_present()) {
      Logger::info(LABEL, "session already present");
    } else {
      Logger::info(LABEL, "new session created");
    }
  } catch (const std::runtime_error& exception) {
    Logger::warn(LABEL, "exception: {}", exception.what());
  }
  if (m_client.is_connected()) {
    onConnected();
  }
}

void Mqtt::onConnected() {
  Logger::info(LABEL, "connected");
  std::unique_lock lock(m_mutex);

  for (const auto& topic : m_topics) {
    Logger::info(LABEL, "subscribe: {}", colored(GREEN, "{}", topic));
    m_client.subscribe(topic, QOS_SUB);
  }

  for (const auto& topic : m_waitingTopics) {
    Logger::info(LABEL, "subscribe: {}", colored(GREEN, "{}", topic));
    m_client.subscribe(topic, QOS_SUB);
    m_topics.insert(topic);
  }
  m_waitingTopics.clear();
}

void Mqtt::onDisconnected() { Logger::info(LABEL, "disconnected"); }

void Mqtt::subscribe(const std::string& topic) {
  std::unique_lock lock(m_mutex);
  if (m_client.is_connected()) {
    if (m_topics.count(topic) == 0) {
      Logger::info(LABEL, "subscribe: {}", colored(GREEN, "{}", topic));
      m_client.subscribe(topic, QOS_SUB);
      m_topics.insert(topic);
    }
  } else {
    m_waitingTopics.insert(topic);
  }
}

void Mqtt::onMessage(const std::string& topic, const std::string& data) {
  Logger::debug(LABEL, "topic: {}, data: {}", topic, data);
  for (auto& [callbackTopic, callback] : m_callbacks) {
    if (topic == callbackTopic) {
      callback(data);
    }
  }
}
