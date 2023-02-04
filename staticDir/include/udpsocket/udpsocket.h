#ifndef __udpsocket_h__
#define __udpsocket_h__

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct{
    int  socket_fd;
    int  socket_cli;
    struct sockaddr_in addrser;
    struct sockaddr_in addrMe; 
    char * socket_ip     ;
    char * socket_port   ;

}UDP_SOCKET;

int create_socket_udp(UDP_SOCKET* udp);

#endif