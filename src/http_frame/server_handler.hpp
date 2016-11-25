#ifndef _SERVER_HANDLER_HPP
#define _SERVER_HANDLER_HPP
#include "http_request.hpp"

class ServerHandler
{
public:
  ServerHandler() {};
  virtual ~ServerHandler() {};

  virtual bool Process(
    const HttpRequest& request,
    std::string& reply,
    std::string& extension) = 0;
};

#endif //_SERVER_HANDLER_HPP
