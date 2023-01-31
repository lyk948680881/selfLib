#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <stdarg.h>

#include "sys-core.h"
#include "sys-service.h"
#include "app-sysctrl.h"


/////////////////////////////////////////////////////////////////
//     Function : recv_nn_char
//  Description : recv number of chars with timeout
//        Input :
//       Output : 
int  recv_nn_char( int s , char * buf , int len , int sec) 
{
    int ret , count ;
    
    count = len ;    
    while( len > 0 )
    {
        ret = recv_with_timeout( s , buf , len , sec ) ;
        if( ret <= 0 )
            return ret ;
        len   -= ret ;
        buf   += ret ;
    }
    
    return count ;
}



/////////////////////////////////////////////////////////////////
//     Function : recv_n_char
//  Description :
int  recv_n_char( int socket , char * buf , int len  )
{
    int   ret , count ;
    char *ptr ;

    ptr   = buf ;
    count = len ;
    do{
        ret = recv( socket , ptr , count , 0 ) ;
        if( ret <= 0 )
        {
            close( socket ) ;
            return 0 ;
        }
        count  -= ret ;
        ptr    += ret ;
    }while( count > 0 ) ;

    return len ;
}


/////////////////////////////////////////////////////////////////
//     Function : recv_with_timeout
//  Description : wait data in for some time
//        Input :
//       Output : 0 if failed
int  recvfrom_with_timeout( int s , char * buf , int len , int sec )
{
    struct sockaddr_in  from;
    struct timeval m_tv;    
    fd_set      fdread;
    int ret , tep ;
    

    memset ((char *) &m_tv , 0 , sizeof( struct timeval ) );
    m_tv.tv_sec = sec ;

    FD_ZERO( &fdread );
    FD_SET( s, &fdread );

    ret = select( FD_SETSIZE , &fdread, NULL, NULL, sec?&m_tv:NULL ) ;
    if( ret <= 0 || !FD_ISSET( s , &fdread ) )
        return 0 ;

    tep = sizeof( struct sockaddr_in );
    return recvfrom( s , buf , len, 0, (struct sockaddr *)&from, (socklen_t *)&tep);        
}


/////////////////////////////////////////////////////////////////
//     Function : recv_with_timeout
//  Description : wait data in for some time
//        Input :
//       Output : 0 if failed
int  recv_with_timeout( int s , char * buf , int len , int sec )
{
    struct timeval m_tv;
    fd_set      fdread;
    int ret = 0;

    memset ((char *) &m_tv , 0 , sizeof( struct timeval ) );
    m_tv.tv_sec  = sec ;

    FD_ZERO( &fdread );
    FD_SET( s, &fdread );

    ret = select( FD_SETSIZE , &fdread, NULL, NULL, sec?&m_tv:NULL ) ;
    if( ret <= 0  )
        return 0 ;

    return recv( s , buf, len , 0 );     //really recv data from socket
}



//////////////////////////////////////////////////////////////////////////////////////
//     Function : connect_with_timeout
//  Description : 
//        Input :
//       Output :
// 带超时的非阻塞式connect()，这是W. Richard Stevens的实现
int connect_with_timeout( int s , U32 ip , int port , int sec ) 
{    
    struct sockaddr_in addr ;
    int                flags, error , ret ;
    fd_set             rset, wset;
    struct timeval     tv;
    
    memset( &addr, 0, sizeof( addr ) );
    addr.sin_family      = AF_INET  ;
    addr.sin_addr.s_addr = (in_addr_t)ip       ; //BIG ENDIAN
    addr.sin_port        = htons( port )   ;    
  
    if ( ( flags = fcntl( s, F_GETFL, 0 ) ) < 0 )
        return 0 ;
    if ( fcntl( s, F_SETFL, flags | O_NONBLOCK ) < 0 )
        return 0 ;

    ret = connect( s , (struct sockaddr *)&addr , sizeof( struct sockaddr ) ) ;    
    if( ret == 0 )
        return s ;
    if( ret < 0 && errno != EINPROGRESS )
        return 0 ;

    
    FD_ZERO( &rset );
    FD_SET( s, &rset );
    wset       = rset;
    tv.tv_sec  = sec ;
    tv.tv_usec = 0;

    ret = select( s + 1, &rset, &wset, NULL, sec ? &tv : NULL );
    if( ret <= 0 )
        return 0 ;

    if( FD_ISSET( s, &rset ) || FD_ISSET( s, &wset ) )
    {
        //check if connection refused
        ret = sizeof(int) ;
        if ( getsockopt( s, SOL_SOCKET, SO_ERROR, &error , (socklen_t*)&ret ) < 0 )
            return 0 ;

        if( error != 0 )
            return 0 ;
    }else{
        return 0 ;
    }

    //连接上了，设置为BLOCK模式
    if ( fcntl( s, F_SETFL, flags ) < 0 )
        return 0 ;
    return s ;
}  



/////////////////////////////////////////////////////////
//     Function : core_printf
//  Description :
//        Input :
//       Output :
int  core_printf( const char * fmt , ... )
{
    KERNEL_MESSAGE  * msg ;
    int    buffer[64] , len ;
    va_list marker ;

    msg = ( KERNEL_MESSAGE  * ) buffer ;

    va_start( marker, fmt );
    len = vsnprintf( msg->data , 200 , fmt , marker ) + 1 ;
    va_end( marker );

    msg->command = 0xFFFF         ; //SYSCTRL_PRINTF
    msg->length  = len            ;
    msg->receiver= SYSTEM_CTRL    ;
    msg->sender  = SYSTEM_CTRL    ;

    core_send_message( msg ) ;    

    return len ;
}

