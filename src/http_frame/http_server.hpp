#ifndef _HTTP_SERVER_HPP
#define _HTTP_SERVER_HPP
#include <sys/epoll.h>
#include <pthread.h>
#include <string>
#include "server_handler.hpp"

#define HTTP_INVALID_FD         -1
#define HTTP_MAX_FD             1024
#define HTTP_READ_BUF_SIZE      1024
#define HTTP_READ_TIMEOUT       50
#define HTTP_WRITE_TIMEOUT      50
#define HTTP_HEADER_MAX_LENGTH  1024

#define SOCK_SEND_BUF_SIZE      (20 * 1024 * 1024)
#define SOCK_RECV_BUF_SIZE      (20 * 1024 * 1024)
 

class HttpReply;

class HttpServer
{
public:
  HttpServer(unsigned short _listen_port, size_t _threads_num);
  virtual ~HttpServer();

  int Start();
  int Stop();
  int SetServerHandler(ServerHandler* h);
  int SetServerName(const std::string& server_name);

private:
  struct Buffer{
    std::string read_buf[2];
  };

private:
  static void* Routine(void* arg);
  int Run();

  int AddEvent(int fd);
  int DelEvent(int fd);
  int SetSocket(int fd);
  int CreateEpoll(int& fd, int max_fd);
  int CreateListen(int& fd, int max_fd);
  int HandleRequest(int fd, HttpReply& reply);
  int SendReply(int fd, HttpReply& reply);

  int ReadHttpHeaderTimeout(int fd, Buffer& buffer, timeval* timeout);
  int ReadHttpContentTimeout(int fd, Buffer& buffer, int len, timeval* timeout);

  int ReadOnceTimeout(int fd, void* buf, size_t buf_len, timeval* timeout);
  int WriteTimeout(int fd, const void *buf, size_t buf_len, timeval *timeout);

  std::string GetServerTime();

private:
  int m_listen_fd;
  int m_epoll_fd;
  unsigned short m_listen_port;

  int m_epoll_ready_event_num;
  struct epoll_event m_epoll_ready_events[HTTP_MAX_FD];

  pthread_t* m_threads;
  int m_threads_num;
  pthread_mutex_t m_epoll_mutex;

  bool m_running;
  ServerHandler* m_handler;
  std::string m_server_name;
};

#endif //_HTTP_SERVER_HPP
