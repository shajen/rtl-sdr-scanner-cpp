#include "websocket_server_session.h"

#include <logger.h>

WebSocketServerSession::WebSocketServerSession(const std::string& remoteAddress, boost::asio::ip::tcp::socket&& socket)
    : m_remoteAddress(remoteAddress), m_isReady(false), m_isAlive(true), m_ws(std::move(socket)), m_isWriting(false) {
  run();
}

WebSocketServerSession::~WebSocketServerSession() { Logger::info("WsSession", "[{}] connection removed", m_remoteAddress); }

void WebSocketServerSession::send(const std::string& message) {
  m_messages.push(message);
  write();
}

bool WebSocketServerSession::isAlive() const { return m_isAlive; }

void WebSocketServerSession::run() { boost::asio::dispatch(m_ws.get_executor(), boost::beast::bind_front_handler(&WebSocketServerSession::onRun, this)); }

void WebSocketServerSession::onRun() {
  m_ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::server));
  m_ws.set_option(boost::beast::websocket::stream_base::decorator(
      [](boost::beast::websocket::response_type& res) { res.set(boost::beast::http::field::server, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async"); }));
  m_ws.async_accept(boost::beast::bind_front_handler(&WebSocketServerSession::onAccept, this));
}

void WebSocketServerSession::onAccept(boost::beast::error_code ec) {
  if (ec) {
    Logger::warn("WsSession", "[{}] accept error: {}", m_remoteAddress, ec.message());
  } else {
    m_isReady = true;
    read();
  }
}

void WebSocketServerSession::read() { m_ws.async_read(m_readBuffer, boost::beast::bind_front_handler(&WebSocketServerSession::onRead, this)); }

void WebSocketServerSession::onRead(boost::beast::error_code ec, std::size_t bytes_transferred) {
  if (ec == boost::beast::websocket::error::closed) {
    m_isAlive = false;
    Logger::info("WsSession", "[{}] connection closed", m_remoteAddress);
    return;
  } else if (ec) {
    m_isAlive = false;
    Logger::warn("WsSession", "[{}] read error: {}", m_remoteAddress, ec.message());
    return;
  } else {
    const std::string message = boost::beast::buffers_to_string(m_readBuffer.data());
    m_readBuffer.clear();
    Logger::info("WsSession", "[{}] message: {}", m_remoteAddress, message);
  }
  read();
}

void WebSocketServerSession::write() {
  if (m_isReady && m_isAlive && !m_messages.empty() && !m_isWriting.exchange(true)) {
    Logger::debug("WsSession", "queue size: {}", m_messages.size());
    const auto message = m_messages.front();
    m_messages.pop();
    m_ws.text(true);
    boost::beast::ostream(m_writeBuffer) << message;
    m_ws.async_write(m_writeBuffer.data(), boost::beast::bind_front_handler(&WebSocketServerSession::onWrite, this));
  }
}

void WebSocketServerSession::onWrite(boost::beast::error_code ec, std::size_t bytes_transferred) {
  if (ec) {
    Logger::warn("WsSession", "[{}] write error: {}", m_remoteAddress, ec.message());
  }
  m_writeBuffer.clear();
  m_isWriting = false;
  write();
}
