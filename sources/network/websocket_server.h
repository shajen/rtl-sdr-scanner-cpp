#pragma once

#include <network/websocket_server_listener.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <string>
#include <thread>

class WebSocketServer {
 public:
  WebSocketServer(const std::string& address, const int port, const std::string& key, const int threads);
  ~WebSocketServer();

  void send(const std::string& message);

 private:
  boost::asio::io_context m_ioc;
  boost::asio::ip::tcp::endpoint m_endpoint;
  WebSocketServerListener m_listener;
  bool m_isRunning;
  std::vector<std::thread> m_threads;
};
