#ifndef __TRACKER_H____
#define __TRACKER_H____


#define  APP_TRACKER_MESSAGE         (APP_TRACKER<<16)
#define  TRACKER_CONFIG_OBJ          (APP_TRACKER_MESSAGE+1)
#define  TRACKER_START_TRACK_0       (APP_TRACKER_MESSAGE+2)
#define  TRACKER_START_TRACK_1       (APP_TRACKER_MESSAGE+3)
#define  TRACKER_STOP_TRACK          (APP_TRACKER_MESSAGE+4)
#define  TRACKER_INFO_REPORT         (APP_TRACKER_MESSAGE+5)
#define  TRACKER_SET_SHAPE_SIZE      (APP_TRACKER_MESSAGE+6)

#define  TRACKER_GET_OBJECT_XY       (APP_TRACKER_MESSAGE+10)
#define  TRACKER_SET_OBJ_SIZE        (APP_TRACKER_MESSAGE+11)
#define  TRACKER_SET_OBJ_TYPE        (APP_TRACKER_MESSAGE+12)
#define  TRACKER_SET_OBJ_FRAME       (APP_TRACKER_MESSAGE+13)
#define  TRACKER_SET_BIN_LEVE        (APP_TRACKER_MESSAGE+14)
#define  TRACKER_SET_SIZE_LEVE       (APP_TRACKER_MESSAGE+15)
#define  TRACKER_SET_MAX_INFO        (APP_TRACKER_MESSAGE+16)

#define  TRACKER_BOX_ADD             (APP_TRACKER_MESSAGE+17)
#define  TRACKER_BOX_SUB             (APP_TRACKER_MESSAGE+18)
#define  TRACKER_BOX_MOVE_LEFT       (APP_TRACKER_MESSAGE+19)
#define  TRACKER_BOX_MOVE_RIGHT      (APP_TRACKER_MESSAGE+20)
#define  TRACKER_BOX_MOVE_UP         (APP_TRACKER_MESSAGE+21)
#define  TRACKER_BOX_MOVE_DOWN       (APP_TRACKER_MESSAGE+22)
#define  TRACKER_SWITCH_ZOOM         (APP_TRACKER_MESSAGE+23)

typedef struct
{
    MESSAG_HEADER header ;
    int   shape_size_x ;    
    int   shape_size_y ;   // 0 ~ 16  
}TRACKER_OBJ_CONFIG_MSG ;


typedef struct
{
    MESSAG_HEADER header ;
    int  x ;
    int  y ;
    int  w ;
    int  h ;
    int frame_id;
    float  zoom ;
}TRACKER_OBJ_OPERATION_MSG ;

typedef struct
{
    MESSAG_HEADER header ;
    int enable ;
    int x ;
    int y ;
    int width ;
    int height ;
}TRACKER_OBJ_CONTROL_MESSAGE ;

typedef struct
{
    int  state ;
    int  x ;
    int  y ;
    int  w ;
    int  h ;    
}TRACKER_OBJ_REPORT ;

#define TRACK_MODE_AI_ENABLE  1

#endif
