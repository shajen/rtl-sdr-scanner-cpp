#include "websocket_server.h"

WebSocketServer::WebSocketServer(const std::string& address, const int port, const std::string& key, const int threads)
    : m_ioc(threads), m_endpoint(boost::asio::ip::make_address(address), static_cast<unsigned short>(port)), m_listener(m_ioc, m_endpoint, key), m_isRunning(true) {
  for (int i = 0; i < threads; ++i) {
    std::thread thread([this]() {
      while (m_isRunning) {
        m_ioc.run_for(std::chrono::milliseconds(100));
      }
    });
    m_threads.push_back(std::move(thread));
  }
}

WebSocketServer::~WebSocketServer() {
  m_isRunning = false;
  for (auto& thread : m_threads) {
    thread.join();
  }
}

void WebSocketServer::send(const std::string& message) { m_listener.send(message); }
