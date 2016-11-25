#ifndef _TCP_COMM_HPP
#define _TCP_COMM_HPP
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define tcp_queue_size         3072
#define tcp_max_fd             1024
#define tcp_sbuf_size          (20 * 1024 * 1024)
#define tcp_rbuf_size          (20 * 1024 * 1024)

// 1500(MTU) - 38(Ethernet) - 20(IP) - 20(TCP) = 1422
#define tcp_pkg_max_size        14220
#define tcp_data_max_size       (tcp_pkg_max_size - sizeof(tcp_package))

#define tcp_read_timeout              100
#define tcp_epoll_timeout             500
#define tcp_connector_idle_timeout    1000

struct tcp_package_head
{
	unsigned int size;
	int port;
};

struct tcp_package
{
    tcp_package_head head;
    unsigned char data[];
};

#endif //_TCP_COMM_HPP
