#include <string>
#include "../tcp_server.hpp"
#ifdef PROTOBUF
#include "../../../server_frame/server_request.hpp"
#endif

class ServerHandler
{
public:
  ServerHandler() {}
  ~ServerHandler() {
    if (server)
      delete server;
  }

  int start()
  {
    server = new tcp_server<ServerHandler, ServerHandler>();
    server->net(6610);
    //server->register_send_handler(this, &ServerHandler::notify_callback);
    server->register_send_handler(this, NULL/* &ServerHandler::notify_callback */);
    server->register_recv_handler(this, &ServerHandler::handle_data);
    server->start();
  }

  int handle_data(int fd, const unsigned char* buf, int size, const sockaddr_in& addr)
  {
    std::string query = std::string((const char*)buf);
#ifdef PROTOBUF
    server_request request;
    request.ParseFromString(std::string((const char*)buf, size));
    query = request.request_str();
#endif
    _INFO("[ServerHandler] [IP:%s] [sock %d] [Query] [%s]", inet_ntoa(addr.sin_addr), fd, query.c_str());


    char newbuf[1024];
    snprintf(newbuf, sizeof(newbuf), "===> %s <===", query.c_str());
    server->send(fd, (unsigned char*)newbuf, strlen(newbuf) + 1);
    return 0;
  }

  int notify_callback(int fd, int ret)
  {
    if (ret == 0)
      _INFO("[ServerHandler] [send back to sock(%d) success]", fd);
    else
      _INFO("[ServerHandler] [send back to sock(%d) failed]", fd);
    return 0;
  }

private:
  tcp_server<ServerHandler, ServerHandler>* server;
};

int main(int argc, char** argv)
{
  ServerHandler* handler = new ServerHandler();
  handler->start();
  while (true)
    sleep(1);
  return 0;
}
