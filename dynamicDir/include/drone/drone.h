/****************************************************************************
  Copyright (C), 2011, Jacky

  Histroy    :
              1)  Created by Jacky   2021/01/20
  Description: 
****************************************************************************/

#ifndef __DRONE_H__
#define __DRONE_H__


#define   APP_DRONE_MSG          ( APP_DRONE << 16 )
#define   DRONE_FRAME_H264       ( APP_DRONE_MSG + 1 )
#define   DRONE_FRAME_JPEG       ( APP_DRONE_MSG + 2 )
#define   DRONE_UPDATE_STATE     ( APP_DRONE_MSG + 3 )
#define   DRONE_SEND_MODE        ( APP_DRONE_MSG + 4 )

#define   DRONE_FONT_SHOW        ( APP_DRONE_MSG + 5 )
#define   DRONE_IR_WHITE_HEAT    ( APP_DRONE_MSG + 6 )
#define   DRONE_IR_BLACK_HEAT    ( APP_DRONE_MSG + 7 )
#define   DRONE_ZOOM_SET         ( APP_DRONE_MSG + 8 )
#define   DRONE_BRIGHT_ADJUST    ( APP_DRONE_MSG + 9 )
#define   DRONE_CONTRAST_ADJUST  ( APP_DRONE_MSG + 10 )
#define   DRONE_ADJUST_AUTO      ( APP_DRONE_MSG + 11 )
#define   DRONE_MODE_SWITCH      ( APP_DRONE_MSG + 12 )
#define   DRONE_MODE_GET         ( APP_DRONE_MSG + 13 )
#define   DRONE_RECORDER_START   ( APP_DRONE_MSG + 14 )
#define   DRONE_RECORDER_STOP    ( APP_DRONE_MSG + 15 )
#define   DRONE_TAKE_PHOTO       ( APP_DRONE_MSG + 16 )
#define   DRONE_START_SERIES_PIC ( APP_DRONE_MSG + 17 )
#define   DRONE_STOP_SERIES_PIC  ( APP_DRONE_MSG + 18 )
#define   DRONE_TIME_SERIES_PIC  ( APP_DRONE_MSG + 19 )
#define   DRONE_WORK_MODE_SWITCH ( APP_DRONE_MSG + 20 )
#define   DRONE_FILES_LIST       ( APP_DRONE_MSG + 21 )
#define   DRONE_FILES_EXPORT     ( APP_DRONE_MSG + 22 )
#define   DRONE_FILE_DELETE      ( APP_DRONE_MSG + 23 )
#define   DRONE_FILES_DELETE     ( APP_DRONE_MSG + 24 )
#define   DRONE_BOARD_INFO       ( APP_DRONE_MSG + 25 )
#define   DRONE_DOME_INFO        ( APP_DRONE_MSG + 26 )
#define   DRONE_STOP_TRACK       ( APP_DRONE_MSG + 27 )
#define   DRONE_LASER_TURN_ON    ( APP_DRONE_MSG + 28 )
#define   DRONE_LASER_TURN_OFF   ( APP_DRONE_MSG + 29 )
#define   DRONE_MODE_SWITCH_TO_VI   ( APP_DRONE_MSG + 30 )
#define   DRONE_MODE_SWITCH_TO_IR   ( APP_DRONE_MSG + 31 )
#define   DRONE_AI_SET_CMD          ( APP_DRONE_MSG + 32 )
#define   DRONE_SET_AI_METHOD       ( APP_DRONE_MSG + 33 )
#define   DRONE_DOME_TEXT_CMD       ( APP_DRONE_MSG + 34 )
#define   DRONE_CROSS_LEFT          ( APP_DRONE_MSG + 35 )
#define   DRONE_CROSS_RIGHT         ( APP_DRONE_MSG + 36 )
#define   DRONE_CROSS_UP            ( APP_DRONE_MSG + 37 )
#define   DRONE_CROSS_DOWN          ( APP_DRONE_MSG + 38 )
#define   DRONE_CROSS_SAVE          ( APP_DRONE_MSG + 39 )


