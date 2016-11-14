#ifndef _SERVER_HANDLER_HPP
#define _SERVER_HANDLER_HPP

class ServerHandler
{
public:
  ServerHandler() {};
  virtual ~ServerHandler() {};

  virtual bool Process(const std::string& request, std::string& reply, std::string& extension) = 0;
};

#endif //_SERVER_HANDLER_HPP
