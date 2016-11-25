#ifndef _TCP_SERVER_HPP
#define _TCP_SERVER_HPP

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <map>
#include <vector>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "tcp_comm.hpp"
#include "linked_list.hpp"
#include "wait_list.hpp"
#include "../looka_log.hpp"

template <typename SendHandler, typename RecvHandler>
class tcp_server
{
public:
  typedef int (SendHandler::*SendCallback)(int /*fd*/, int /*ret*/);
  typedef int (RecvHandler::*RecvCallback)(int /*fd*/, const unsigned char* /*buf*/, int /*size*/, const struct sockaddr_in& /*addr*/);

  tcp_server();
  ~tcp_server();

  int start();
  int net(unsigned short listen_port);
  int register_send_handler(SendHandler* h, SendCallback c);
  int register_recv_handler(RecvHandler* h, RecvCallback c);
  int stop();

  int send(int fd, const unsigned char* buf, size_t size, int retry_times = 3);

private:
  static void* receiver_routine(void* arg);
  static void* sender_routine(void* arg);

  int sender();
  int receiver();

  int set_socket(int fd);
  int create_epoll(int &fd, int max_fd);
  int create_listen(int &fd, int max_fd, unsigned short port);
  int recv_request(int fd, void* buf, size_t buf_len);
  int readn_timeout(int fd, void* buf, size_t buf_len, timeval* timeout);
  int add_input_fd(int fd);
  int del_input_fd(int fd);
  int add_input_fd_record(int fd);
  int update_record();

  int add_sockaddr(int fd, const struct sockaddr_in& addr);
  int get_sockaddr(int fd, struct sockaddr_in& addr);
  int del_sockaddr(int fd);

private:
  struct record_t {
    int fd;
    int op;
    struct epoll_event event;
  };

  struct send_node {
    linked_list_node_t list_node;
    int fd;
    int retry_times;
    int offset;

    union {
      unsigned char data[tcp_pkg_max_size];
      tcp_package package;
    };
  };

private:
  wait_list_t<send_node, &send_node::list_node> send_list;
  std::vector<struct send_node*> send_free_list;

  SendHandler* send_handler;
  SendCallback send_callback;
  RecvHandler* recv_handler;
  RecvCallback recv_callback;

  int listen_fd;
  int epoll_fd;
  unsigned short listen_port;
  pthread_mutex_t listener_mutex;

  pthread_t* sender_threads;
  int sender_threads_num;

  pthread_t* receiver_threads;
  int receiver_threads_num;

  int epoll_ready_event_num;
  struct epoll_event epoll_ready_events[tcp_max_fd];

  std::vector<struct record_t> update_records;
  pthread_mutex_t update_record_mutex;

  std::map<int, struct sockaddr_in> sock_addrs;
  pthread_mutex_t sock_addr_mutex;

  int send_free_num;
  pthread_mutex_t send_free_list_mutex;

  bool svc_activate;
};

template <typename SendHandler, typename RecvHandler>
tcp_server<SendHandler, RecvHandler>::tcp_server()
{
  send_handler  = NULL;
  send_callback = NULL;
  recv_handler  = NULL;
  recv_callback = NULL;

  listen_port = -1;
  listen_fd = -1;
  epoll_fd = -1;

  receiver_threads = NULL;
  receiver_threads_num = 0;

  sender_threads = NULL;
  sender_threads_num = 0;

  epoll_ready_event_num = 0;
  svc_activate = false;

  pthread_mutex_init(&listener_mutex, NULL);
  pthread_mutex_init(&update_record_mutex, NULL);
  pthread_mutex_init(&sock_addr_mutex, NULL);
  pthread_mutex_init(&send_free_list_mutex, NULL);
}

