#include "../tcp_client.hpp"
#include <vector>
#ifdef PROTOBUF
#include "../../../server_frame/server_request.hpp"
#endif

class ClientHandler {
public:
  ClientHandler() {}
  virtual ~ClientHandler() {
    if (client)
      delete client;
  }

  void init_addrs() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //addr.sin_port = htons(33521);
    addr.sin_port = htons(6610);
    addrs.push_back(addr);
  }

  int start() {
    init_addrs();
    client = new tcp_client<ClientHandler, ClientHandler>();
    client->net(&addrs[0], addrs.size(), 8);
    client->register_send_handler(this, &ClientHandler::notify_callback);
    //client->register_send_handler(this, NULL/*&ClientHandler::notify_callback*/);
    client->register_recv_handler(this, &ClientHandler::handle_data);

    client->start();

    sleep(1);

    char* data;
    size_t len;
    char  buf[1024];
    unsigned int i = 0;
    while (i < 100000) {
      memset(buf, 0, sizeof(buf));
      sprintf(buf, "Hello! This is test_client %u.", i + 1);

      data = buf;
      len = (size_t)strlen(buf) + 1;
#ifdef PROTOBUF
      std::string serialized_str;
      server_request request;

      request.set_request_id(i + 1);
      request.set_request_str(std::string(buf));
      if (request.SerializeToString(&serialized_str)) {
        data = (char*)&serialized_str[0];
        len = (size_t)serialized_str.length();
      }
#endif
      client->send(0, (const unsigned char*)data, len, 3);
      usleep(0.5 * 1000);
      i++;
    }

    while (true)
      sleep(5);

    return 0;
  }

  int notify_callback(int fd, int server_id, int ret) {
    /*
    if (ret == 0) {
      _INFO("[ClientHandler::notify_callback] [send to server(%d) success] [sock %d]", server_id, fd);
    } else {
      _ERROR("[ClientHandler::notify_callback] [send to server(%d) failed] [sock %d]", server_id, fd);
    }
    */
    if (ret == -1)
      _ERROR_RETURN(-1, "[ClientHandler::notify_callback] [send to server(%d) failed] [sock %d]", server_id, fd);
    return 0;
  }

  int handle_data(const unsigned char* buf, int size, const struct sockaddr_in& addr) {
    uint64_t request_id = 0;
    const char* ptr = (const char*)buf;
#ifdef PROTOBUF
    server_request request;
    request.ParseFromString(std::string((const char*)buf, size));
    ptr = request.request_str().c_str();
    request_id = request.request_id();
#endif
    _INFO("[ClientHander::handle_data] [receive ack from %s:%hu] [RequestId %lu] [%s]",
      inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), request_id, ptr);
    return 0;
  }

private:
  tcp_client<ClientHandler, ClientHandler>* client;
  std::vector<struct sockaddr_in> addrs;
};

int main(int argc, char** argv)
{
  ClientHandler* client = new ClientHandler();
  client->start();

  sleep(1);
  return 0;
}
