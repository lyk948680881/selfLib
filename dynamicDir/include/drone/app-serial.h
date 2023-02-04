
/***************************************************************************
  Copyright (C), 2011, Jacky

  Histroy    :
              1)  Created by Jacky   2021/01/20
  Description:
****************************************************************************/

#ifndef __APP_SERIAL__
#define __APP_SERIAL__

#include "sys-config.h"

#define MAX_SERIAL_PORT    5

#define  APP_SERIAL_MSG                ( APP_SERIAL<<16)
#define  APP_SERIAL_PORT0_REG_CB       ( APP_SERIAL_MSG + 0 )
#define  APP_SERIAL_PORT1_REG_CB       ( APP_SERIAL_MSG + 1 )
#define  APP_SERIAL_PORT2_REG_CB       ( APP_SERIAL_MSG + 2 )
#define  APP_SERIAL_PORT3_REG_CB       ( APP_SERIAL_MSG + 3 )
#define  APP_SERIAL_PORT4_REG_CB       ( APP_SERIAL_MSG + 4 )

#define  APP_SERIAL_PORT0_UNREG_CB      ( APP_SERIAL_MSG + 5 )
#define  APP_SERIAL_PORT1_UNREG_CB      ( APP_SERIAL_MSG + 6 )
#define  APP_SERIAL_PORT2_UNREG_CB      ( APP_SERIAL_MSG + 7 )
#define  APP_SERIAL_PORT3_UNREG_CB      ( APP_SERIAL_MSG + 8 )
#define  APP_SERIAL_PORT4_UNREG_CB      ( APP_SERIAL_MSG + 9 )

#define  APP_SERIAL_SEND_PORT0_DATA    ( APP_SERIAL_MSG + 10 )
#define  APP_SERIAL_SEND_PORT1_DATA    ( APP_SERIAL_MSG + 11 )
#define  APP_SERIAL_SEND_PORT2_DATA    ( APP_SERIAL_MSG + 12 )
#define  APP_SERIAL_SEND_PORT3_DATA    ( APP_SERIAL_MSG + 13 )
#define  APP_SERIAL_SEND_PORT4_DATA    ( APP_SERIAL_MSG + 14 )


typedef int (* RECV_CALLBACK)(U8 *fr, int len, void *priv ) ;

typedef struct _tagSERIAL_CALLBACK
{
    RECV_CALLBACK cb ;
    void * param;
    struct _tagSERIAL_CALLBACK *next;
}SERIAL_CALLBACK;

typedef struct
{
    int    index ;
    void * priv  ;
}SERIAL_DATA_PORT ;

typedef struct
{
    int               running ;
    MSG_Q_ID          queue;
    SHM_CONFIG       *shmcfg ;

    pthread_mutex_t   cb_mutex    ;   //回调互锁信号量
    pthread_mutex_t   send_mutex  ;   //发送互锁信号量
    int permit         ;
    
    SERIAL_DATA_PORT   port[MAX_SERIAL_PORT];
    int                fd  [MAX_SERIAL_PORT];    
    SERIAL_CALLBACK   *callback[MAX_SERIAL_PORT] ;

}SERIAL_CONTEXT ;

#define SERIAL_SEND_DATA(num, data, len) core_do_command( APP_SERIAL, APP_SERIAL_SEND_PORT##num##_DATA , len , (void*)data , NULL )

#define SERIAL_CALLBACK_REG(num, callback, priv)  \
{  \
    sprintf( priv->name, "Serial %d:", num );  \
    SERIAL_CALLBACK serial_cb ;    \
    serial_cb.cb    = callback;            \
    serial_cb.param = priv;        \
    serial_cb.next  = NULL;         \
    core_do_command( APP_SERIAL, APP_SERIAL_PORT##num##_REG_CB , sizeof(SYSENC_CALLBACK) , (void*)&serial_cb , NULL ); \
}

#define SERIAL_CALLBACK_UNREG(num, callback, priv)   \
{  \
    SERIAL_CALLBACK serial_cb ;    \
    serial_cb.cb    = callback;            \
    serial_cb.param = priv;        \
    serial_cb.next  = NULL;         \
    core_do_command( APP_SERIAL, APP_SERIAL_PORT##num##_UNREG_CB , sizeof(SYSENC_CALLBACK) , (void*)&serial_cb , NULL ); \
}


#endif

