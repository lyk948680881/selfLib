/****************************************************************************
  Copyright (C), 2010, Dali Tech. Co. Ltd.

  Description:
            ¼����ģ��
  Histroy  :
            1)  Created by gongsuhui   2010/04/02

*****************************************************************************/
#ifndef __RECORDER_H__
#define __RECORDER_H__




#define   APP_RECORDER_MSG   ( APP_RECORDER << 16  )

#define   RECORDER_START_REC            ( APP_RECORDER_MSG + 0x01 )
#define   RECORDER_STOP_REC             ( APP_RECORDER_MSG + 0x02 )
#define   RECORDER_START_SNAP           ( APP_RECORDER_MSG + 0x03 )
#define   RECORDER_JPEG_IN              ( APP_RECORDER_MSG + 0x04 )
#define   RECORDER_UPDATE_PARAM         ( APP_RECORDER_MSG + 0x05 )
#define   RECORDER_TIMER_CHECK_STATUS   ( APP_RECORDER_MSG + 0x06 )
#define   RECORDER_RAW_IN               ( APP_RECORDER_MSG + 0X07 )
#define   RECORDER_AVSTREAM_IN          ( APP_RECORDER_MSG + 0x08 )

#define   RECORDER_FILE_FINISHED        ( APP_RECORDER_MSG + 0x10 )

#define   RECORDER_GET_REC_STATE        ( APP_RECORDER_MSG + 0x20 )
#define   RECORDER_GET_SD_STATE         ( APP_RECORDER_MSG + 0x21 )
#define   RECORDER_UPDATE_GPS_INFO      ( APP_RECORDER_MSG + 0x22 )

#define   RECORDER_TYPE_RAW        0
#define   RECORDER_TYPE_AVSTREAM   1
#define   RECORDER_TYPE_TEMP       2
#define   RECORDER_TYPE_JPEG       3

#define RECORDER_REC_STATE_IDLE       0
#define RECORDER_REC_STATE_REC        1

#define RECORDER_SD_STATE_NOTEXIST    0
#define RECORDER_SD_STATE_NORMAL      1
#define RECORDER_SD_STATE_FULL        2


#define RECORDER_EVENT_TYPE_KEY         0
#define RECORDER_EVENT_TYPE_MOTION      1
#define RECORDER_EVENT_TYPE_ALARM       2
#define RECORDER_EVENT_TYPE_REMOTE      3
#define RECORDER_EVENT_TYPE_SERIAL      4
#define RECORDER_EVENT_TYPE_AUTO        5


typedef  struct
{
    MESSAG_HEADER header ;
    int           rec_type     ;  // 0 : RAW ; 1 : JPG     
    int           event_type   ;  // ����¼������� 
    int           secs         ;  // ¼��ʱ����(<=0 ��ʾ����¼���Զ�ֹͣ)
    int           channel      ;  // ¼��ͨ����
    char          name[32]     ;  // �ļ���(��Ҫ��չ��)����еĻ�
}RECORDER_REC_MSG ;


typedef struct
{
    MESSAG_HEADER header;
    int           state ;
    int           used ;     // ʹ���˶��� �� M Ϊ��λ
    int           capacity ; // ���� �� M Ϊ��λ
}RECORDER_SD_STATE_MSG ;


typedef struct
{
    MESSAG_HEADER header ;
    int           state ;
}RECORDER_REC_STATE_MSG ;


typedef struct
{
    MESSAG_HEADER header    ;
    char          name[64]  ;  // �ļ���
}RECORDER_REC_FILE_MSG ;


typedef struct
{
    int  latitude     [3] ; //γ�ȣ��ȡ��֡���
    char latitude_ref     ; //"N" or "S"
    int  longitude    [3] ; //���ȣ��ȡ��֡��� 
    char longitude_ref    ; //"E" or "W"
    
    int  altitude  ;    //���� x10 ����0.1 M
    int  speed     ;    //�ٶ� x10 ����0.1 KM/H
    
    char date_time [20] ;  //GPS��ʱ��
}GPS_INFO ;

typedef struct
{
    MESSAG_HEADER   header ;
    GPS_INFO        gps    ;
}RECORDER_REC_GPS_MSG      ;



#endif

