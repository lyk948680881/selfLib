/****************************************************************************
  Copyright (C), 2011, Robert Fan

  Histroy  :
            1)  Created by fanshaohua   2011/08/10
            
  
*/

#ifndef __APP_SYSENC__
#define __APP_SYSENC__

#ifdef __cplusplus
    extern "C" {
#endif


//#define  IRGRAY_OUTLINE

#define  SYS_CODEC                     2         
#define  SYS_CODEC_MSG                (SYS_CODEC<<16)

//Message for sysenc
#define  SYSENC_SET_H264_PARAM         ( SYS_CODEC_MSG + 0 )
#define  SYSENC_SET_JPEG_PARAM         ( SYS_CODEC_MSG + 1 )
#define  SYSENC_SET_AUDIO_PARAM        ( SYS_CODEC_MSG + 2 )
#define  SYSENC_GET_H264_PARAM         ( SYS_CODEC_MSG + 3 )
#define  SYSENC_GET_JPEG_PARAM         ( SYS_CODEC_MSG + 4 )
#define  SYSENC_GET_AUDIO_PARAM        ( SYS_CODEC_MSG + 5 )
#define  SYSENC_SET_FPS_PARAM          ( SYS_CODEC_MSG + 6 )
#define  SYSENC_INSERT_I_FRAME         ( SYS_CODEC_MSG + 7 )
#define  SYSENC_UPDATE_MTPARAM         ( SYS_CODEC_MSG + 8 )
#define  SYSENC_SET_IRDATA_PARAM       ( SYS_CODEC_MSG + 9 )

#define  SYSENC_INSERT_I_FRAME_100MS   ( SYS_CODEC_MSG + 0x100 )

#define  SYSENC_JPEG_SNAP_SHOT         ( SYS_CODEC_MSG + 10 )

#define  SYSENC_ADD_REF                ( SYS_CODEC_MSG + 20 )
#define  SYSENC_SUB_REF                ( SYS_CODEC_MSG + 21 )
#define  SYSENC_REG_CB                 ( SYS_CODEC_MSG + 22 )
#define  SYSENC_UNREG_CB               ( SYS_CODEC_MSG + 23 )
#define  SYSENC_GET_USBVI_COUNT        ( SYS_CODEC_MSG + 24 )
#define  SYSENC_GET_ENC_COUNT          ( SYS_CODEC_MSG + 25 )
#define  SYSENC_SET_MTLIB              ( SYS_CODEC_MSG + 26 )
#define  SYSENC_SET_MTPARAM            ( SYS_CODEC_MSG + 27 )
#define  SYSENC_SET_VIDEO_MODE         ( SYS_CODEC_MSG + 28 )
#define  SYSENC_SET_COLORBAR_ID        ( SYS_CODEC_MSG + 29 )

#define  SYSDEC_SEND_STREAM            ( SYS_CODEC_MSG + 30 )







//码流通道分布 
#define  VENC_MIX_CHANNEL      0      //主码流-融合或者分割画面，有OSD叠加
#define  VENC_IR_CHANNEL       1      //红外
#define  VENC_VI_CHANNEL       2      //可见光
#define  AENC_CHANNEL          0
#define  ADEC_CHANNEL          0
#define  JPEG_CHANNEL          0
#define  JPEG_IR_CHANNEL       1  
#define  JPEG_VI_CHANNEL       2  

#define  SYSAPP_CODEC_IR_PORT_W     720
#define  SYSAPP_CODEC_IR_PORT_H     576
#define  SYSAPP_CODEC_VI_PORT_W     1920
#define  SYSAPP_CODEC_VI_PORT_H     1080


//H264编码动态参数调整
typedef struct{
    MESSAG_HEADER    head;
    
    int                  channel  ;          //
    int                    enable ;
    VIDEO_ENCODER_CHANNEL   h264  ;
}ENCODER_H264_PARAMETER_MSG ;

typedef struct{
    MESSAG_HEADER    head ;
    
    int              channel ;
}ENCODER_INSERT_IDR_MSG ;


typedef struct {
    MESSAG_HEADER    head ;
    void             *av ;
}SEND_FRAME_MSG;



//JPEG编码动态参数调整
typedef struct{
    MESSAG_HEADER  head  ;
    
    int            channel;        
    int            enable ;
    VIDEO_ENCODER_CHANNEL       jpeg;
}ENCODER_JPEG_PARAMETER_MSG ;


//JPEG编码动态参数调整
typedef struct{
    MESSAG_HEADER  head  ;
    
    int            fps ;
}ENCODER_IRDATA_PARAMETER_MSG ;

typedef struct {
    MESSAG_HEADER  head ;
    
    int            k ;
    int            m ;
    int            c ;
    int            back  ;
    int            vtemp ;
}SYSENC_MT_PARAMETER_MSG ;

//AUDIO动态参数,音频只支持enable/disable
typedef struct{
    MESSAG_HEADER  head  ;
    
    int           channel       ;  
    int            enable       ;    
    AUDIO_CODEC_CHANNEL    audio;      
}ENCODER_AUDIO_PARAMETER_MSG    ;



///////////////////////////////////////////////////////////////////
//回调出去的帧类型，注意使用方法
typedef struct _tagSYSENC_FR
{
    //通过调整引用次数，来决定是否需要释放
    //以减少内存COPY次数
    int   reference            ;   //引用次数，为0时表示无人使用了    
    AV_STREAM_FRAME     frame  ;    
    struct _tagSYSENC_FR * next;   //链表用
}SYSENC_FRAME ;


#define  FR_VIDEO         0x0001       //视频流
#define  FR_AUDIO         0x0002       //音频流
#define  FR_YUV           0x0004       //yuv图片
#define  FR_PICTURE       0x0080       //JPEG图片
typedef  int ( * FRAME_CALLBACK ) ( SYSENC_FRAME * fr ,  void * para ) ;
typedef struct _tagSYSENC_CALLBACK
{
    int  channel ;                      //指定通道，-1 表示接受所有通道数据
    int  type    ;                      //类型 bit组合成
    void * param ;
    FRAME_CALLBACK  callback ;
    struct _tagSYSENC_CALLBACK * next ; //链表用
}SYSENC_CALLBACK ;

#define  SYSENC_FR_ADDREF(fr)   core_do_command( SYS_CODEC , SYSENC_ADD_REF , sizeof(SYSENC_FRAME) , fr , NULL ) 
#define  SYSENC_FR_SUBREF(fr)   core_do_command( SYS_CODEC , SYSENC_SUB_REF , sizeof(SYSENC_FRAME) , fr , NULL ) 

//向Encoder注册一个指定类型type的回调函数，
//注册成功后返回系统唯一的非零正数callback_id，否则返回0
//注销回调函数时通过callback_id注销指定的回调函数
#define  SYSENC_CALLBACK_REG(cb)    core_do_command( SYS_CODEC , SYSENC_REG_CB , sizeof(SYSENC_CALLBACK) , (void*)cb , NULL ) 
#define  SYSENC_CALLBACK_UNREG(id)  core_do_command( SYS_CODEC , SYSENC_UNREG_CB , 0 , (void*)id , NULL ) 



CORE_SERVICE * syscodec_service   ( ) ;

#ifdef __cplusplus
    }
#endif

#endif
