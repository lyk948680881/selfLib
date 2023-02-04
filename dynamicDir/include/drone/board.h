/****************************************************************

    单板平台相关的功能定义在这里
        1. GPIO
        2. RTC
        3. WATCHDOG
        4. VI/VO/AI/AO 配置

*****************************************************************/

#ifndef __BOARD_H__
#define __BOARD_H__

#define   APP_BOARD_MSG       ( APP_BOARD << 16 )

//Message for Board
#define   BOARD_SET_ALARM_PORT    ( APP_BOARD_MSG + 1 )
#define   BOARD_CLEAR_ALARM_PORT  ( APP_BOARD_MSG + 2 )
#define   BOARD_GET_ALARM_STATUS  ( APP_BOARD_MSG + 3 )

#define   BOARD_ENABLE_WATCHDOG   ( APP_BOARD_MSG + 5 )
#define   BOARD_DISABLE_WATCHDOG  ( APP_BOARD_MSG + 6 )
#define   BOARD_FEED_WATCHDOG     ( APP_BOARD_MSG + 7 )
#define   BOARD_RESET_SYSTEM      ( APP_BOARD_MSG + 8 )

#define   BOARD_GET_RTC_TIME      ( APP_BOARD_MSG + 10 )
#define   BOARD_SET_RTC_TIME      ( APP_BOARD_MSG + 11 )

#define   BOARD_SET_VIDEO_BRIGHTNESS (APP_BOARD_MSG + 12 )
#define   BOARD_SET_VIDEO_CONTRAST   (APP_BOARD_MSG + 13 )
#define   BOARD_SET_VIDEO_SATURATION (APP_BOARD_MSG + 14 )
#define   BOARD_SET_VIDEO_HUE        (APP_BOARD_MSG + 15 )
#define   BOARD_GET_KEYBOARD_INPUT   (APP_BOARD_MSG + 16 )
#define   BOARD_GET_HDMI_STATE       (APP_BOARD_MSG + 17 )
#define   BOARD_GET_RESET_STATUS     (APP_BOARD_MSG + 18 )
#define   BOARD_SWITCH_TO_VI         (APP_BOARD_MSG + 19 )
#define   BOARD_SWITCH_TO_IR         (APP_BOARD_MSG + 20 )
#define   BOARD_SWITCH_TO_VI_SMALL   (APP_BOARD_MSG + 21 )
#define   BOARD_SWITCH_TO_VI_LARGE   (APP_BOARD_MSG + 22 )
#define   BOARD_GET_SENSOR_SIZE      (APP_BOARD_MSG + 30 )

#define  BOARD_IR_PORT_WIDTH       720
#define  BOARD_IR_PORT_HEIGHT      576
#define  BOARD_IR_CROPPING_W       40
#define  BOARD_IR_CROPPING_H       32
#endif
