#include "mqtt.h"

#include <logger.h>
#include <utils.h>

constexpr auto KEEP_ALIVE = 60;
constexpr auto LOOP_TIMEOUT_MS = 100;
constexpr auto QOS_SUB = 2;
constexpr auto RECONNECT_INTERVAL = std::chrono::seconds(1);
constexpr auto QUEUE_MAX_SIZE = 1000;

Mqtt::Mqtt(const Config &config)
    : m_client(mosquitto_new(nullptr, true, this)), m_isRunning(true), m_thread([this, config]() {
        Logger::info("Mqtt", "start thread id: {}", getThreadId());
        setThreadParams("mqtt", PRIORITY::LOW);
        mosquitto_username_pw_set(m_client, config.mqttUsername().c_str(), config.mqttPassword().c_str());
        mosquitto_connect_callback_set(m_client, [](mosquitto *, void *p, int) { reinterpret_cast<Mqtt *>(p)->onConnect(); });
        mosquitto_disconnect_callback_set(m_client, [](mosquitto *, void *p, int) { reinterpret_cast<Mqtt *>(p)->onDisconnect(); });
        mosquitto_message_callback_set(m_client, [](mosquitto *, void *p, const struct mosquitto_message *m) { reinterpret_cast<Mqtt *>(p)->onMessage(m); });
        mosquitto_connect(m_client, config.mqttHostname().c_str(), config.mqttPort(), KEEP_ALIVE);
        while (m_isRunning) {
          mosquitto_loop(m_client, LOOP_TIMEOUT_MS, 1);
          while (m_isRunning && !m_messages.empty()) {
            const auto &[topic, data, qos] = m_messages.front();
            mosquitto_publish(m_client, nullptr, topic.c_str(), data.size(), data.data(), qos, false);
            std::unique_lock lock(m_mutex);
            m_messages.pop();
          }
        }
        Logger::info("Mqtt", "stop thread id: {}", getThreadId());
      }) {}

Mqtt::~Mqtt() {
  m_isRunning = false;
  m_thread.join();
  mosquitto_disconnect(m_client);
  mosquitto_destroy(m_client);
}

void Mqtt::publish(const std::string &topic, const std::string &data, int qos) {
  std::unique_lock lock(m_mutex);
  if (m_messages.size() < QUEUE_MAX_SIZE) {
    m_messages.emplace(topic, std::vector<uint8_t>{data.begin(), data.end()}, qos);
    Logger::debug("Mqtt", "queue size: {}", m_messages.size());
  }
}

void Mqtt::publish(const std::string &topic, const std::vector<uint8_t> &data, int qos) {
  std::unique_lock lock(m_mutex);
  if (m_messages.size() < QUEUE_MAX_SIZE) {
    m_messages.emplace(topic, data, qos);
    Logger::debug("Mqtt", "queue size: {}", m_messages.size());
  }
}

void Mqtt::publish(const std::string &topic, const std::vector<uint8_t> &&data, int qos) {
  std::unique_lock lock(m_mutex);
  if (m_messages.size() < QUEUE_MAX_SIZE) {
    m_messages.emplace(topic, std::move(data), qos);
    Logger::debug("Mqtt", "queue size: {}", m_messages.size());
  }
}

void Mqtt::setMessageCallback(std::function<void(const std::string &, const std::string &)> callback) { m_callbacks.push_back(callback); }

void Mqtt::subscribe(const std::string &topic) {
  mosquitto_subscribe(m_client, nullptr, topic.c_str(), QOS_SUB);
  m_topics.push_back(topic);
}

void Mqtt::onConnect() {
  Logger::info("Mqtt", "connected");
  for (const auto &topic : m_topics) {
    Logger::info("Mqtt", "subscribe: {}", topic);
    mosquitto_subscribe(m_client, nullptr, topic.c_str(), QOS_SUB);
  }
}

void Mqtt::onDisconnect() {
  if (!m_isRunning) {
    return;
  }
  Logger::warn("Mqtt", "disconnected");
  while (m_isRunning && mosquitto_reconnect(m_client) != MOSQ_ERR_SUCCESS) {
    Logger::info("Mqtt", "reconnecting");
    std::this_thread::sleep_for(RECONNECT_INTERVAL);
  }
  Logger::info("Mqtt", "reconnecting success");
  std::unique_lock lock(m_mutex);
  while (!m_messages.empty()) {
    m_messages.pop();
  }
}

void Mqtt::onMessage(const mosquitto_message *message) {
  Logger::info("Mqtt", "topic: {}, data: {}", message->topic, static_cast<char *>(message->payload));
  const std::string topic(message->topic);
  const std::string data(static_cast<char *>(message->payload), message->payloadlen);
  for (auto &callback : m_callbacks) {
    callback(topic, data);
  }
}
