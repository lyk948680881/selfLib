
#ifndef _APP_MESSAGE_H_
#define _APP_MESSAGE_H_

#define  APP_KERNEL_MSG     0x0 
#define  APP_VTEMP_CHECK_MSG       ( APP_KERNEL_MSG + 2 ) 
#define  APP_TIMER_500MS_MSG       ( APP_KERNEL_MSG + 3 ) 
#define  APP_TIMER_100MS_MSG       ( APP_KERNEL_MSG + 4 ) 
#define  APP_TIMER_50MS_MSG        ( APP_KERNEL_MSG + 5 ) 
#define  KEYBOARD_REPEAT_TIMER     ( APP_KERNEL_MSG + 6 )
#define  APP_SHOW_CONFIG_OVER_MSG  ( APP_KERNEL_MSG + 7 )
#define  APP_UPDATE_SNAP_STATE_MSG ( APP_KERNEL_MSG + 8 )
#define  APP_UPDATE_REC_STATE_MSG  ( APP_KERNEL_MSG + 9 )
#define  APP_UPDATE_SD_STATE_MSG   ( APP_KERNEL_MSG + 10)

//MENU消息定义，可用消息为512个 
#define  APP_GUI_MSG        0x1000
#define  GUI_GET_FOCUS      (0x200)  //MASK
#define  GUI_LOST_FOCUS     (0x400)  //MASK

#define  GUI_SHOW           (APP_GUI_MSG + 0)
#define  GUI_HIDE           (APP_GUI_MSG + 1)
#define  GUI_SWITCH_PAGE    (APP_GUI_MSG + 2)
#define  GUI_STAT_UPDATE_TIMER  (APP_GUI_MSG + 3)

#define  GUI_VFMT_PAL       (APP_GUI_MSG + 10)
#define  GUI_VFMT_NTSC      (APP_GUI_MSG + 11)
#define  GUI_VFMT_UVC       (APP_GUI_MSG + 12)

#define  GUI_COLORBAR_INDEX    (APP_GUI_MSG + 20)
#define  GUI_COLORBAR_REVERSE  (APP_GUI_MSG + 21)

#define  GUI_OSD_CONTROL       (APP_GUI_MSG + 30)
#define  GUI_OSD_ALPHA         (APP_GUI_MSG + 31)
#define  GUI_OSD_FLIP          (APP_GUI_MSG + 32)

#define  GUI_OSD_CURSOR_ID     (APP_GUI_MSG + 35)
#define  GUI_OSD_CURSOR_DX     (APP_GUI_MSG + 36)
#define  GUI_OSD_CURSOR_DY     (APP_GUI_MSG + 37)

#define  GUI_COLOR_MODE        (APP_GUI_MSG + 40)
#define  GUI_COLOR_GRAY_LOW    (APP_GUI_MSG + 41)
#define  GUI_COLOR_GRAY_HIGH   (APP_GUI_MSG + 42)
#define  GUI_COLOR_ROI_X1      (APP_GUI_MSG + 43)
#define  GUI_COLOR_ROI_Y1      (APP_GUI_MSG + 44)
#define  GUI_COLOR_ROI_X2      (APP_GUI_MSG + 45)
#define  GUI_COLOR_ROI_Y2      (APP_GUI_MSG + 46)
#define  GUI_COLOR_GRAY_SEG3   (APP_GUI_MSG + 47)

#define  GUI_COLOR_MAP_LOW     (APP_GUI_MSG + 50) 
#define  GUI_COLOR_MAP_HIGH    (APP_GUI_MSG + 51)
#define  GUI_COLOR_CONTRAST    (APP_GUI_MSG + 52)
#define  GUI_COLOR_BRIGHT      (APP_GUI_MSG + 53)

#define  GUI_IMAGE_VFILTER     (APP_GUI_MSG + 55)
#define  GUI_IMAGE_TFILTER     (APP_GUI_MSG + 56)
#define  GUI_IMAGE_FLIP        (APP_GUI_MSG + 57)
#define  GUI_IMAGE_UPDATE      (APP_GUI_MSG + 58)

#define  GUI_IMAGE_ENHANCE     (APP_GUI_MSG + 60)
#define  GUI_IMAGE_ENHANCE_DETAIL  (APP_GUI_MSG + 61)
#define  GUI_IMAGE_ENHANCE_LEVEL   (APP_GUI_MSG + 62)

#define  GUI_ALARM_MOTION          (APP_GUI_MSG + 65)

#define  GUI_GRAY_ALARM_TARGET     (APP_GUI_MSG + 66)
#define  GUI_CLEAR_GRAY_ALARM      (APP_GUI_MSG + 67)

#define  GUI_CONFIG_SAVE       (APP_GUI_MSG + 70)
#define  GUI_CONFIG_CLOSE       (APP_GUI_MSG + 71)


//坏点界面消息定义
#define  APP_BP_MSG         0x2000
#define  BP_GUI_ACTIVE      (APP_BP_MSG + 0)

#define  BP_SAVE_MSG        (APP_BP_MSG + 100)
#define  BP_EXIT_MSG        (APP_BP_MSG + 101)
#define  BP_APPLY_MSG       (APP_BP_MSG + 102)
#define  BP_SHOW_HIDE_MSG   (APP_BP_MSG + 103)


//消息体定义 48 Bytes
typedef struct
{
    int  sender   ;      //发送模块
    int  receiver ;      //接受模块
    int  command  ;      //命令字
    int  length   ;      //内容长度
    int  data     ;
    void * pdata  ;    
}APP_MESSAGE ;


typedef struct 
{
    int key ;
    int state ;
}APP_UI_KEY   ;



#endif

