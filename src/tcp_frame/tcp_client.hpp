#ifndef _TCP_CLIENT_HPP
#define _TCP_CLIENT_HPP

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <vector>
#include <map>
#include "tcp_comm.hpp"
#include "linked_list.hpp"
#include "wait_list.hpp"
#include "../looka_log.hpp"

template <typename SendHandler, typename RecvHandler>
class tcp_client
{
public:
  typedef int (SendHandler::*SendCallback)(int /*sock*/, int /*server_id*/, int /*ret*/);
  typedef int (RecvHandler::*RecvCallback)(const unsigned char* /*buf*/, int /*size*/, const struct sockaddr_in& /*addr*/);

  tcp_client();
  ~tcp_client();

  int start();
  int net(const struct sockaddr_in* addrs, int server_num_, int conn_num_ = 1);
  int register_send_handler(SendHandler* h, SendCallback c);
  int register_recv_handler(RecvHandler* h, RecvCallback c);
  //int stop();

  int send(int server_id, const unsigned char* buf, size_t size, int retry_times);

private:

  enum {
    TCP_CONNECTED = 0,
    TCP_DISCONNECT,
    TCP_INPROGRESS,
    TCP_INUSE,
  };

  struct conn_node {
    linked_list_node_t list_node;

    int sock_id;
    int fd;
    int flag;
    struct sockaddr_in addr;

    pthread_mutex_t lock;
  };

  struct send_node {
    linked_list_node_t list_node;

    int server_id;
    int conn_id;
    int retry_times;
    int offset;

    union {
      unsigned char data[tcp_pkg_max_size];
      tcp_package package;
    };
  };

  struct recv_node {
    linked_list_node_t list_node;
    int fd;
  };

private:
  static void* connector_routine(void* arg);
  static void* monitor_routine(void* arg);
  static void* sender_routine(void* arg);
  //static void* receiver_routine(void *arg);

  int connector();
  int monitor();
  int sender();
  //int receiver();

  int recv_request(int fd, void* buf, size_t buf_len);
  int readn_timeout(int fd, void* buf, size_t buf_len, timeval* timeout);

  int set_socket(int fd);
  int add_socknode(int fd, struct conn_node*& node);
  int get_socknode(int fd, struct conn_node*& node);
  int del_socknode(int fd);


private:

  wait_list_t<recv_node, &recv_node::list_node> recv_list;
  wait_list_t<send_node, &send_node::list_node> send_list;
  wait_list_t<conn_node, &conn_node::list_node> conn_list;
  std::vector<struct send_node*> send_free_list;
  std::vector<struct recv_node*> recv_free_list;

  int epoll_fd;

  struct sockaddr_in* servers;
  int server_num;
  int conn_num;
  int send_free_num;
  int recv_free_num;

  struct conn_node* sock_nodes;
  int sock_num;

  std::map<int, int> server_conn_ids;
  std::map<int, conn_node*> conn_table;
  pthread_mutex_t conn_table_mutex;
  pthread_mutex_t send_free_list_mutex;
  pthread_mutex_t recv_free_list_mutex;
  typedef typename std::map<int, conn_node*>::iterator conn_table_iterator_t;


  SendHandler* send_handler;
  SendCallback send_callback;
  RecvHandler* recv_handler;
  RecvCallback recv_callback;

  pthread_t* connector_threads;
  int connector_num;

  pthread_t* monitor_threads;
  int monitor_num;

  pthread_t* sender_threads;
  int sender_num;

  //pthread_t* receiver_threads;
  //int receiver_num;

  pthread_mutex_t monitor_mutex;

};