#define   DRONE_TIME_FC_ACK         ( APP_DRONE_MSG + 200 )
#define   DRONE_DOME_ACK             ( APP_DRONE_MSG + 221 )
#define   DRONE_DOME_HAND_SPEED      ( APP_DRONE_MSG + 222 )
#define   DRONE_DOME_SCAN_MODE       ( APP_DRONE_MSG + 223 )
#define   DRONE_DOME_CENTRA_BACK     ( APP_DRONE_MSG + 224 )
#define   DRONE_DOME_VERTICAL_DOWN   ( APP_DRONE_MSG + 225 )
#define   DRONE_DOME_DATA_LEAD       ( APP_DRONE_MSG + 226 )
#define   DRONE_DOME_GEOGROPY_LEAD   ( APP_DRONE_MSG + 227 )
#define   DRONE_DOME_PROTECT_MODE    ( APP_DRONE_MSG + 228 )
#define   DRONE_DOME_HAND_TRACE      ( APP_DRONE_MSG + 229 )
#define   DRONE_DOME_AUTO_TRACK_MODE ( APP_DRONE_MSG + 230 )
#define   DRONE_DOME_TARGET_LOST     ( APP_DRONE_MSG + 231 )

#define   DRONE_DOME_MOTOR_TURN_ON   ( APP_DRONE_MSG + 233 )
#define   DRONE_DOME_MOTOR_TURN_OFF  ( APP_DRONE_MSG + 234 )

#define   DRONE_DOME_SUPPRESS_GYRO_DIRFT  ( APP_DRONE_MSG + 235 )
#define   DRONE_DOME_ELECTRIC_TRUN_ON     ( APP_DRONE_MSG + 236 )
#define   DRONE_DOME_ELECTRIC_TRUN_OFF    ( APP_DRONE_MSG + 237 )
#define   DRONE_DOME_TRACKER_TRUN_ON      ( APP_DRONE_MSG + 238 )
#define   DRONE_DOME_TRACKER_TRUN_OFF     ( APP_DRONE_MSG + 239 )
#define   DRONE_DOME_STOP_TRACK_MODE      ( APP_DRONE_MSG + 240 )
#define   DRONE_DOME_LASER_TURN_ON        ( APP_DRONE_MSG + 241 )
#define   DRONE_DOME_LASER_TURN_OFF       ( APP_DRONE_MSG + 242 )


#define   DRONE_DOME_MAX_CMD         ( APP_DRONE_MSG + 260 )

#define   DRONE_GET_FLIGHT_DATA     ( APP_DRONE_MSG + 100 )
#define   DRONE_REPORT_TRACK_INFO   ( APP_DRONE_MSG + 101 )

#define   DRONE_MODE_VIDEO_VI       0
#define   DRONE_MODE_VIDEO_IR       1

#define   DRONE_LIVE_VIEW_MODE       0
#define   DRONE_FIELS_TRANSFER_MODE  1

#define   FC_TASK_WORKING         0xD1
#define   FC_TASK_SLEEPING        0xD2


#define   FC_ZOOM_INCREASE        0xCA

#define   FC_AUTO_ZERO            0x73
#define   FC_BRIGHT_INCREASE      0x77
#define   FC_BRIGHT_DECREASE      0x79
#define   FC_CONTRAST_INCRESE     0x7B
#define   FC_CONTRAST_DECRESE     0x7D
#define   FC_CORRECT_CONTROL      0x22
//#define   FC_TAKE_PICTURE         0x55
//#define   FC_START_REC            0xAB
//#define   FC_STOP_REC             0xAA
#define   FC_START_SERIES_REC     0x60
#define   FC_STOP_SERIES_REC      0x72
#define   FC_DIGI_ZOOM_1X         0x87
#define   FC_DIGI_ZOOM_2X         0x88
#define   FC_DIGI_ZOOM_3X         0x89
#define   FC_DIGI_ZOOM_4X         0x8A
#define   FC_DIGI_ZOOM_5X         0x8B
#define   FC_FONT_SHOW            0x93
#define   FC_VIDEO_MODE_2M        0x81
#define   FC_VIDEO_MODE_4M        0x82
#define   FC_VIDEO_MODE_8M        0x83
#define   FC_VIDEO_MODE_12M       0x84

