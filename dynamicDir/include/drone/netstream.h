/****************************************************************************
  
  Description:
            ������������ģ�飬ʹ��http(tcp/udp)Э��
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


//�û���Ϣ
typedef struct{
    int    index       ;
    int    stream      ;
    int    stream_type ;
    int    xfer_type   ;
    int    remote_ip   ;       //UPD/MUL��
    int    port        ;
}USER_QUERY_INFO ;

//�û���ѯ
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

#define  HEART_BEAT_TIMEOUT       15    //15��

#define  SYSTEM_STREAM_GET   0x20544547      //'GET '
#define  SYSTEM_STREAM_POST  0x54534F50      //'POST'


#define  REMOTE_LOGIN_REQUEST  0x1001 //���й��������¼��������ֽ���
#define  REMOTE_ACCEPT_OK	   0x10C8
#define  REMOTE_ACCEPT_FAIL	   0x10C9

#define  DVR_IPCamera_GQ_3512   14
#define  DVR_IPCamera_GQ_3515   23

//UDP ���ƶ˿ڵ�������ඨ��
#define  HEART_BEAT_MSG       0x436F         //�����ӿ���Ϣ��ʹ���������ֽ��� 'Co'


//�������ӽṹ��
typedef struct{              //TCP����ʱ��ʶʲô����
    int  command  ;
    int  channel  ;
    int  method   ;          //��������HTTP/TCP
    int  stream   ;
    int  stream_type ;       //Frame/RAW
    int  xfer_type   ;       //TCP/UDP/MUL
    int  remote_ip   ;
    int  port        ;
    int  snap        ;
    int  heart_beat  ;       //UDP
    int  frames      ;       //JPEG/LIVE ����֡����-1һֱ���
    int  interval    ;
    int  backward    ;       //����ʱ��
}TCP_REQUEST_MSG ;


//�������󷵻�
typedef struct{               //TCP����ʱ��ʶʲô����
    int  command  ;
    int  channel  ;
    int  width    ;
    int  height   ;
    int  frame_rate ;
    int  audio_sample ;
    int  reserved1 ;
    int  reserved2 ;
}TCP_RESPONSE_MSG ;

//���й��������¼����
struct tagREMOTE_LOGIN_REQUEST_MSG{
    short  sender        ;
    short  command       ;
    char   user[32]      ;
    char   pass[16]      ;
}PACK_OPTION ;
typedef struct tagREMOTE_LOGIN_REQUEST_MSG REMOTE_LOGIN_REQUEST_MSG ;

//���й��������¼����
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