template <typename SendHandler, typename RecvHandler>
tcp_client<SendHandler, RecvHandler>::tcp_client()
{
  // init server
  epoll_fd = -1;
  servers = NULL;
  server_num = 0;
  conn_num = 0;
  sock_num = 0;
  send_free_num = 0;
  recv_free_num = 0;
  pthread_mutex_init(&monitor_mutex, NULL);
  pthread_mutex_init(&conn_table_mutex, NULL);
  pthread_mutex_init(&send_free_list_mutex, NULL);
  pthread_mutex_init(&recv_free_list_mutex, NULL);

  // init handler
  send_handler  = NULL;
  send_callback = NULL;
  recv_handler  = NULL;
  recv_callback = NULL;

  // init threads;
  connector_threads = NULL;
  monitor_threads = NULL;
  sender_threads = NULL;
  //receiver_threads = NULL;
  connector_num = 0;
  monitor_num = 0;
  sender_num = 0;
  //receiver_num = 0;
}

template <typename SendHandler, typename RecvHandler>
tcp_client<SendHandler, RecvHandler>::~tcp_client()
{
  //stop();
  if (monitor_num > 0)
  {
    for (int i=0; i<monitor_num; ++i)
      pthread_join(monitor_threads[i], NULL);
    delete []monitor_threads;
  }
  if (connector_num > 0)
  {
    for (int i=0; i<connector_num; ++i)
      pthread_join(connector_threads[i], NULL);
    delete []connector_threads;
  }
  if (sender_num > 0)
  {
    for (int i=0; i<sender_num; ++i)
      pthread_join(sender_threads[i], NULL);
    delete []sender_threads;
  }
  /*
  if (receiver_num > 0)
  {
    for (int i=0; i<receiver_num; ++i)
      pthread_join(receiver_threads[i], NULL);
    delete []receiver_threads;
  }
  */

  for (size_t i=0; i<send_free_list.size(); ++i)
    if (send_free_list[i])
      delete send_free_list[i];

  for (size_t i=0; i<recv_free_list.size(); ++i)
    if (recv_free_list[i])
      delete recv_free_list[i];
}

template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::net(const struct sockaddr_in* addrs, int server_num_, int conn_num_)
{
  if (!(server_num_ > 0 && conn_num_ > 0))
    return -1;

  server_num = server_num_;
  servers = new sockaddr_in[server_num];
  for (int i=0; i<server_num; i++) {
    memcpy(&servers[i], &addrs[i], sizeof(struct sockaddr_in));
  }

  conn_num = conn_num_;
  sock_num = server_num_ * conn_num_;
  sock_nodes = new conn_node[sock_num];
  return 0;
}


template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::register_send_handler(SendHandler* h, SendCallback c)
{
  send_handler = h;
  send_callback = c;
  return 0;
}


template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::register_recv_handler(RecvHandler* h, RecvCallback c)
{
  recv_handler = h;
  recv_callback = c;
  return 0;
}


