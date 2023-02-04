#ifndef __DETECTOR_H__
#define __DETECTOR_H__


#define   APP_DETECTOR_MSG       ( APP_DETECTOR << 16 )

#define   DETECTOR_HEARTBEAT          (APP_DETECTOR_MSG + 1 )
#define   DETECTOR_CMD_KEY            (APP_DETECTOR_MSG + 2 )
#define   DETECTOR_CMD_FREEZE         (APP_DETECTOR_MSG + 3 )
#define   DETECTOR_CMD_ZOOM           (APP_DETECTOR_MSG + 4 )
#define   DETECTOR_SET_CONF           (APP_DETECTOR_MSG + 5 )
#define   DETECTOR_GET_CONF           (APP_DETECTOR_MSG + 6 )
#define   DETECTOR_SAVE_CONF          (APP_DETECTOR_MSG + 7 )
#define   DETECTOR_GET_VERSION        (APP_DETECTOR_MSG + 8 )
#define   DETECTOR_GET_SIZE           (APP_DETECTOR_MSG + 9 )

#define   DETECTOR_GET_STATISTICAL_INFO (APP_DETECTOR_MSG + 10 )
#define   DETECTOR_CTRL_LENS          (APP_DETECTOR_MSG + 11 )

#define   DETECTOR_FILL_CMD_PKT       (APP_DETECTOR_MSG + 12 )
#define   DETECTOR_CMD_NOACK          (APP_DETECTOR_MSG + 13 )
#define   DETECTOR_CMD_ACK            (APP_DETECTOR_MSG + 14 )
#define   DETECTOR_DATA_MSG           (APP_DETECTOR_MSG + 15 )

#define   DETECTOR_RESET_USB          (APP_DETECTOR_MSG + 16 )
#define   DETECTOR_START_USB          (APP_DETECTOR_MSG + 17 )
#define   DETECTOR_GET_MTLIB          (APP_DETECTOR_MSG + 18 )
#define   DETECTOR_REBOOT_CORE        (APP_DETECTOR_MSG + 20 )

#define   DETECTOR_CTRL_SHELL         (APP_DETECTOR_MSG + 30 )
#define   DETECTOR_SEND_SHELL_DATA    (APP_DETECTOR_MSG + 31 )
#define   DETECTOR_READ_SHELL_DATA    (APP_DETECTOR_MSG + 32 )

#define   DETECTOR_CONF_CMD_VIDEO_FORMAT          ( 0 )
#define   DETECTOR_CONF_CMD_OSD_STATE             ( 1 )
#define   DETECTOR_CONF_CMD_OSD_ALPHA             ( 2 )
#define   DETECTOR_CONF_CMD_COLORBAR_ID           ( 3 )
#define   DETECTOR_CONF_CMD_COLORBAR_POLARITY     ( 4 )
#define   DETECTOR_CONF_CMD_CHARGE_STATE          ( 5 )
#define   DETECTOR_CONF_CMD_CROSS_CURSOR_COLOR    ( 6 )
#define   DETECTOR_CONF_CMD_VFILTER               ( 7 )
#define   DETECTOR_CONF_CMD_BRIGHTNESS            ( 8 )
#define   DETECTOR_CONF_CMD_BOTTOM_COLOR          ( 9 )
#define   DETECTOR_CONF_CMD_UP_COLOR              ( 10 )
#define   DETECTOR_CONF_CMD_GRAY_FLOOR            ( 11 )
#define   DETECTOR_CONF_CMD_GRAY_CEILING          ( 12 )
#define   DETECTOR_CONF_CMD_AUTO_ADJUST_COLOR     ( 13 )
#define   DETECTOR_CONF_CMD_ENHANCEMENT           ( 14 )
#define   DETECTOR_CONF_CMD_SMOOTH_COEFFICIENT    ( 15 )
#define   DETECTOR_CONF_CMD_DETAIL_COEFFICIENT    ( 16 )
#define   DETECTOR_CONF_CMD_CONSTRAST             ( 17 )
#define   DETECTOR_CONF_CMD_IMAGE_FLIP            ( 18 )
#define   DETECTOR_CONF_CMD_OSD_FLIP              ( 19 )
#define   DETECTOR_CONF_CMD_FIR_FILTER            ( 20 )
#define   DETECTOR_CONF_CMD_MOTION_THRESHOLD      ( 21 )
#define   DETECTOR_CONF_CMD_SHOW_COLORBAR         ( 22 )
#define   DETECTOR_CONF_CMD_GRAY_ALARM            ( 23 )
#define   DETECTOR_CONF_CMD_LEARNING_TIME         ( 26 )
#define   DETECTOR_CONF_CMD_CROSS_CURSOR_ID       ( 30 )


