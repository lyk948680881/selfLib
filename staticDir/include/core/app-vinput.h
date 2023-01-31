/****************************************************************************
  Copyright (C), 2011, Robert Fan

  Histroy  :
            1)  Created by fanshaohua   2011/08/10
            
  
*/

#ifndef __APP_VINPUT__
#define __APP_VINPUT__

#ifdef __cplusplus
    extern "C" {
#endif


#include "sys-hal.h"

#define  SYS_VINPUT                   3 
#define  SYS_DINPUT                   6        
#define  SYS_VINPUT_MSG              (SYS_VINPUT<<16)

#define  SYSVIN_TIMER_OUT              ( SYS_VINPUT_MSG + 0 )

#define  SYSVIN_UPDATE_REC_STATE       ( SYS_VINPUT_MSG + 14 )
#define  SYSVIN_UPDATE_ALARM_STATE     ( SYS_VINPUT_MSG + 15 )
#define  SYSVIN_UPDATE_SNAP_STATE      ( SYS_VINPUT_MSG + 16 )
#define  SYSVIN_UPDATE_SD_STATE        ( SYS_VINPUT_MSG + 17 )
#define  SYSVIN_UPDATE_MOTION_STATE    ( SYS_VINPUT_MSG + 18 )

#define  SYSVIN_SET_CROSS_OSD          ( SYS_VINPUT_MSG + 20 )
#define  SYSVIN_UPDATE_CROSS_OSD       ( SYS_VINPUT_MSG + 21 )
#define  SYSVIN_ENABLE_CROSS_OSD       ( SYS_VINPUT_MSG + 22 )
#define  SYSVIN_SET_CALIBRATE_CROSS_OSD        ( SYS_VINPUT_MSG + 23 )
#define  SYSVIN_SET_SHOOT_CROSS_OSD            ( SYS_VINPUT_MSG + 24 )
#define  SYSVIN_ENABLE_CALIBRATE_CROSS_OSD     ( SYS_VINPUT_MSG + 25 )
#define  SYSVIN_ENABLE_SHOOT_CROSS_OSD         ( SYS_VINPUT_MSG + 26 )

//新的SDK接口
#define  SYSVIN_SET_USER_GUI           ( SYS_VINPUT_MSG + 40 )
#define  SYSVIN_ENABLE_USER_GUI        ( SYS_VINPUT_MSG + 41 )
#define  SYSVIN_UPDATE_USER_GUI        ( SYS_VINPUT_MSG + 42 )
#define  SYSVIN_GET_USER_GUI           ( SYS_VINPUT_MSG + 43 )
#define  SYSVIN_FLUSH_USER_GUI         ( SYS_VINPUT_MSG + 44 )
#define  SYSVIN_DESTROY_USER_GUI       ( SYS_VINPUT_MSG + 45 )

#define  SYSVIN_SWITCH_VIDEO_VI        ( SYS_VINPUT_MSG + 50 )
#define  SYSVIN_SWITCH_VIDEO_IR        ( SYS_VINPUT_MSG + 51 )
#define  SYSVIN_SWITCH_VIDEO_VI_SMALL  ( SYS_VINPUT_MSG + 52 )
#define  SYSVIN_SWITCH_VIDEO_VI_LARGE  ( SYS_VINPUT_MSG + 53 )

#define  SYSVIN_REG_CB                 ( SYS_VINPUT_MSG + 80 )
#define  SYSVIN_UNREG_CB               ( SYS_VINPUT_MSG + 81 )
#define  SYSVIN_ADD_REF                ( SYS_VINPUT_MSG + 82 )
#define  SYSVIN_SUB_REF                ( SYS_VINPUT_MSG + 83 )
#define  SYSVIN_GET_VPSS_OUT_SIZE      ( SYS_VINPUT_MSG + 84 )
#define  SYSVIN_GET_TRACK_SIZE         ( SYS_VINPUT_MSG + 85 )
#define  SYSVIN_GET_DISPLAY_INFO       ( SYS_VINPUT_MSG + 86 )
#define  SYSVIN_GET_MERGE_SIZE         ( SYS_VINPUT_MSG + 87 )


#define  SYSVIN_SEND_FRAME_OUTPUT      ( SYS_VINPUT_MSG + 100 )


//内部宏
#define  SYSVIN_IR_PORT      0
#define  SYSVIN_VI_PORT      1
#define  SYSVIN_VO_PORT      0

#define  SYSVIN_VPSS_GRP_IR  SYSVIN_IR_PORT
#define  SYSVIN_VPSS_GRP_VI  SYSVIN_VI_PORT
#define  SYSVIN_VPSS_GRP_VO  4

#define  SYSVIN_VPSS_VI_CHN  2  //
#define  SYSVIN_VPSS_IR_CHN  1  //

#define  SYSVIN_VPSS_VO_CHN  0  

#define  SYSVIN_OSD_BASE     SYSVIN_VPSS_GRP_VO        //需要保存到码流中


#define  VIN_OSD_CROSS                 1      //十字光标
#define  VIN_OSD_CALIBRATE_CROSS       2
#define  VIN_OSD_SHOOT_CROSS           4 
#define  VIN_OSD_PRIVATEGUI            3
#define  VIN_MAX_OSD                   5      //osd叠加通道数，需要内部保存参数的

//以下都是无需保存参数在内部，使用SYSVIN_SET_USER_GUI接口
//可以使用SDK接口进行创建和控制
#define  VIN_OSD_USERGUI_EX1           11     
#define  VIN_OSD_USERGUI_EX2           12     
#define  VIN_OSD_USERGUI_EX3           13
#define  VIN_OSD_USERGUI_EX4           14
#define  VIN_OSD_USERGUI_EX5           15

///回调图像大小//////
#define  MAX_CALLBACK_WIDTH    416
#define  MAX_CALLBACK_HEIGHT   416 

#define  SYSVIN_SD_STATE_NULL          0
#define  SYSVIN_SD_STATE_NORMAL        1
#define  SYSVIN_SD_STATE_FULL          2

//显示模式
#define  SYSVIN_VIDEO_MODE_IR      0
#define  SYSVIN_VIDEO_MODE_VI      1

#define  SYSVIN_VPSS_IR_PORT_W      720
#define  SYSVIN_VPSS_IR_PORT_H      576
#define  SYSVIN_VPSS_VI_PORT_W      1920
#define  SYSVIN_VPSS_VI_PORT_H      1080

#define  CROSS_OSD_SIZE_HD    128
#define  CROSS_OSD_SIZE_SD    48


typedef enum
{
    IMX178_BW_MODE     = 0 ,
    YUV_COLOR_MODE         ,  // lvds sensor, sony 7710 etc.
    IMX178_COLOR_MODE      ,
    SC2310_COLOR_MODE      ,
    OV4689_COLOR_MODE      ,
}VINPUT_FORMAT ;

 

typedef struct{
    MESSAG_HEADER head   ;
    int           channel;
    int           enable ;
    int           x ;
    int           y ;
    int           width  ;
    int           height ;
    int           fcolor;
    int           bcolor;
    int           alpha ;
    char        * data  ;
}VINPUT_USER_GUI_MSG ;



typedef struct
{
    int        mode   ;
    int        x      ;
    int        y      ;
    int        width  ;
    int        height ;
}VINPUT_DISPLAY_INFO  ;



///////////////////////////////////////////////////////////////////
//回调出去的帧类型，注意使用方法
typedef struct _tagSYSVIN_FR
{
    //通过调整引用次数，来决定是否需要释放
    //以减少内存COPY次数
    int   reference            ;   //引用次数，为0时表示无人使用了    
    AV_STREAM_FRAME     frame  ;    
    void * private             ;
    struct _tagSYSVIN_FR * next;   //链表用
}SYSVIN_FRAME ;

typedef  int ( * VINPUT_CALLBACK ) ( SYSVIN_FRAME * fr ,  void * para ) ;
typedef struct _tagSYSVIN_CALLBACK
{
    int  channel ;                      //指定通道，-1 表示接受所有通道数据
    int  type    ;                      //类型 bit组合成
    void * param ;
    VINPUT_CALLBACK  callback ;
    struct _tagSYSVIN_CALLBACK * next ; //链表用
}SYSVIN_CALLBACK ;

//向Vinput注册一个指定类型type的回调函数，
//注册成功后返回系统唯一的非零正数callback_id，否则返回0
//注销回调函数时通过callback_id注销指定的回调函数
#define  SYSVIN_CALLBACK_REG(cb)    core_do_command( SYS_VINPUT , SYSVIN_REG_CB , sizeof(SYSVIN_CALLBACK) , (void*)cb , NULL ) 
#define  SYSVIN_CALLBACK_UNREG(id)  core_do_command( SYS_VINPUT , SYSVIN_UNREG_CB , 0 , (void*)id , NULL ) 

#define  SYSVIN_FR_ADDREF(fr)   core_do_command( SYS_VINPUT , SYSVIN_ADD_REF , sizeof(SYSVIN_FRAME) , fr , NULL ) 
#define  SYSVIN_FR_SUBREF(fr)   core_do_command( SYS_VINPUT , SYSVIN_SUB_REF , sizeof(SYSVIN_FRAME) , fr , NULL ) 



CORE_SERVICE * sysvin_service   ( void ) ;


#ifdef __cplusplus
    }
#endif

#endif

