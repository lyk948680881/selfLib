/****************************************************************************
    Shanghai Pallas Digital Tech. Ltd,  (C) 2002 , All rights are reserved
    Description:
        存放ui和cgi程序通用宏定义
****************************************************************************/

#ifndef _CAMERA_HEADER_
#define _CAMERA_HEADER_

#define  APP_UI_MSG           ( APP_CAMERA  << 16 )


//Message for UI & Camera
#define   UI_SYSTEM_REBOOT_MSG           ( APP_UI_MSG + 0x01 )
#define   UI_START_RECORD_MSG            ( APP_UI_MSG + 0x02 )
#define   UI_STOP_RECORD_MSG             ( APP_UI_MSG + 0x03 )
#define   UI_SNAP_JPEG_MSG               ( APP_UI_MSG + 0x04 )
#define   UI_NTP_UPDATE_MSG              ( APP_UI_MSG + 0x05 )
#define   UI_ADD_ALARM_CLIENT_MSG        ( APP_UI_MSG + 0x06 )
#define   UI_DEL_ALARM_CLIENT_MSG        ( APP_UI_MSG + 0x07 )
#define   UI_RESTORE_DEFAULT_MSG         ( APP_UI_MSG + 0x08 )
#define   UI_CTRL_ALARM_OUT_MSG          ( APP_UI_MSG + 0x09 )
#define   UI_CTRL_PTZ_MSG                ( APP_UI_MSG + 0x0A )
#define   UI_QUERY_STATUS_MSG            ( APP_UI_MSG + 0x0B )
#define   UI_REQUEST_ID_MSG              ( APP_UI_MSG + 0x0C )
#define   UI_SYSTEM_LOG_MSG              ( APP_UI_MSG + 0x0D )
#define   UI_KILL_USER_MSG               ( APP_UI_MSG + 0x0F )
#define   UI_SET_DATE_MSG                ( APP_UI_MSG + 0x10 )
#define   UI_SERACH_LOG_MSG              ( APP_UI_MSG + 0x11 )
#define   UI_SERACH_DATA_MSG             ( APP_UI_MSG + 0x12 )
#define   UI_BLOCK_DISK_MSG              ( APP_UI_MSG + 0x13 )
#define   UI_UNBLOCK_DISK_MSG            ( APP_UI_MSG + 0x14 )
#define   UI_GET_SYSTEM_VER_MSG          ( APP_UI_MSG + 0x15 )
#define   UI_SYSTEM_REBOOT_TEMPCORE      ( APP_UI_MSG + 0x16 )

#define   UI_SET_GB28181_PARA            ( APP_UI_MSG + 0x17 )

#define   UI_SET_LANUAGE_PARA              ( APP_UI_MSG + 0x20 )
#define   UI_SET_ALARM_IN_PARA             ( APP_UI_MSG + 0x21 )
#define   UI_SET_SENSOR_PARA               ( APP_UI_MSG + 0x22 )
#define   UI_SET_VIDEO_ENC_PARA            ( APP_UI_MSG + 0x23 )
#define   UI_SET_JPEG_ENC_PARA             ( APP_UI_MSG + 0x24 )
#define   UI_SET_VIDEO_MASK_PARA           ( APP_UI_MSG + 0x25 )
#define   UI_SET_EVENT_PARA                ( APP_UI_MSG + 0x26 )
#define   UI_SET_RULES_PARA                ( APP_UI_MSG + 0x27 )
#define   UI_SET_NETWORK_PARA              ( APP_UI_MSG + 0x28 )
#define   UI_SET_NAME_OSD_PARA             ( APP_UI_MSG + 0x29 )
#define   UI_SET_DATE_OSD_PARA             ( APP_UI_MSG + 0x2A )
#define   UI_SET_RECODRER_PARA             ( APP_UI_MSG + 0x2B )
#define   UI_SET_JPEG_SNAP_PARA            ( APP_UI_MSG + 0x2C )
#define   UI_SET_DDNS_PARA                 ( APP_UI_MSG + 0x2D )
#define   UI_SET_DOME_PARA                 ( APP_UI_MSG + 0x2E )
#define   UI_SET_SMTP_PARA                 ( APP_UI_MSG + 0x2F )
#define   UI_SET_FTP_PARA                  ( APP_UI_MSG + 0x30 )
#define   UI_SET_NTP_PARA                  ( APP_UI_MSG + 0x31 )
#define   UI_SET_AUTO_REBOOT_PARA          ( APP_UI_MSG + 0x32 )
#define   UI_SET_OVERRIDE_PARA             ( APP_UI_MSG + 0x33 )
#define   UI_SET_VIDEO_OUT                 ( APP_UI_MSG + 0x34 )
#define   UI_SET_IR_MASK_PARA              ( APP_UI_MSG + 0x35 )
#define   UI_SET_STORAGE_PARA              ( APP_UI_MSG + 0x36 )
#define   UI_SET_CORRECT_TEMP_PARA         ( APP_UI_MSG + 0x37 )
#define   UI_SET_TEMP_OSD_PARA             ( APP_UI_MSG + 0x38 )
#define   UI_SET_COMMON_TEMP_PARA          ( APP_UI_MSG + 0x39 )
#define   UI_SET_INTELLIGENCE_PARA         ( APP_UI_MSG + 0x3A )
#define   UI_SET_TEMP_OBJECT_PARA          ( APP_UI_MSG + 0x3B )
#define   UI_SET_LOGO_OSD_PARA             ( APP_UI_MSG + 0x3C )
#define   UI_SET_MOTION_PARA               ( APP_UI_MSG + 0x3D )
#define   UI_SET_GRAY_ALARM_PARA           ( APP_UI_MSG + 0x3E )
#define   UI_SET_IRDATA_VIDEO_ENC_PARA     ( APP_UI_MSG + 0x3F )

