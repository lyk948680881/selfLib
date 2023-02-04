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







//����ͨ���ֲ� 
#define  VENC_MIX_CHANNEL      0      //������-�ںϻ��߷ָ�棬��OSD����
#define  VENC_IR_CHANNEL       1      //����
#define  VENC_VI_CHANNEL       2      //�ɼ���
#define  AENC_CHANNEL          0
#define  ADEC_CHANNEL          0
#define  JPEG_CHANNEL          0
#define  JPEG_IR_CHANNEL       1  
#define  JPEG_VI_CHANNEL       2  

#define  SYSAPP_CODEC_IR_PORT_W     720
#define  SYSAPP_CODEC_IR_PORT_H     576
#define  SYSAPP_CODEC_VI_PORT_W     1920
#define  SYSAPP_CODEC_VI_PORT_H     1080


//H264���붯̬��������
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



//JPEG���붯̬��������
typedef struct{
    MESSAG_HEADER  head  ;
    
    int            channel;        
    int            enable ;
    VIDEO_ENCODER_CHANNEL       jpeg;
}ENCODER_JPEG_PARAMETER_MSG ;


//JPEG���붯̬��������
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

//AUDIO��̬����,��Ƶֻ֧��enable/disable
typedef struct{
    MESSAG_HEADER  head  ;
    
    int           channel       ;  
    int            enable       ;    
    AUDIO_CODEC_CHANNEL    audio;      
}ENCODER_AUDIO_PARAMETER_MSG    ;



///////////////////////////////////////////////////////////////////
//�ص���ȥ��֡���ͣ�ע��ʹ�÷���
typedef struct _tagSYSENC_FR
{
    //ͨ���������ô������������Ƿ���Ҫ�ͷ�
    //�Լ����ڴ�COPY����
    int   reference            ;   //���ô�����Ϊ0ʱ��ʾ����ʹ����    
    AV_STREAM_FRAME     frame  ;    
    struct _tagSYSENC_FR * next;   //������
}SYSENC_FRAME ;


#define  FR_VIDEO         0x0001       //��Ƶ��
#define  FR_AUDIO         0x0002       //��Ƶ��
#define  FR_YUV           0x0004       //yuvͼƬ
#define  FR_PICTURE       0x0080       //JPEGͼƬ
typedef  int ( * FRAME_CALLBACK ) ( SYSENC_FRAME * fr ,  void * para ) ;
typedef struct _tagSYSENC_CALLBACK
{
    int  channel ;                      //ָ��ͨ����-1 ��ʾ��������ͨ������
    int  type    ;                      //���� bit��ϳ�
    void * param ;
    FRAME_CALLBACK  callback ;
    struct _tagSYSENC_CALLBACK * next ; //������
}SYSENC_CALLBACK ;

#define  SYSENC_FR_ADDREF(fr)   core_do_command( SYS_CODEC , SYSENC_ADD_REF , sizeof(SYSENC_FRAME) , fr , NULL ) 
#define  SYSENC_FR_SUBREF(fr)   core_do_command( SYS_CODEC , SYSENC_SUB_REF , sizeof(SYSENC_FRAME) , fr , NULL ) 

//��Encoderע��һ��ָ������type�Ļص�������
//ע��ɹ��󷵻�ϵͳΨһ�ķ�������callback_id�����򷵻�0
//ע���ص�����ʱͨ��callback_idע��ָ���Ļص�����
#define  SYSENC_CALLBACK_REG(cb)    core_do_command( SYS_CODEC , SYSENC_REG_CB , sizeof(SYSENC_CALLBACK) , (void*)cb , NULL ) 
#define  SYSENC_CALLBACK_UNREG(id)  core_do_command( SYS_CODEC , SYSENC_UNREG_CB , 0 , (void*)id , NULL ) 



CORE_SERVICE * syscodec_service   ( ) ;

#ifdef __cplusplus
    }
#endif

#endif