typedef enum
{
    DK_UP = 0 ,
    DK_DOWN ,
    DK_RIGHT ,
    DK_LEFT ,
    DK_A ,
    DK_F ,
    DK_S ,
    DK_C ,
}DETECTOR_KEY_EN ;


typedef enum
{
    DMT_RAW         = 0,// 原始数据,分辨率为探测器大小
    DMT_YUV422,         // yuv422 数据 , 分辨率为720X576
    DMT_GRAY ,          // 灰度数据，分辨率为探测器分辨率
    DMT_YUV_RAW     = 5,        // 每16帧YUV422数据有一帧原始数据
    DMT_GRAY_RAW        // 每16帧灰度数据有一帧原始数据
}DETECTOR_MODE_TYPE_EN ;



typedef struct
{
    int     key         ;
    int     state ;    
}DETECTOR_KEY_CMD ;


typedef struct
{
    MESSAG_HEADER  head ;
    int freezed ;
}DETECTOR_FREEZE_MSG;

typedef struct
{
    int  factor ;
    int  x ;
    int  y ;
}DETECTOR_ZOOM_CMD;


typedef struct 
{
    int   id    ;
    int   value ;
}DETECTOR_CONF_CMD ;

typedef struct
{
    int video_fmt ;
    int osd_state;
    int osd_alpha ;
    int colorbar_id ;
    int colorbar_polarity ;
    int colorbar_bottom_color ;
    int colorbar_top_color ;
    int colorbar_threshold ;
    int colorbar_offset ;
    int gray_floor ;
    int gray_ceiling ;
    int auto_adjust_floor;
    int auto_adjust_ceiling;
    int auto_adjust_color ;
    int enhancement ;
    int smooth_coefficient;
    int detail_coefficient;
    int contrast ;
    int image_flip ;
    int osd_flip ;
    int fir_filter;
    int motion_threshold;
    int show_colorbar;
    int gray_alarm_threshold;
    int reserve1;
    int reserve2;
    int learning_time;
    int reserve3;
    int reserve4;
    int reserve5;
    int cross_cursor_id;
    int reserve6;
}DETECTOR_CONF ;


typedef struct
{
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
    int k ;                //  Q16精度
    int m ;                //  Q16精度
    int c ;                //  Q16精度
    int back ;             // 位段 0~3 low 4~7 high
}DETECTOR_STATISTICAL_INFO ;

typedef struct 
{
    int w ;
    int h ;
}DETECTOR_SIZE;

typedef struct 
{
    MESSAG_HEADER   head ;
    DETECTOR_CONF   conf ;
}DETECTOR_WHOLE_CONF_MSG ;


typedef struct 
{
    MESSAG_HEADER   head ;
    char ver[64] ;
}DETECTOR_VERSION_MSG ;

typedef enum
{
    DLC_STOP = 0,
    DLC_ZOOM ,
    DLC_FOCUS,
    DLC_AUTOFOCUS ,
    DLC_ZEROPRESET,
    DLC_SETPRESET,
    DLC_CALLPRESET,
    DLC_NULL,
}DETECTOR_LENS_CTRL_EN ;

typedef enum
{
    DZA_FAR = 0,
    DZA_NEAR ,
}DETECTOR_ZOOM_ACTION_EN ;

typedef struct 
{
    int type ;
    int action ;
}DETECTOR_CTRL_LENS_CMD ;


typedef struct
{
    int start;
    int fps  ;
    int mode ; // 0:raw 1:rgb 2:mix
}DETECTOR_USB_VI_CMD ;


typedef struct
{
    int cmd     ;
    int dat[4]  ;    
}DETECTOR_RAW_CMD ;

#endif