#define   UI_CTRL_IMAGE_FOCUS              ( APP_UI_MSG + 0x40 )
#define   UI_CTRL_IMAGE_FOCUS_SPEED        ( APP_UI_MSG + 0x41 )
#define   UI_CTRL_IMAGE_BRIGHTNESS         ( APP_UI_MSG + 0x42 )
#define   UI_CTRL_IMAGE_CONTRAST           ( APP_UI_MSG + 0x43 )
#define   UI_CTRL_IMAGE_SATURATION         ( APP_UI_MSG + 0x44 )
#define   UI_CTRL_IMAGE_PALLETTE           ( APP_UI_MSG + 0x45 )
#define   UI_CTRL_IMAGE_PALLETTEPOLARITY   ( APP_UI_MSG + 0x46 )
#define   UI_CTRL_IMAGE_UPER_BOUND         ( APP_UI_MSG + 0x47 )
#define   UI_CTRL_IMAGE_LOWER_BOUND        ( APP_UI_MSG + 0x48 )
#define   UI_CTRL_IMAGE_ISOTHERM_COLOR     ( APP_UI_MSG + 0x49 )
#define   UI_CTRL_IMAGE_ISOTHERM_TEMP      ( APP_UI_MSG + 0x4A )



#define   UI_CTRL_IRIMAGE_GET_VERSION      ( APP_UI_MSG + 0x50 )
#define   UI_CTRL_IRIMAGE_KEY_CMD          ( APP_UI_MSG + 0x51 )
#define   UI_CTRL_IRIMAGE_GET_CONF         ( APP_UI_MSG + 0x52 )
#define   UI_CTRL_IRIMAGE_SET_CONF         ( APP_UI_MSG + 0x53 )
#define   UI_CTRL_IRIMAGE_SAVE_CONF        ( APP_UI_MSG + 0x54 )
#define   UI_CTRL_IRIMAGE_ZOOM             ( APP_UI_MSG + 0x55 )
#define   UI_CTRL_IRIMAGE_FREEZE           ( APP_UI_MSG + 0x56 )
#define   UI_CTRL_IRIMAGE_CTRL_LENS        ( APP_UI_MSG + 0x57 )
#define   UI_CTRL_IRIMAGE_GET_STATIC_INFO  ( APP_UI_MSG + 0x58 )



#define   UI_SET_CUSTOM_CFGFILE            ( APP_UI_MSG + 0x60 )
#define   UI_GET_CUSTOM_CFGFILE            ( APP_UI_MSG + 0x61 )

#define   UI_CTRL_GUI_DISPLAY              ( APP_UI_MSG + 0x62 )

#define   UI_EVENT_GET_ALARM               ( APP_UI_MSG + 0x64 )

#define   UI_EVENT_ALARM_IN                ( APP_UI_MSG + 0x100 )
#define   UI_EVENT_MOTION_DETECTIVE        ( APP_UI_MSG + 0x101 )
#define   UI_EVENT_HIGHEST_GRAY            ( APP_UI_MSG + 0x102 )
#define   UI_EVENT_ALARM_TARGETS           ( APP_UI_MSG + 0x103 )
#define   UI_EVENT_ALARM_CUSTOM            ( APP_UI_MSG + 0x104 )


#define   UI_SYSTEM_ACCEPT_OK              ( APP_UI_MSG + 0x200 )
#define   UI_SYSTEM_ACCEPT_FAIL            ( APP_UI_MSG + 0x201 )
#define   UI_SYSTEM_LIST_MODULE            ( APP_UI_MSG + 0x202 )
#define   UI_SYSTEM_DEBUG_MODULE           ( APP_UI_MSG + 0x203 )
#define   UI_SYSTEM_LIST_THREAD            ( APP_UI_MSG + 0x204 )


