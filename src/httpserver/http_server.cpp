#include <sstream>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "http_server.hpp"
#include "http_request.hpp"
#include "http_request_parser.hpp"
#include "http_reply.hpp"
#include "mime_type.hpp"
#include "../looka_log.hpp"

HttpServer::HttpServer(unsigned short _listen_port, size_t _threads_num)
{
  m_listen_fd = HTTP_INVALID_FD;
  m_epoll_fd = HTTP_INVALID_FD;
  m_listen_port = _listen_port;
  m_threads_num = _threads_num;
  m_threads = NULL;

  m_running = false;
  m_epoll_ready_event_num = 0;

  pthread_mutex_init(&m_epoll_mutex, NULL);
  m_handler = NULL;

  m_server_name = "exhttpd";
}

HttpServer::~HttpServer()
{
  Stop();
  if (m_threads_num > 0) {
    for (int i = 0; i < m_threads_num; i++) {
      pthread_join(m_threads[i], NULL);
    }
    delete [] m_threads;
  }

  pthread_mutex_destroy(&m_epoll_mutex);
}

int HttpServer::SetServerHandler(ServerHandler* h)
{
  m_handler = h;
  return 0;
}

int HttpServer::SetServerName(const std::string& server_name)
{
  m_server_name = m_server_name + "/" + server_name;
  return 0;
}

int HttpServer::Start()
{
  if (CreateEpoll(m_epoll_fd, HTTP_MAX_FD))
    return -1;
  if (CreateListen(m_listen_fd, HTTP_MAX_FD))
    return -1;
  if (AddEvent(m_listen_fd))
    return -1;

  m_running = true;

  m_threads = new pthread_t[m_threads_num];
  for (int i = 0; i < m_threads_num; i++)
    if (pthread_create(&m_threads[i], NULL, Routine, this))
      return -1;

  return 0;
}

int HttpServer::Stop()
{
  m_running = false;
  return 0;
}

void* HttpServer::Routine(void* arg)
{
  (static_cast<HttpServer*>(arg))->Run();
  return NULL;
}

int HttpServer::Run()
{
  while (m_running) {
    pthread_mutex_lock(&m_epoll_mutex);
    if (m_epoll_ready_event_num <= 0) {
      m_epoll_ready_event_num =
        epoll_wait(m_epoll_fd, m_epoll_ready_events, HTTP_MAX_FD, -1);
    }
    if (m_epoll_ready_event_num-- < 0) {
      pthread_mutex_unlock(&m_epoll_mutex);
      continue;
    }

    int new_fd = HTTP_INVALID_FD;
    int fd = m_epoll_ready_events[m_epoll_ready_event_num].data.fd;
    if (fd == m_listen_fd) {
      struct sockaddr_in addr;
      socklen_t addr_len;
      while ((new_fd = accept(fd, (struct sockaddr*)&addr, &addr_len)) >= 0) {
        SetSocket(new_fd);
        AddEvent(new_fd);
      }
      // accept
      pthread_mutex_unlock(&m_epoll_mutex);
      continue;
    }
    DelEvent(fd);
    pthread_mutex_unlock(&m_epoll_mutex);

    int ret;
    HttpReply reply;
    if ((ret = HandleRequest(fd, reply)) != 0) {
      _INFO("[bad request r %d]", ret);
    }
    if ((ret = SendReply(fd, reply)) != 0) {
      _INFO("[send reply fail]");
    }
    close(fd);
  }
  return 0;
}

int HttpServer::HandleRequest(int fd, HttpReply& reply)
{
  Buffer       buffer;
  HttpRequest  request;
  timeval      timeout = {0, HTTP_READ_TIMEOUT * 1000};

  if (ReadHttpHeaderTimeout(fd, buffer, &timeout) < 0) {
    reply = HttpReply::DirectReply(HttpReply::STATUS_BAD_REQUEST);
    return -1;
  }

  if (HttpRequestParser::ParseHeader(request, buffer.read_buf[0]) != 0) {
    reply = HttpReply::DirectReply(HttpReply::STATUS_BAD_REQUEST);
    return -2;
  }

  std::string length_str = request.GetHeaderValue("Content-Length");
  if (!length_str.empty()) {
    int length = atoi(length_str.c_str());
    if (ReadHttpContentTimeout(fd, buffer, length, &timeout) != length) {
      reply = HttpReply::DirectReply(HttpReply::STATUS_BAD_REQUEST);
      return -3;
    }
    if (HttpRequestParser::ParseContent(request, buffer.read_buf[0]) != 0) {
      reply = HttpReply::DirectReply(HttpReply::STATUS_BAD_REQUEST);
      return -5;
    }
  }

  std::string handle_reply_str;
  std::string extension;

  if (m_handler && m_handler->Process(request, handle_reply_str, extension)) {
    reply.status = HttpReply::STATUS_OK;
    reply.content = handle_reply_str;
    std::stringstream ss_content_length;
    ss_content_length << reply.content.length();
    reply.AddHeader("Content-Length", ss_content_length.str());
    reply.AddHeader("Content-Type", MimeType::ExtensionToType(extension));
  } else {
    reply = HttpReply::DirectReply(HttpReply::STATUS_NOT_FOUND);
    return -4;
  }

  return 0;
}