template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::start()
{
  for (send_free_num=0; send_free_num<tcp_queue_size; send_free_num++) {
    send_free_list.push_back(new send_node);
  }

  for (recv_free_num=0; recv_free_num<tcp_queue_size; recv_free_num++) {
    recv_free_list.push_back(new recv_node);
  }

  // create epoll
  if ((epoll_fd = epoll_create(1)) == -1)
    return -1;

  for (int i=0; i<sock_num; i++)
  {
    pthread_mutex_init(&sock_nodes[i].lock, NULL);
    sock_nodes[i].fd = -1;
    sock_nodes[i].sock_id = i;
    sock_nodes[i].flag = TCP_DISCONNECT;
    sock_nodes[i].addr = servers[i/conn_num];

    int sock_fd;
    int ret;
    struct sockaddr_in* addr = &servers[i/conn_num];

    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) != -1)
    {
      if (sock_fd >= tcp_max_fd)
        return -1;

      set_socket(sock_fd);

      ret = connect(sock_fd, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
      // connect success
      if (ret == 0)
      {
        sock_nodes[i].flag = TCP_CONNECTED;

        epoll_event event;
        event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
        event.data.fd = sock_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event);


        _INFO("[tcp_client] [connected to %s:%hu] [sock_fd %d]",
          inet_ntoa(addr->sin_addr), ntohs(addr->sin_port), sock_fd);
      }
      // connect later
      else if (ret == -1 && errno == EINPROGRESS)
      {
        sock_nodes[i].flag = TCP_INPROGRESS;

        epoll_event event;
        event.events = EPOLLOUT | EPOLLONESHOT;
        event.data.fd = sock_fd;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &event);

        // _INFO("[tcp_client] [connect later] [sock_fd %d]", sock_fd);
      }
      // connect failed
      else
      {
        close(sock_fd);
        sock_fd = -1;
      }
    }
    if (sock_fd == -1)
    {
      _ERROR("[tcp_client] [cannot connect to %s:%hu]",
        inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
    }

    sock_nodes[i].fd = sock_fd;

    if (sock_fd != -1)
      conn_table[sock_fd] = &sock_nodes[i];

    conn_list.put(sock_nodes[i]);
  }

  connector_num = 4;
  connector_threads = new pthread_t[connector_num];
  for (int i=0; i<connector_num; i++)
    pthread_create(&connector_threads[i], NULL, connector_routine, this);

  monitor_num = 4;
  monitor_threads = new pthread_t[monitor_num];
  for (int i=0; i<monitor_num; i++)
    pthread_create(&monitor_threads[i], NULL, monitor_routine, this);

  sender_num = 10;
  sender_threads = new pthread_t[sender_num];
  for (int i=0; i<sender_num; i++)
    pthread_create(&sender_threads[i], NULL, sender_routine, this);

  //receiver_num = 10;
  //receiver_threads = new pthread_t[receiver_num];
  //for (int i=0; i<receiver_num; i++)
  //  pthread_create(&receiver_threads[i], NULL, receiver_routine, this);

  return 0;
}

template <typename SendHandler, typename RecvHandler>
void* tcp_client<SendHandler, RecvHandler>::connector_routine(void* arg)
{
  ((tcp_client<SendHandler, RecvHandler>*)arg)->connector();
  return NULL;
}

template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::connector()
{
  conn_node fake_node;
  fake_node.sock_id = -1;
  pthread_mutex_init(&(fake_node.lock), NULL);
  conn_list.put(fake_node);

  conn_node *node = NULL;
  while ((node = conn_list.get()) != NULL)
  {
    pthread_mutex_lock(&(node->lock));
    if (node->sock_id == -1)
    {
      timeval tm;
      tm.tv_sec  = 0;
      tm.tv_usec = tcp_connector_idle_timeout * 1000;
      select(1, NULL, NULL, NULL, &tm);
      conn_list.put(fake_node);
      pthread_mutex_unlock(&(node->lock));
      continue;
    }

    if (node->flag == TCP_DISCONNECT)
    {
      int ret;
      int sock;

      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) != -1)
      {
        if (sock >= tcp_max_fd) {
          close(sock);
          sock = -1;
        }

        set_socket(sock);

        struct sockaddr_in* addr = &(node->addr);
        ret = connect(sock, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
        if (ret == 0) {
          node->flag = TCP_CONNECTED;

          epoll_event event;
          event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
          event.data.fd = sock;
          epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &event);

          _INFO("[tcp_client] [connector] [connected to %s:%hu]", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
        }
        else if (ret == -1 && errno == EINPROGRESS) {
          node->flag = TCP_INPROGRESS;

          epoll_event event;
          event.events = EPOLLOUT | EPOLLONESHOT;
          event.data.fd = sock;
          epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &event);
        }
        else {
          close(sock);
          sock = -1;
        }
      }

      if (sock != -1) {
        add_socknode(sock, node);
      }
    }

    if (node->flag == TCP_DISCONNECT) {
      conn_list.put(*node);
    }

    pthread_mutex_unlock(&(node->lock));
  }
  return 0;
}

template <typename SendHandler, typename RecvHandler>
void* tcp_client<SendHandler, RecvHandler>::monitor_routine(void* arg)
{
  ((tcp_client<SendHandler, RecvHandler>*)arg)->monitor();
  return NULL;
}