#define   CAMERA_SET_CONFIG                ( APP_UI_MSG + 0x500 )
#define   CAMERA_RESTORE_DEFAULT           ( APP_UI_MSG + 0x501 )
#define   CAMERA_SHOW_CONFIG_TIMEOUT       ( APP_UI_MSG + 0x502 )
#define   CAMERA_MTPARAM_TIME_OUT          ( APP_UI_MSG + 0X503 )
#define   CAMERA_TIMER_OUT_1S              ( APP_UI_MSG + 0x510 )
#define   CAMERA_UPDATE_TEMP               ( APP_UI_MSG + 0x511 )
#define   CAMERA_GET_DETECTOR_STATS        ( APP_UI_MSG + 0x512 )
#define   CAMERA_CHANGE_KEY_RECEIVER       ( APP_UI_MSG + 0x513 )



//CGI 通用的消息类型
typedef struct
{    
    int  command  ;      //命令字
    int  channel  ;    
    int  data[0]  ;      //不占空间，最大128个    
}UI_CMD_MESSAGE   ;

typedef struct
{
    int command ;
    int dome_cmd ;
    int data ;
}UI_DOME_MESSAGE ;

//告警客户端
typedef struct
{
    int    ip   ;
    int    port ; 
    int    data    ;
    int    timeout ;   
}UI_ALARM_CLIENT ;

//设置告警输出端口
typedef struct
{
    int port  ;
    int state ;
}UI_ALARM_PORT ;

typedef struct
{
    int     protocol;  //不用
    int     addr    ;  //不用
    int     sub_cmd ;  //不用
    int     data    ;    
}UI_DOME_PTZ ;

typedef struct
{
    int command ;
    char ver[0] ;
}UI_IRIMAGE_VERSION_MESSAGE ;

typedef struct
{
    int command ;
    int key ;
    int state ;
}UI_IRIMAGE_KEY_MESSAGE ;

typedef struct
{
    int command ;
    int factor ;   //f * 100 
    int x ;
    int y ;
}UI_IRIMAGE_ZOOM_MESSAGE ;

typedef struct
{
    int command ;
    int freeze ;
}UI_IRIMAGE_FREEZE_MESSAGE ;

typedef struct
{
    int command ;
    int ctrl_type ;
    int action ;
}UI_IRIMAGE_CTRLENS_MESSAGE ;

typedef struct
{
    int command ;
    int conf_id ;
    int conf_value ;
}UI_IRIMAGE_CONF_MESSAGE ;

typedef struct
{
    int command ;
    int channel ;
    DETECTOR_CONF conf;
}UI_IRIMAGE_GET_CONF_MESSAGE ;



#define USER_CFGFILE_MAGIC_START     0x55aaaa55
#define USER_CFGFILE_MAGIC_END       0xaa5555aa
typedef struct 
{
    int command ;
    int data_length;
    char dat[0] ;
}UI_USER_CFGFILE_MESSAGE;


typedef struct
{
    int command ;
    int min_gray ;
    int average_gray ;
    int max_gray ;
    short clip_y;
    short clip_x;
    short clip_h ;
    short clip_w ;
    int vtemp ;
    int highest_temp ;     //最高温度X10
    short ht_x ;           //最高温度x值
    short ht_y ;           //最高温度y值
    int lowest_temp ;      //最低温度X10
    short lt_x ;           //最低温度x值
    short lt_y ;           //最低温度y值
    int center_temp ;      //中心点温度X10
    int avg_temp ;         //平均温度X10
}UI_IRIMAGE_GET_STAT_INFO ;


typedef struct
{
    int command       ;
    int operation     ;    
    int color         ;
       
    int   x           ;
    int   y           ;    
    int   width       ;
    int   height      ;
    
    char  data[80]    ;
}UI_CONTROL_GUI_MESSAGE ;

typedef struct{
    int command       ;
    int context       ;
    int count         ;
    char data[0]      ;
}UI_GET_ALARM_MESSAGE ;


#define  QUERY_ONLINE_USER  0
#define  QUERY_RECORDER     1
#define  QUERY_NETWORK      2

//搜索记录：LOG/JPEG/H264
typedef struct
{
    int      begin     ;
    int      end       ;
    int      type      ;    
    int      max       ;     //输入：最大条数 输出：实际条数
    int      start     ;     //查找开始位置，-1 表示第一次开始查找。
                             //重复调用，可以找出所有符合要求的                            
}UI_DATA_SEARCH ;

#endif

