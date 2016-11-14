//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "server.hpp"
#include <sstream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>

namespace http {
namespace server3 {

server::server(
  const int& port,
  std::size_t thread_pool_size)
  : thread_pool_size_(thread_pool_size),
    acceptor_(io_service_),
    new_connection_(new connection(io_service_, request_handler_))
{
  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  std::stringstream ss;
  ss << port;
  std::string port_s = ss.str();
  boost::asio::ip::tcp::resolver resolver(io_service_);
  boost::asio::ip::tcp::resolver::query query(get_local_ip(), port_s);
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();
  acceptor_.async_accept(new_connection_->socket(),
      boost::bind(&server::handle_accept, this,
        boost::asio::placeholders::error));
}

void server::run()
{
  // Create a pool of threads to run all of the io_services.
  std::vector<boost::shared_ptr<boost::thread> > threads;
  for (std::size_t i = 0; i < thread_pool_size_; ++i)
  {
    boost::shared_ptr<boost::thread> thread(new boost::thread(
          boost::bind(&boost::asio::io_service::run, &io_service_)));
    threads.push_back(thread);
  }

  // Wait for all threads in the pool to exit.
  for (std::size_t i = 0; i < threads.size(); ++i)
    threads[i]->join();
}

void server::stop()
{
  io_service_.stop();
}

void server::set_server_handler(ServerHandler* handler)
{
  request_handler_.set_server_handler(handler);
}

void server::handle_accept(const boost::system::error_code& e)
{
  if (!e)
  {
    new_connection_->start();
    new_connection_.reset(new connection(io_service_, request_handler_));
    acceptor_.async_accept(new_connection_->socket(),
        boost::bind(&server::handle_accept, this,
          boost::asio::placeholders::error));
  }
}

std::string server::get_local_ip()
{
  long ip = -1;
  int  fd = 0;
  int MAX_INTERFACES = 16;
  struct ifreq buf[MAX_INTERFACES];
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)
  {
    struct ifconf ifc;
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t)buf;
    if (!ioctl(fd, SIOCGIFCONF, (char*)&ifc))
    {
      int intrface = ifc.ifc_len / sizeof (struct ifreq);
      while (intrface-- > 0)
      {
        if (!(ioctl(fd, SIOCGIFADDR, (char*)&buf[intrface])))
        {
          ip = inet_addr(inet_ntoa(((struct sockaddr_in*)(&buf[intrface].ifr_addr))->sin_addr));
          break;
        }
      }
    }
    close(fd);
  }

  union ipdumper {
    long ip; 
    uint8_t slot[4];
  } dumper;
  dumper.ip = ip;

  std::string addr("");
  if (dumper.ip != -1) {
    char ipbuf[16];
    snprintf(ipbuf, sizeof(ipbuf), "%u.%u.%u.%u",
      dumper.slot[0],
      dumper.slot[1],
      dumper.slot[2],
      dumper.slot[3]);
    addr = std::string(ipbuf);
  }
  return addr;
}

} // namespace server3
} // namespace http
