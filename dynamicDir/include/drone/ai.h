#ifndef __AI_WORK_H____
#define __AI_WORK_H____

#define  APP_AI_MESSAGE    (APP_AI << 16)
#define  AI_SET_METHOD           (APP_AI_MESSAGE + 1)
#define  AI_SET_OBJECT_CLASS     (APP_AI_MESSAGE + 2)

#define  AI_STOP                 (APP_AI_MESSAGE + 10)
#define  AI_START                (APP_AI_MESSAGE + 11)
#define  AI_DETECT               (APP_AI_MESSAGE + 12)
#define  AI_START_TRACK_0        (APP_AI_MESSAGE + 14)
#define  AI_START_TRACK_1        (APP_AI_MESSAGE + 15)
#define  AI_STOP_TRACK           (APP_AI_MESSAGE + 16)
#define  AI_SWITCH_ZOOM          (APP_AI_MESSAGE + 17)
#define  AI_OBJ_SHOW 1
#define  AI_OBJ_HIDE 0

typedef struct
{
    MESSAG_HEADER  head  ;
    SYSVIN_FRAME * fr    ;
}AI_FRAME_MSG ;


typedef struct
{
    int x ;
    int y ;
    int width  ;
    int height ;
}AI_TRACKER_INFO   ;

int AI_stop ( void * priv );

#define AI_NONE               0x00
#define AI_FACE_DETECT        0x01
#define AI_CARPLATE_DETECT    0x02
#define AI_OBJECT_DETECT      0x03

// "Person" , "Bike", "Car" , "Motor", "Bus"   , "Truck", "Light" , "Sign" , "Other" ,
//    "Person" , "Bike", "Car" , "Motor", "Plane" , "Bus"  , "Truck" , "Boat" , "Light" , "Train"  , "Sign" ,
#define  OBJ_PERSON_IR      0
#define  OBJ_BIKE_IR        1
#define  OBJ_CAR_IR         2
#define  OBJ_MOTOR_IR       3
#define  OBJ_BUS_IR         4
#define  OBJ_TRUCK_IR       5
#define  OBJ_LIGHT_IR       6
#define  OBJ_SIGN_IR        7
#define  OBJ_OTHER_IR       8

#define  OBJ_PERSON_VI      9
#define  OBJ_BIKE_VI        10
#define  OBJ_CAR_VI         11
#define  OBJ_MOTOR_VI       12
#define  OBJ_PLANE_VI       13
#define  OBJ_BUS_VI         14
#define  OBJ_TRUCK_VI       15
#define  OBJ_BOAT_VI        16
#define  OBJ_LIGHT_VI       17
#define  OBJ_TRAIN_VI       18
#define  OBJ_SIGN_VI        19

#define  OBJ_ALL            20
#endif
