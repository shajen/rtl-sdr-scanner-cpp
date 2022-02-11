#include "websocket_server_session.h"

#include <logger.h>
#include <rapidjson/document.h>

constexpr auto QUEUE_THRESHOLD_SIZE = 900;
constexpr auto QUEUE_MAX_SIZE = 1000;
constexpr auto TIMEOUT = std::chrono::seconds(30);

WebSocketServerSession::WebSocketServerSession(const std::string& key, const std::string& remoteAddress, boost::asio::ip::tcp::socket&& socket)
    : m_key(key), m_remoteAddress(remoteAddress), m_isQueueFullWasReported(false), m_isReady(false), m_isAlive(true), m_isAuthorized(false), m_ws(std::move(socket)), m_isWriting(false) {
  run();
}

WebSocketServerSession::~WebSocketServerSession() { Logger::info("WsSession", "[{}] connection removed", m_remoteAddress); }

void WebSocketServerSession::send(const std::string& message) {
  if (!m_isAuthorized || !m_isReady || !m_isAlive) {
    return;
  }
  if (m_messages.size() < QUEUE_MAX_SIZE) {
    m_messages.push(message);
    write();
    if (m_isQueueFullWasReported) {
      m_isQueueFullWasReported = m_messages.size() <= QUEUE_THRESHOLD_SIZE;
    }
  } else {
    if (!m_isQueueFullWasReported) {
      Logger::warn("WsSession", "[{}] queue size: {}, queue is full", m_remoteAddress, m_messages.size());
    }
    m_isQueueFullWasReported = true;
  }
}

bool WebSocketServerSession::isAlive() const { return m_isAlive; }

bool WebSocketServerSession::authorize(const std::string& message) const {
  try {
    rapidjson::Document document;
    document.Parse(message.c_str());
    const auto userKey = std::string(document["key"].GetString());
    const auto authorized = document["command"] == "authorize" && (m_key.empty() || userKey == m_key);
    if (authorized) {
      Logger::info("WsSession", "[{}] authorized", m_remoteAddress);
    }
    return authorized;
  } catch (const std::exception& exception) {
    return false;
  }
}

void WebSocketServerSession::run() { boost::asio::dispatch(m_ws.get_executor(), boost::beast::bind_front_handler(&WebSocketServerSession::onRun, this)); }

void WebSocketServerSession::onRun() {
  boost::beast::websocket::stream_base::timeout opt{TIMEOUT, TIMEOUT, true};
  m_ws.set_option(opt);
  m_ws.set_option(boost::beast::websocket::stream_base::decorator(
      [](boost::beast::websocket::response_type& res) { res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async"); }));
  m_ws.async_accept(boost::beast::bind_front_handler(&WebSocketServerSession::onAccept, this));
}

void WebSocketServerSession::onAccept(boost::beast::error_code ec) {
  if (ec) {
    Logger::warn("WsSession", "[{}] accept error: {}", m_remoteAddress, ec.message());
    m_isAlive = false;
  } else {
    Logger::info("WsSession", "[{}] accept success", m_remoteAddress);
    m_isReady = true;
    read();
  }
}

void WebSocketServerSession::read() { m_ws.async_read(m_readBuffer, boost::beast::bind_front_handler(&WebSocketServerSession::onRead, this)); }

void WebSocketServerSession::onRead(boost::beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    m_isAlive = false;
    if (ec == boost::beast::websocket::error::closed) {
      Logger::info("WsSession", "[{}] connection closed", m_remoteAddress);
    } else {
      Logger::warn("WsSession", "[{}] read error: {}", m_remoteAddress, ec.message());
    }
  } else {
    const std::string message = boost::beast::buffers_to_string(m_readBuffer.data());
    m_readBuffer.clear();
    m_isAuthorized = authorize(message);
    Logger::debug("WsSession", "[{}] message: {}", m_remoteAddress, message);
    read();
  }
}

void WebSocketServerSession::write() {
  if (m_isReady && m_isAlive && !m_messages.empty() && !m_isWriting.exchange(true)) {
    Logger::debug("WsSession", "[{}] queue size: {}", m_remoteAddress, m_messages.size());
    const auto message = m_messages.front();
    m_messages.pop();
    m_ws.text(true);
    boost::beast::ostream(m_writeBuffer) << message;
    m_ws.async_write(m_writeBuffer.data(), boost::beast::bind_front_handler(&WebSocketServerSession::onWrite, this));
  }
}

void WebSocketServerSession::onWrite(boost::beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    m_isAlive = false;
    if (ec == boost::beast::websocket::error::closed) {
      Logger::info("WsSession", "[{}] connection closed", m_remoteAddress);
    } else {
      Logger::warn("WsSession", "[{}] write error: {}", m_remoteAddress, ec.message());
    }
  } else {
    m_writeBuffer.clear();
    m_isWriting = false;
    write();
  }
}