int HttpServer::SendReply(int fd, HttpReply& reply)
{
  reply.AddHeader("Date", GetServerTime());
  reply.AddHeader("Server", m_server_name);

  timeval timeout = {0, HTTP_READ_TIMEOUT * 1000};
  std::string write_buf = reply.ToString();
  if (WriteTimeout(fd, write_buf.c_str(), write_buf.length(), &timeout) !=
      static_cast<int>(write_buf.length())) {
    return -1;
  }
  return 0;
}

int HttpServer::ReadHttpHeaderTimeout(
  int fd, Buffer& buffer, timeval* timeout)
{
  int n;
  int pos;
  char buf[HTTP_READ_BUF_SIZE + 1];
  std::string& ahead  = buffer.read_buf[0];
  std::string& behind = buffer.read_buf[1];
  while (true) {
    if ((pos = ahead.find("\r\n\r\n")) >= 0) {
      behind = ahead.substr(pos + 4);
      ahead  = ahead.substr(0, pos + 4);
      return 0;
    }
    if ((n = ReadOnceTimeout(fd, buf, HTTP_READ_BUF_SIZE, timeout)) <= 0)
      return -1;
    buf[n] = '\0';
    ahead += buf;
  }
  return -1;
}

int HttpServer::ReadHttpContentTimeout(
  int fd, Buffer& buffer, int len, timeval* timeout)
{
  int n;
  int read_len;
  char buf[HTTP_READ_BUF_SIZE + 1];
  std::string& ahead  = buffer.read_buf[0];
  std::string& behind = buffer.read_buf[1];

  int left = len - behind.length();
  if (left < 0) {
    ahead  = behind.substr(0, len);
    behind = behind.substr(len);
    return len;
  }

  ahead  = behind;
  behind = "";
  while (left > 0) {
    read_len = left > HTTP_READ_BUF_SIZE ? HTTP_READ_BUF_SIZE : left;
    if ((n = ReadOnceTimeout(fd, buf, read_len, timeout)) <= 0) {
      return len - left;
    }
    buf[n] = '\0';
    ahead += buf;
    left -= n;
  }
  return len;
}

int HttpServer::AddEvent(int fd)
{
  epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = fd;
  epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, fd, &event);
  return 0;
}

int HttpServer::DelEvent(int fd)
{
  epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = fd;
  epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, &event);
  return 0;
}

int HttpServer::SetSocket(int fd)
{
  int options;
  options = SOCK_SEND_BUF_SIZE;
  setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &options, sizeof(int));
  options = SOCK_RECV_BUF_SIZE;
  setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &options, sizeof(int));
  options = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &options, sizeof(int));
  options = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &options, sizeof(int));
  options = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, options | O_NONBLOCK);
  return 0;
}

int HttpServer::CreateEpoll(int& fd, int max_fd)
{
  if ((fd = epoll_create(max_fd)) == -1)
    return -1;
  return 0;
}

int HttpServer::CreateListen(int& fd, int max_fd)
{
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(m_listen_port);

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    return -1;
  if (SetSocket(fd))
    return -1;
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)))
    return -1;
  if (listen(fd, max_fd))
    return -1;

  _INFO("[HttpServer] [Listening %s:%hu]",
    inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
  return 0;
}

int HttpServer::ReadOnceTimeout(
  int fd, void* buf, size_t buf_len, timeval* timeout)
{
  fd_set rset;
  FD_ZERO(&rset);
  FD_SET(fd, &rset);
  if (select(fd + 1, &rset, NULL, NULL, timeout) <= 0)
    return -1;
  return read(fd, buf, buf_len);
}

int HttpServer::WriteTimeout(
  int fd, const void *buf, size_t buf_len, timeval *timeout)
{
  int left = buf_len;
  int n;
  fd_set wset;

  while (left > 0)
  {
    FD_ZERO(&wset);
    FD_SET(fd, &wset);
    if (select(fd + 1, NULL, &wset, NULL, timeout) <= 0)
      return -1;
    if ((n = write(fd, buf, left)) <= 0)
      return buf_len - left;
    buf = (char *)buf + n;
    left -= n;
  }
  return buf_len;
}

std::string HttpServer::GetServerTime()
{
  time_t t;
  struct tm tmp_time;
  time(&t);
  char timestr[64] = {0};
  strftime(timestr, sizeof(timestr),
    "%a, %e %b %Y %H:%M:%S %Z", localtime_r(&t, &tmp_time));
  return std::string(timestr);
}