#define   FC_ID_SET               0x9C
#define   FC_DEBUG_CONTROL_CMD    0xFE




#define   FC_DOME_HAND_POSITION_MODE         0x18

#define   FC_DOEM_SCAN_MODE                  0xC9

#define   FC_DOME_BIRD_VIEW_MODE             0xf1//0x15

#define   FC_DOME_GEOGRAPY_MODE              0x20
#define   FC_DOME_PROTECT_MODE               0xD3



#define   FC_VIDEO_SWITCH_VI                 0x01
#define   FC_VIDEO_SWITCH_IR                 0x02
#define   FC_POLARITY_BLACK                  0x03
#define   FC_POLARITY_WHITE                  0x04
#define   FC_IMAGE_ENHANCE_TURN_ON           0x05
#define   FC_IMAGE_ENHANCE_TURN_OFF          0x06
#define   FC_AI_TURN_ON                      0X07
#define   FC_AI_TURN_OFF                     0X08
#define   FC_START_REC                       0x09
#define   FC_STOP_REC                        0x0A

#define   FC_DOME_AUTO_TRACK                 0x0D
#define   FC_DOME_STOP_TRACK                 0x0E

#define   FC_AI_SELF_ADAPTION                0x13
#define   FC_AI_PEOPLE                       0X14
#define   FC_AI_CAR                          0X15

#define   FC_DOME_ZOOM                       0X23
#define   FC_DOME_HAND_ANGLE_MODE            0x24
#define   FC_DOME_ZOOM_RATE                  0x25
#define   FC_DOME_DATA_LEAD_MODE             0x26
#define   FC_MOTOR_TRUN_ON                   0x27
#define   FC_MOTOR_TRUN_OFF                  0x28
#define   FC_TRACKER_TRUN_ON                 0x29
#define   FC_TRACKER_TRUN_OFF                0x2a
#define   FC_DOME_CENTRE_MODE                0x2b
#define   FC_DOME_SUPPESS_GYRO_DRIFT         0x2c
#define   FC_LASER_TURN_ON                   0x2d
#define   FC_LASER_TURN_OFF                  0x2e
#define   FC_ELECTRIC_TURN_ON                0x30
#define   FC_ELECTRIC_TURN_OFF               0x31

#define   FC_GUN_AIM_FOLLOW_OPEN             0x41
#define   FC_GUN_AIM_FOLLOW_CLOSE            0x42

#define   FC_TRACKER_BOX_ADD                 0x51
#define   FC_TRACKER_BOX_SUB                 0x52
#define   FC_TRACKER_BOX_MANUAL_SET          0x53
#define   FC_TRACKER_BOX_MOVE_LEFT           0x54
#define   FC_TRACKER_BOX_MOVE_RIGHT          0x55
#define   FC_TRACKER_BOX_MOVE_UP             0x56
#define   FC_TRACKER_BOX_MOVE_DOWN           0x57

#define   FC_CROSS_MOVE_LEFT                 0x61
#define   FC_CROSS_MOVE_RIGHT                0x62
#define   FC_CROSS_MOVE_UP                   0x63
#define   FC_CROSS_MOVE_DOWN                 0x64
#define   FC_CROSS_SAVE                      0x65

#define   FC_IR_IMAGE_ADJUST                 0x71
    
#define   FC_NULL_CMD                        0xFF


#endif
