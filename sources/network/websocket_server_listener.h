#pragma once

#include <network/websocket_server_session.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

class WebSocketServerListener {
 public:
  WebSocketServerListener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint& endpoint);
  ~WebSocketServerListener();

  void send(const std::string& message);

 private:
  void accept();
  void onAccept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

  boost::asio::io_context& m_ioc;
  boost::asio::ip::tcp::endpoint& m_endpoint;
  boost::asio::ip::tcp::acceptor m_acceptor;
  std::vector<std::unique_ptr<WebSocketServerSession>> m_sessions;
};