template <typename SendHandler, typename RecvHandler>
tcp_server<SendHandler, RecvHandler>::~tcp_server()
{
  stop();
  if (receiver_threads_num > 0)
  {
    for (int i=0; i<receiver_threads_num; i++)
      pthread_join(receiver_threads[i], NULL);
    delete []receiver_threads;
  }
  if (sender_threads_num > 0)
  {
    for (int i=0; i<sender_threads_num; i++)
      pthread_join(sender_threads[i], NULL);
    delete []sender_threads;
  }

  pthread_mutex_destroy(&listener_mutex);
  pthread_mutex_destroy(&update_record_mutex);
  pthread_mutex_destroy(&sock_addr_mutex);
  pthread_mutex_destroy(&send_free_list_mutex);
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::set_socket(int fd)
{
  int options;
  options = tcp_sbuf_size;
  setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &options, sizeof(int));
  options = tcp_rbuf_size;
  setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &options, sizeof(int));
  options = 1;
  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &options, sizeof(int));
  options = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &options, sizeof(int));
  options = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, options | O_NONBLOCK);
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::create_epoll(int &fd, int max_fd)
{
  if ((fd = epoll_create(max_fd)) == -1)
    return -1;
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::create_listen(int &fd, int max_fd, unsigned short port)
{
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    return -1;
  if (set_socket(fd))
    return -1;
  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)))
    return -1;
  if (listen(fd, max_fd))
    return -1;

  _INFO("[tcp_server] [listen at %s:%hu]", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::add_input_fd(int fd)
{
  epoll_event event;
  event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
  event.data.fd = fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::del_input_fd(int fd)
{
  epoll_event event;
  event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
  event.data.fd = fd;
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, &event);
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::add_input_fd_record(int fd)
{
  struct record_t r;
  r.fd = fd;
  r.op = EPOLL_CTL_ADD;
  r.event.events = EPOLLIN | EPOLLET |EPOLLRDHUP;
  r.event.data.fd = fd;

  pthread_mutex_lock(&update_record_mutex);
  update_records.push_back(r);
  pthread_mutex_unlock(&update_record_mutex);

  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::update_record()
{
  pthread_mutex_lock(&update_record_mutex);
  for (size_t i=0; i<update_records.size(); i++)
    epoll_ctl(epoll_fd, update_records[i].op, update_records[i].fd, &update_records[i].event);
  update_records.clear();
  std::vector<struct record_t>().swap(update_records);
  pthread_mutex_unlock(&update_record_mutex);
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::add_sockaddr(int fd, const struct sockaddr_in& addr)
{
  pthread_mutex_lock(&sock_addr_mutex);
  sock_addrs[fd] = addr;
  pthread_mutex_unlock(&sock_addr_mutex);
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::get_sockaddr(int fd, struct sockaddr_in& addr)
{
  int ret = 0;
  pthread_mutex_lock(&sock_addr_mutex);
  if (sock_addrs.find(fd) == sock_addrs.end())
    ret = -1;
  else
    addr = sock_addrs[fd];
  pthread_mutex_unlock(&sock_addr_mutex);
  return ret;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::del_sockaddr(int fd)
{
  pthread_mutex_lock(&sock_addr_mutex);
  std::map<int, struct sockaddr_in>::iterator it = sock_addrs.find(fd);
  if (it != sock_addrs.end())
    sock_addrs.erase(it);
  pthread_mutex_unlock(&sock_addr_mutex);
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::net(unsigned short port)
{
  listen_port = port;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::register_send_handler(SendHandler* h, SendCallback c)
{
  send_handler = h;
  send_callback = c;
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::register_recv_handler(RecvHandler* h, RecvCallback c)
{
  recv_handler = h;
  recv_callback = c;
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::start()
{
  if (create_epoll(epoll_fd, tcp_max_fd))
    return -1;
  if (create_listen(listen_fd, tcp_max_fd, listen_port))
    return -1;
  if (add_input_fd(listen_fd))
    return -1;

  svc_activate = true;

  for (send_free_num=0; send_free_num<tcp_queue_size; send_free_num++) {
    send_free_list.push_back(new send_node);
  }

  receiver_threads_num = 10;
  receiver_threads = new pthread_t[receiver_threads_num];
  for (int i=0; i<receiver_threads_num; i++)
    if (pthread_create(&receiver_threads[i], NULL, receiver_routine, this)) return -1;

  sender_threads_num = 10;
  sender_threads = new pthread_t[sender_threads_num];
  for (int i=0; i<sender_threads_num; i++)
    if (pthread_create(&sender_threads[i], NULL, sender_routine, this)) return -1;

  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::stop()
{
  svc_activate = false;
}

template <typename SendHandler, typename RecvHandler>
void* tcp_server<SendHandler, RecvHandler>::receiver_routine(void *arg)
{
  ((tcp_server<SendHandler, RecvHandler>*)arg)->receiver();
  return NULL;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::receiver()
{
  unsigned char buf[tcp_pkg_max_size];
  while (svc_activate)
  {
    pthread_mutex_lock(&listener_mutex);
    update_record();

    if (epoll_ready_event_num <= 0)
      epoll_ready_event_num = epoll_wait(epoll_fd, epoll_ready_events, tcp_max_fd, -1);

    if (epoll_ready_event_num-- < 0)
    {
      pthread_mutex_unlock(&listener_mutex);
      continue;
    }

    bool is_continue = false;
    int new_fd;
    int fd = epoll_ready_events[epoll_ready_event_num].data.fd;
    if (fd == listen_fd)
    {
      is_continue = true;
      struct sockaddr_in addr;
      socklen_t addr_len;
      while ((new_fd = accept(fd, (struct sockaddr*)&addr, &addr_len)) >= 0)
      {
        set_socket(new_fd);
        add_input_fd(new_fd);
        add_sockaddr(new_fd, addr);

        _INFO("[tcp_server] [new connection on socket(%d) ip(%s)]", fd, inet_ntoa(addr.sin_addr));
      }
    }
    else if (epoll_ready_events[epoll_ready_event_num].events & EPOLLRDHUP)
    {
      is_continue = true;
      close(fd);
      del_sockaddr(fd);
      _ERROR("[tcp_server] [disconnect error on socket(%d)]", fd);
    }

    if (is_continue)
    {
      pthread_mutex_unlock(&listener_mutex);
      continue;
    }

    del_input_fd(fd);
    pthread_mutex_unlock(&listener_mutex);

    int len = recv_request(fd, buf, sizeof(buf));
    add_input_fd_record(fd);

    if (len > 0) {
      struct sockaddr_in addr;
      if (get_sockaddr(fd, addr) != 0) {
        _ERROR("[tcp_rceiver] [get_sockaddr error] [sock %d]", fd);
      } else {
        (recv_handler->*recv_callback)(fd, buf, len, addr);
      }
    }
  }

  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::recv_request(int fd, void* buf, size_t buf_len)
{
  timeval timeout = { 0, tcp_read_timeout * 1000 };
  size_t len;
  size_t head_len;
  size_t body_len = 0;
  tcp_package_head head;
  head_len = sizeof(head);

  if ((len = readn_timeout(fd, &head, head_len, &timeout)) == head_len)
  {
    body_len = head.size - head_len;
    if (body_len > buf_len)
      _ERROR_RETURN(-1, "[Read Overflow] [buff length %d] [read length %d]", (int)buf_len, (int)body_len);

    if ((len = readn_timeout(fd, buf, body_len, &timeout)) != body_len)
      _ERROR_RETURN(-1, "[Read Failed]");
  }
  return body_len;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::readn_timeout(int fd, void* buf, size_t buf_len, timeval* timeout)
{
  int left = buf_len;
  int n;
  fd_set rset;

  while (left > 0)
  {
    FD_ZERO(&rset);
    FD_SET(fd, &rset);
    if (select(fd + 1, &rset, NULL, NULL, timeout) <= 0)
      return -1;
    if ((n = read(fd, buf, left)) == 0)
      return buf_len - left;
    buf = (char *)buf + n;
    left -= n;
  }
  return  buf_len;
}

template <typename SendHandler, typename RecvHandler>
void* tcp_server<SendHandler, RecvHandler>::sender_routine(void *arg)
{
  ((tcp_server<SendHandler, RecvHandler>*)arg)->sender();
  return NULL;
}

template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::sender()
{
  int ret;
  int sock;
  int size;
  bool success = false;
  bool sock_close = false;

  struct send_node* node = NULL;
  while (svc_activate && (node = send_list.get()) != NULL)
  {
    if (node->retry_times-- < 0) {
      continue;
    }

    sock = node->fd;
    size = node->package.head.size - node->offset;
    struct sockaddr_in addr;
    if (get_sockaddr(sock, addr) == 0) {
      ret = ::send(sock, node->data + node->offset, size, 0);
      if (ret == size) {
        success = true;
      } else if (ret >= 0) {
        node->offset += ret;
      } else {
        if (!(errno == EAGAIN || errno == EWOULDBLOCK)) {
          sock_close = true;

          close(sock);
          node->fd = -1;
          node->offset = 0;
        }
      }
    }

    if (success) {
      pthread_mutex_lock(&send_free_list_mutex);
      send_free_list[send_free_num++] = node;
      pthread_mutex_unlock(&send_free_list_mutex);
      if (send_callback != NULL) {
        (send_handler->*send_callback)(sock, 0);
      }
    } else if (node->retry_times > 0 && !sock_close) {
      send_list.put(*node);
    } else {
      pthread_mutex_lock(&send_free_list_mutex);
      send_free_list[send_free_num++] = node;
      pthread_mutex_unlock(&send_free_list_mutex);
      if (send_callback != NULL) {
        (send_handler->*send_callback)(sock, -1);
      }
    }
  }
  return 0;
}


template <typename SendHandler, typename RecvHandler>
int tcp_server<SendHandler, RecvHandler>::send(int fd, const unsigned char* buf, size_t size, int retry_times)
{
  if (send_handler == NULL)
    return -1;

  struct send_node *node;

  pthread_mutex_lock(&send_free_list_mutex);
  int val = send_free_num;
  if (val <= 10)
  {
    _INFO("[tcp_client::send] [free buf %d]", val);
  }
  if (val <= 0)
  {
    _ERROR("[tcp_client::send] [no free buf]");
    pthread_mutex_unlock(&send_free_list_mutex);
    return -1;
  }
  node = send_free_list[--send_free_num];
  pthread_mutex_unlock(&send_free_list_mutex);


  node->fd = fd;
  node->retry_times = retry_times;
  node->offset = 0;
  node->package.head.size = size + sizeof(struct tcp_package_head);
  memcpy(node->package.data, buf, size);

  send_list.put(*node);
  return 0;
}

#endif //_TCP_SERVER_HPP
