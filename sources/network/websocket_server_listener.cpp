#include "websocket_server_listener.h"

#include <logger.h>

WebSocketServerListener::WebSocketServerListener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint& endpoint) : m_ioc(ioc), m_endpoint(endpoint), m_acceptor(m_ioc) {
  boost::beast::error_code ec;

  m_acceptor.open(endpoint.protocol(), ec);
  if (ec) {
    Logger::warn("WsListener", "error: %s", ec.message().c_str());
    throw std::runtime_error(ec.message());
  }

  m_acceptor.set_option(boost::asio::socket_base::reuse_address(true), ec);
  if (ec) {
    Logger::warn("WsListener", "error: %s", ec.message().c_str());
    throw std::runtime_error(ec.message());
  }

  m_acceptor.bind(endpoint, ec);
  if (ec) {
    Logger::warn("WsListener", "error: {}", ec.message());
    throw std::runtime_error(ec.message());
  }

  m_acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    Logger::warn("WsListener", "error: {}", ec.message());
    throw std::runtime_error(ec.message());
  }

  Logger::info("WsListener", "start listening on {}:{}", endpoint.address().to_string(), endpoint.port());
  accept();
}

WebSocketServerListener::~WebSocketServerListener() {
  Logger::info("WsListener", "stop listening on {}:{}", m_endpoint.address().to_string(), m_endpoint.port());
  m_acceptor.cancel();
}

void WebSocketServerListener::send(const std::string& message) {
  const auto it = std::remove_if(m_sessions.begin(), m_sessions.end(), [](std::unique_ptr<WebSocketServerSession>& session) { return !session->isAlive(); });
  if (it != m_sessions.end()) {
    m_sessions.erase(it, m_sessions.end());
    Logger::info("WsListener", "sessions: {}", m_sessions.size());
  }
  for (auto& session : m_sessions) {
    session->send(message);
  }
}

void WebSocketServerListener::accept() { m_acceptor.async_accept(boost::asio::make_strand(m_ioc), boost::beast::bind_front_handler(&WebSocketServerListener::onAccept, this)); }

void WebSocketServerListener::onAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
  const auto address = socket.remote_endpoint().address().to_string();
  const auto port = socket.remote_endpoint().port();
  const std::string remoteAddress(address + ":" + std::to_string(port));
  if (ec) {
    Logger::warn("WsListener", "[{}] connection error: {}", remoteAddress, ec.message());
  } else {
    Logger::info("WsListener", "[{}] connection success", remoteAddress);
    m_sessions.push_back(std::make_unique<WebSocketServerSession>(remoteAddress, std::move(socket)));
    Logger::info("WsListener", "sessions: {}", m_sessions.size());
  }
  accept();
}
