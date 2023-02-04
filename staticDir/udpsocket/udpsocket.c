#include "udpsocket.h"
#include <stdio.h>
#include <stdlib.h>

int create_socket_udp(UDP_SOCKET* udp)
{   
    if(!sizeof(udp))
    {
       return -1;
    }
    udp->socket_ip = "192.168.6.200";
    udp->socket_port = "8001";

    udp->addrMe.sin_family       = AF_INET;
    udp->addrMe.sin_port         = htons(atoi("1001"));
    udp->addrMe.sin_addr.s_addr  = INADDR_ANY;

    udp->addrser.sin_addr.s_addr = inet_addr(udp->socket_ip);
    udp->addrser.sin_family      = AF_INET;
    udp->addrser.sin_port        = htons(atoi(udp->socket_port));     
    udp->socket_fd = socket(AF_INET ,SOCK_DGRAM,0);
    bind(udp->socket_fd, (struct sockaddr*)&udp->addrMe, sizeof(udp->addrMe));

    return 0;
}