#pragma once

#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/tcp_stream.hpp>
#include <boost/beast/websocket.hpp>
#include <queue>

class WebSocketServerSession {
 public:
  WebSocketServerSession(const std::string& key, const std::string& remoteAddress, boost::asio::ip::tcp::socket&& socket);
  ~WebSocketServerSession();

  void send(const std::string& message);
  bool isAlive() const;

 private:
  bool authorize(const std::string& message) const;
  void run();
  void onRun();
  void onAccept(boost::beast::error_code ec);
  void read();
  void onRead(boost::beast::error_code ec, std::size_t bytes_transferred);
  void write();
  void onWrite(boost::beast::error_code ec, std::size_t bytes_transferred);

  const std::string m_key;
  const std::string m_remoteAddress;
  bool m_isQueueFullWasReported;
  bool m_isReady;
  bool m_isAlive;
  bool m_isAuthorized;
  boost::beast::websocket::stream<boost::beast::tcp_stream> m_ws;
  boost::beast::flat_buffer m_readBuffer;
  boost::beast::flat_buffer m_writeBuffer;
  std::queue<std::string> m_messages;
  std::atomic_bool m_isWriting;
};