template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::monitor()
{
  while (true)
  {
    int sock;
    bool success = false;
    bool remotedisconnect = false;
    bool isreadable = false;
    bool reset = false;
    epoll_event event;

    pthread_mutex_lock(&monitor_mutex);
    int ready = epoll_wait(epoll_fd, &event, 1, tcp_epoll_timeout);
    sock = event.data.fd;
    pthread_mutex_unlock(&monitor_mutex);
    if (ready <= 0) {
      continue;
    }

    conn_node* node = NULL;
    if (get_socknode(sock, node) != 0)
      continue;

    pthread_mutex_lock(&(node->lock));
    if (node->flag == TCP_INPROGRESS)
    {
      int err     = -1;
      socklen_t err_len = sizeof(err);
      if ((getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &err_len) == 0) && err == 0)
        success = true;
    } else if (event.events & EPOLLRDHUP) {
      remotedisconnect = true;
    } else if (event.events & EPOLLIN) {
      isreadable = true;
    }

    if (success) {
      node->flag = TCP_CONNECTED;

      epoll_event mod_event;
      mod_event.events = EPOLLIN | EPOLLRDHUP | EPOLLET;
      mod_event.data.fd = sock;
      epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sock, &mod_event);

      _INFO("[tcp_client] [monitor] [connected to %s:%hu]",
        inet_ntoa(node->addr.sin_addr), ntohs(node->addr.sin_port));
    } else if (isreadable) {
      if (recv_handler != NULL && recv_callback != NULL) {
        epoll_event event;

        // delete
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        event.data.fd = sock;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock, &event);

        unsigned char buf[tcp_pkg_max_size];
        int len = recv_request(sock, buf, sizeof(buf));
        if (len > 0) {
          (recv_handler->*recv_callback)(buf, len, node->addr);
        }

        // add
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
        event.data.fd = sock;
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &event);
      }
    } else {
      close(sock);
      del_socknode(sock);

      node->fd = -1;
      node->flag = TCP_DISCONNECT;
      conn_list.put(*node);

      if (remotedisconnect) {
        _ERROR("[tcp_client] [monitor] [remote disconnected]");
      } else {
        _ERROR("[tcp_client] [monitor] [cannot connect to %s:%hu]",
          inet_ntoa(node->addr.sin_addr), ntohs(node->addr.sin_port));
      }
    }

    pthread_mutex_unlock(&(node->lock));
  }
  return 0;
}

template <typename SendHandler, typename RecvHandler>
void* tcp_client<SendHandler, RecvHandler>::sender_routine(void* arg)
{
  ((tcp_client<SendHandler, RecvHandler>*)arg)->sender();
  return NULL;
}

