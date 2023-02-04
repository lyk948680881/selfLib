/****************************************************************************
  
  Description:
            网络码流发送模块，使用http(tcp/udp)协议
  Histroy  :
  
*****************************************************************************/
#ifndef _NET_STREAM_H_
#define _NET_STREAM_H_

#include "frame.h"


#define  APP_NETSTREAM_MSG       ( APP_NETSTREAM << 16 )

#define   NETSTREAM_USER_ADD            ( APP_NETSTREAM_MSG + 0x01 )
#define   NETSTREAM_USER_DEL            ( APP_NETSTREAM_MSG + 0x02 )
#define   NETSTREAM_USER_QUERY          ( APP_NETSTREAM_MSG + 0x03 )
#define   NETSTREAM_REQUEST             ( APP_NETSTREAM_MSG + 0x10 )
#define   NETSTREAM_HEART_BEAT          ( APP_NETSTREAM_MSG + 0x40 )

#define   NETSTREAM_ACCEPT_OK           ( APP_NETSTREAM_MSG + 0x80 )
#define   NETSTREAM_ACCEPT_FAIL         ( APP_NETSTREAM_MSG + 0x81 )
#define   NETSTREAM_EXTERNAL_TEMP       ( APP_NETSTREAM_MSG + 0x82 )

#define   NETSTREAM_FRAME_IND           ( APP_NETSTREAM_MSG + 0x100 )
#define   NETSTREAM_VIDEO_IND           ( APP_NETSTREAM_MSG + 0x101 )


//用户信息
typedef struct{
    int    index       ;
    int    stream      ;
    int    stream_type ;
    int    xfer_type   ;
    int    remote_ip   ;       //UPD/MUL用
    int    port        ;
}USER_QUERY_INFO ;

//用户查询
typedef struct{
    int   count    ;
    USER_QUERY_INFO user[0]  ;
}NET_USER_QUERY ;





#define  XFER_VIDEO_MAIN_STREAM    0x01
#define  XFER_VIDEO_SUB_STREAM     0x02
#define  XFER_AUDIO_STREAM         0x04
#define  XFER_JPEG_STREAM          0x08
#define  XFER_TALK_STREAM          0x10
#define  XFER_YUV_STREAM           0x20
#define  XFER_TEMP_STREAM          0x40
#define  XFER_FIFO_STREAM          0x80


//XFER MODE
#define  XFER_TCP_MODE            0x01
#define  XFER_UDP_MODE            0x02
#define  XFER_MUL_MODE            0x04


#define  XFER_FRAME               0
#define  XFER_RAW                 1
#define  XFER_FLV                 2

#define  REQ_TCP_METHOD          0
#define  REQ_HTTP_METHOD         1

#define  HEART_BEAT_TIMEOUT       15    //15秒

#define  SYSTEM_STREAM_GET   0x20544547      //'GET '
#define  SYSTEM_STREAM_POST  0x54534F50      //'POST'


#define  REMOTE_LOGIN_REQUEST  0x1001 //集中管理软件登录命令，网络字节序
#define  REMOTE_ACCEPT_OK	   0x10C8
#define  REMOTE_ACCEPT_FAIL	   0x10C9

#define  DVR_IPCamera_GQ_3512   14
#define  DVR_IPCamera_GQ_3515   23

//UDP 控制端口的命令分类定义
#define  HEART_BEAT_MSG       0x436F         //心跳接口消息：使用了网络字节序 'Co'


//网络连接结构体
typedef struct{              //TCP连接时标识什么类型
    int  command  ;
    int  channel  ;
    int  method   ;          //请求类型HTTP/TCP
    int  stream   ;
    int  stream_type ;       //Frame/RAW
    int  xfer_type   ;       //TCP/UDP/MUL
    int  remote_ip   ;
    int  port        ;
    int  snap        ;
    int  heart_beat  ;       //UDP
    int  frames      ;       //JPEG/LIVE 发送帧数，-1一直输出
    int  interval    ;
    int  backward    ;       //回溯时间
}TCP_REQUEST_MSG ;


//码流请求返回
typedef struct{               //TCP连接时标识什么类型
    int  command  ;
    int  channel  ;
    int  width    ;
    int  height   ;
    int  frame_rate ;
    int  audio_sample ;
    int  reserved1 ;
    int  reserved2 ;
}TCP_RESPONSE_MSG ;

//集中管理软件登录请求
struct tagREMOTE_LOGIN_REQUEST_MSG{
    short  sender        ;
    short  command       ;
    char   user[32]      ;
    char   pass[16]      ;
}PACK_OPTION ;
typedef struct tagREMOTE_LOGIN_REQUEST_MSG REMOTE_LOGIN_REQUEST_MSG ;

//集中管理软件登录返回
struct tagREMOTE_LOGIN_RESPONSE_MSG{
    short  sender        ;
    short  command       ;
    int    reason        ;
    short  userid        ;
    char   unused[63]    ;
    char   new_type_flag ;
    short  alarm_in_num  ;
    short  alarm_out_num ;
    short  disk_num ;
    short  dvr_type ;
    short  chan_num ;
}PACK_OPTION ;
typedef struct tagREMOTE_LOGIN_RESPONSE_MSG REMOTE_LOGIN_RESPONSE_MSG ;






#endif

