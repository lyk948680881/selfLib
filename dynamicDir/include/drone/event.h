/****************************************************************

    �¼�ģ��

*****************************************************************/

#ifndef __EVENT_H__
#define __EVENT_H__



#define   APP_EVENT_MSG       ( APP_EVENT << 16 )

//Message for Event
#define   EVENT_ADD_CLIENT        ( APP_EVENT_MSG + 0 )
#define   EVENT_DEL_CLIENT        ( APP_EVENT_MSG + 1 )
#define   EVENT_EXT_ALARM_INT     ( APP_EVENT_MSG + 2 )
#define   EVENT_SET_PORT_OUT      ( APP_EVENT_MSG + 3 )
#define   EVENT_MOTION_INT        ( APP_EVENT_MSG + 4 )
#define   EVENT_GRAY_FRAME_INT    ( APP_EVENT_MSG + 5 )
#define   EVENT_GET_ALARM_ITEM    ( APP_EVENT_MSG + 6 )
#define   EVENT_REPORT_ALARM_TARGET ( APP_EVENT_MSG + 7 )
#define   EVENT_USER_ALARM          ( APP_EVENT_MSG + 8 )
#define   EVENT_GRAY_ALARM_INT      ( APP_EVENT_MSG + 9 )




#define   EVENT_UPDATE_CONFIG     ( APP_EVENT_MSG + 10 )
#define   EVENT_UPDATE_RULES      ( APP_EVENT_MSG + 11 )

#define   EVENT_TIMER_OUT_1S       ( APP_EVENT_MSG + 0x40 )
#define   EVENT_TIMER_OUT_500MS    ( APP_EVENT_MSG + 0x41 )


#define  ALARM_ACTION_PORTOUT       0x0001
#define  ALARM_ACTION_EMAIL         0x0002
#define  ALARM_ACTION_SNAPSHOT      0x0004
#define  ALARM_ACTION_REC           0x0008

#define  MAX_ALARM_OUT        1
#define  MAX_ALARM_IN         1
#define  MAX_ALARM_RULE       8
#define  MAX_ALARM_TARGET     5

#define  ALARM_TYPE_PORT      0
#define  ALARM_TYPE_MOTION    1
#define  ALARM_TYPE_GRAY      2


typedef struct
{
    U32    port_in ;        //�澯����˿�
    U32    motion_state ;   //�ƶ����
}ALARM_STATUS ;

typedef struct 
{
    int enable;
    U8  start_hour ;
    U8  start_minute ;
    U8  end_hour ;
    U8  end_minute ;
}ALARM_TIME_PHASE;


typedef struct 
{
    int       enable ;
    ALARM_STATUS  or ;  // ֻҪ������һ�������������ھʹ����澯
    ALARM_STATUS  and;  // �����������ڲŴ���
    int       action     ; //SNAP REC EMAIL PORT
    
    int       port_out   ; //����Ķ˿ں�       
    int       alarm_time ; //�澯ʱ�� ��
    
    int       rec_channel ; //¼��ͨ��
    int       rec_time    ; //¼��ʱ�� �� 
    
    int       snap_type   ;  //����������ץͼ����
    int       snap_count  ;  //��������ʱץ����ͼ
    
    ALARM_TIME_PHASE       active_time1;
    ALARM_TIME_PHASE       active_time2;
}ALARM_RULE_CONFIG ;



//struct for EVENT report
typedef struct{    
    int    ip         ; //0 - ���� ,   ip = onvif context ���� network address
    int    port       ; //0 - ���� , port = -1 ��ʾONVIF�û�             
    
    int    readed     ; //�澯��ȡ��Ϣ     
    U32    timeout    ;    
}EVENT_SUBSCRIBE_INFO ;


typedef struct
{
    MESSAG_HEADER        head    ;
    int                  port    ;
    int                  flag    ;
}EVENT_ALARM_MSG ;

typedef struct 
{
    MESSAG_HEADER         head    ;
    int                   channel ;
    int                   state   ;
}EVENT_MOTION_MSG;  


typedef struct
{
    MESSAG_HEADER         head   ;
    int                   channel;
    int                   state  ;
    int                   max_gray ;
    int                   avg_gray ;
    int                   x;
    int                   y;
}EVENT_GRAY_MSG ;


typedef struct
{
    MESSAG_HEADER        head ;    
    EVENT_SUBSCRIBE_INFO info ;
}EVENT_SUBSCRIBE_MSG ;


typedef struct
{
    MESSAG_HEADER  head ; 
    int  context   ;
    int  max_count ;  
}ALARM_ITEM_REQUEST_MSG  ;


typedef struct
{
    U32    occur_time  ;
    short  alarm_type  ;
    short  alarm_state ; //true or false    
}ALARM_ITEM_INFO  ;



#endif