template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::sender()
{
  int ret;
  int size;
  int sock;
  int sock_id;

  bool success = false;
  bool sock_close = false;

  struct send_node* node = NULL;
  while ((node = send_list.get()) != NULL)
  {
    if (node->retry_times-- < 0) {
      continue;
    }

    sock_id = node->server_id * conn_num + server_conn_ids[node->server_id];

    server_conn_ids[node->server_id]++;
    if (server_conn_ids[node->server_id] >= conn_num)
      server_conn_ids[node->server_id] = 0;

    conn_node* sock_node = &sock_nodes[sock_id];
    pthread_mutex_lock(&sock_node->lock);
    sock = sock_node->fd;

    if (sock_node->flag != TCP_INUSE && sock_node->flag != TCP_CONNECTED)
    {
      node->offset = 0;
      --node->retry_times;
    }
    else
    {
      size = node->package.head.size - node->offset;
      ret = ::send(sock, node->data + node->offset, size, 0);

      // send success
      if (ret == size)
      {
        success = true;
        sock_node->flag = TCP_CONNECTED;
      }
      else if (ret >= 0)
      {
        node->offset += ret;
        sock_node->flag = TCP_INUSE;
      }
      else
      {
        --node->retry_times;
        if (!(errno == EAGAIN || errno == EWOULDBLOCK))
        {
          sock_close = true;
        }

        if (sock_close)
        {
          close(sock);

          node->offset = 0;
          sock_node->fd = -1;
          sock_node->flag = TCP_DISCONNECT;

          del_socknode(sock);
          conn_list.put(*sock_node);
        }
      }
    }

    pthread_mutex_unlock(&sock_node->lock);

    if (success)
    {
      pthread_mutex_lock(&send_free_list_mutex);
      send_free_list[send_free_num++] = node;
      pthread_mutex_unlock(&send_free_list_mutex);
      if (send_callback != NULL) {
        (send_handler->*send_callback)(sock, node->server_id, 0);
      }
    }
    else if (node->retry_times > 0)
    {
      send_list.put(*node);
    }
    else
    {
      pthread_mutex_lock(&send_free_list_mutex);
      send_free_list[send_free_num++] = node;
      pthread_mutex_unlock(&send_free_list_mutex);
      if (send_callback != NULL) {
        (send_handler->*send_callback)(sock, node->server_id, -1);
      }
    }
  }
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::send(int server_id, const unsigned char* buf, size_t size, int retry_times)
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

  node->server_id = server_id;
  node->retry_times = retry_times;
  node->offset = 0;
  node->package.head.size = size + sizeof(struct tcp_package_head);
  memcpy(node->package.data, buf, size);

  send_list.put(*node);
  return 0;
}

/*
template <typename SendHandler, typename RecvHandler>
void* tcp_client<SendHandler, RecvHandler>::receiver_routine(void* arg)
{
  ((tcp_client<SendHandler, RecvHandler>*)arg)->receiver();
  return NULL;
}

template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::receiver()
{
  int fd;
  int len = 0;
  struct sockaddr_in addr;
  unsigned char buf[tcp_pkg_max_size];
  conn_node* snode = NULL;
  recv_node* rnode = NULL;
  while ((rnode = recv_list.get()) != NULL)
  {
    fd = rnode->fd;
    if (get_socknode(fd, snode) != 0)
      continue;

    pthread_mutex_lock(&(snode->lock));
    len  = recv_request(fd, buf, sizeof(buf));
    addr = snode->addr;
    pthread_mutex_unlock(&(snode->lock));

    if (len > 0 && recv_handler) {
      (recv_handler->*recv_callback)(buf, len, addr);
    }

    pthread_mutex_lock(&recv_free_list_mutex);
    ++ recv_free_num;
    pthread_mutex_unlock(&recv_free_list_mutex);
  }
  return 0;
}
*/

template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::recv_request(int fd, void* buf, size_t buf_len)
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
int tcp_client<SendHandler, RecvHandler>::readn_timeout(int fd, void* buf, size_t buf_len, timeval* timeout)
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
int tcp_client<SendHandler, RecvHandler>::set_socket(int fd)
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
int tcp_client<SendHandler, RecvHandler>::add_socknode(int fd, struct conn_node*& node)
{
  pthread_mutex_lock(&conn_table_mutex);
  conn_table[fd] = node;
  pthread_mutex_unlock(&conn_table_mutex);
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::get_socknode(int fd, struct conn_node*& node)
{
  pthread_mutex_lock(&conn_table_mutex);
  conn_table_iterator_t it = conn_table.find(fd);
  if (it == conn_table.end()) {
    pthread_mutex_unlock(&conn_table_mutex);
    return -1;
  }

  node = conn_table[fd];
  pthread_mutex_unlock(&conn_table_mutex);
  return 0;
}

template <typename SendHandler, typename RecvHandler>
int tcp_client<SendHandler, RecvHandler>::del_socknode(int fd)
{
  int ret = 0;
  pthread_mutex_lock(&conn_table_mutex);
  conn_table_iterator_t it = conn_table.find(fd);
  if (it == conn_table.end())
    ret = -1;
  else
    conn_table.erase(it);
  pthread_mutex_unlock(&conn_table_mutex);
  return ret;
}

#endif //_TCP_CLIENT_HPP
