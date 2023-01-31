/****************************************************************************
  Copyright (C), 2011, Robert Fan

  Histroy  :
            1)  Created by fanshaohua   2011/08/10

*/
//////////////////////////////////////////////////////////////////////////////
//      !!!!!!重要!!!!!!!
//      接口中所有的字节序位 CPU 字节序，即与CPU处理方式一致，不需要额外转化


#ifndef  __SERVICE__
#define  __SERVICE__

#ifdef __cplusplus
    extern "C" {
#endif

#define  LOG_PRINTF(a,b...)   core_printf((a),##b)

//系统内部SERVICE定义 0 ~ 127
#define  USER_APP_SERVICE    0x80
#define  APP_CAMERA           ( USER_APP_SERVICE + 1 )
#define  APP_RECORDER         ( USER_APP_SERVICE + 2 )
#define  APP_EVENT            ( USER_APP_SERVICE + 3 )
#define  APP_DOME             ( USER_APP_SERVICE + 4 )
#define  APP_FORESTFIRE       ( USER_APP_SERVICE + 5 )
#define  APP_SERIAL           ( USER_APP_SERVICE + 6 )

#define  APP_BOARD            ( USER_APP_SERVICE + 7 )
#define  APP_EMAIL            ( USER_APP_SERVICE + 8 )
#define  APP_NETSTREAM        ( USER_APP_SERVICE + 9 )
#define  APP_SYSLOG           ( USER_APP_SERVICE + 10 )
#define  APP_NETWORK          ( USER_APP_SERVICE + 11 )
#define  APP_DETECTOR         ( USER_APP_SERVICE + 12 )
#define  APP_PTZ_SERVER       ( USER_APP_SERVICE + 13 )
#define  APP_APPGUI           ( USER_APP_SERVICE + 14 )
#define  APP_TRACKER          ( USER_APP_SERVICE + 15 )
#define  APP_OSD              ( USER_APP_SERVICE + 16 )
#define  APP_MTLIB            ( USER_APP_SERVICE + 17 )
#define  APP_DRONE            ( USER_APP_SERVICE + 18 )
#define  APP_AI               ( USER_APP_SERVICE + 19 )


#define  APP_USER1            ( USER_APP_SERVICE + 32 )
#define  APP_USER2            ( USER_APP_SERVICE + 33 )

#define  APP_UAV              ( USER_APP_SERVICE + 35 )
#define  APP_GB28181          ( USER_APP_SERVICE + 36 )








//服务定义 
typedef struct _tagCORE_SERVICE
{
    char * description ;
    int    id          ;

    int ( * start ) ( void * priv ) ;
    int ( * stop  ) ( void * priv ) ;
    int ( * service_message ) ( void * priv , int sender , void * msg ) ;
    int ( * service_command ) ( void * priv , int op , int len , void * ibuf , void * obuf ) ;
            
    void * private_data ;       //私有变量
        
    struct _tagCORE_SERVICE * next ;
    
}CORE_SERVICE ;


/*模块间消息结构——头部定义*/
typedef struct
{
    int  sender   ;      //发送模块
    int  receiver ;      //接受模块
    int  command  ;      //命令字
    int  length   ;      //内容长度
}MESSAG_HEADER ;

typedef struct
{
    int  sender   ;      //发送模块
    int  receiver ;      //接受模块
    int  command  ;      //命令字
    int  length   ;      //内容长度
    char data[128] ;     //内容，不一定为128字节
}KERNEL_MESSAGE   ;

int  service_init  ( void ) ;
int  service_exit  ( void ) ;
int  register_service  ( CORE_SERVICE * service ) ;
int  unregister_service( int id ) ;
int  core_list_service ( char *buf ) ;

//发送消息，无返回数据
int  core_send_message( KERNEL_MESSAGE * msg ) ;
//执行一个命令，可以有返回值
int  core_do_command  ( int id , int op , int len , void * ibuf , void * obuf );


void core_fill_message( MESSAG_HEADER * h , int sender , int receiver , int cmd , int total ) ;

unsigned long gettickcount(void);

#ifdef __cplusplus
    }
#endif

#endif

