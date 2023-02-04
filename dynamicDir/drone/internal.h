/****************************************************************************
  Copyright (C), 2011, Jacky

  Histroy    :
              1)  Created by Jacky   2021/01/20
  Description: 
****************************************************************************/

#ifndef __INTERNAL_H__
#define __INTERNAL_H__
#include <netinet/in.h>

#define  MAX_H264_JPG_FILES    10240 
#define  STORAGE_MOUNT_DIR     "/hivintek/sd"
#define  STORAGE_DATA_DIR      "/hivintek/sd" 

#define  MAX_SPI_PKT_LEN       256 //128
#define  MAX_SPI_BUFFER        (MAX_SPI_PKT_LEN * 1)
#define  MAX_MUL_BUFFER        256
#define  INVALID_FID           (0xFFFF)
#define  PLANE_PARAMETER_LEN    47

#define  COE_NUM               10

#define  IR_WIDTH              640
#define  IR_HEIGHT             512
#define  IR_PIEXL              12000
#define  IR_FOCAL_LEN          2500
#define  VI_WIDTH              2560
#define  VI_HEIGHT             1440
#define  VI_PIEXL              2000
#define  VI_FOCAL_LEN          1200
//Total is 16 bytes
typedef struct
{    
    U16  fid         ;  
    U8   channel     ; 
    U8   video       ; 
    U32  length      ;
    U32  create_time ;
    U32  access_time ;   
}H264_JPG_FILE ;

typedef struct
{    
    U8   name[8]        ;  // 0 ~ 7 
    U8   ext[3]         ;  // 8 ~ 10  
    U8   reserved1      ;  // 11
    U8   reserved2      ;  // 12
    U8   create_time_10ms    ;  // 13
    U8   create_time[2] ;  // 14 ~ 15
    U8   create_date[2] ;  // 16 ~ 17
    U8   access_time[2] ;  // 18 ~ 19
    U8   access_date[2] ;  // 20 ~ 21 
    U8   modify_time[2] ;  // 22 ~ 23
    U8   modify_date[2] ;  // 24 ~ 25
    U8   reserved3[2]   ;  // 26 ~ 27
    U32  length         ;  // 28 ~ 31 
}H264_JPG_SPI_FILE ;

typedef struct
{
    U8    magic[2]      ;   // 0xEB90
    U8    dst[2]        ;   // 0xFF00   
    U8    src[2]        ;   // 0xF200
    U8    type          ;   // 0x21: vi frame 0x23; ir frame 0x26: JPEG 0x27: JPEG ir 0x20: NULL frame 0x2d: data
    U8    seq           ;   // frame num
    U8    encryption[3] ;
    U8    reserved[2]   ;   // 0x0000
    U8    len           ;
}FRAME_SPI_HEADER ;

#define  DOME_LEN        28
#define  DOME_STATE_SYNC  0
#define  DOME_STATE_TYPE  1
#define  DOME_STATE_DATA  2
#define  DOME_STATE_CRC1  3
#define  DOME_STATE_CRC2  4

#define  FC_PACKET_LENGHT          36
#define  PACKET_STATE_SYNC1        0
#define  PACKET_STATE_TYPE2        1
#define  PACKET_STATE_DATA1        2
#define  FC_PACKET_STATE_CRC       3


typedef struct
{
    int          running     ;
    MSG_Q_ID     queue       ;
    MSG_Q_ID     fc_queue    ;
    MSG_Q_ID     dome_queue  ;
    HAL_DRIVER   * vpss      ;   
    HAL_DRIVER   * vidrv     ; 
    
    pthread_mutex_t    comm_mutex    ;
    
    SHM_CONFIG   *shmcfg    ; 
    int          drone_mode ; // 0: VI ; 1: IR
    int          work_mode ;  // 0: live view ; 1: file export mode
    int          ir_mode ;    // 0: write heat ; 1: black heat
    
    H264_JPG_FILE    *  h264_jpg_files   ;  //存储在制定目录的文件信息     
    int                 current_file_pos ;
    int                 max_file_count   ;  //当前文件个数
    int                 current_file_id ;   //当前文件编号

    char         new_cmd;
    char         old_cmd;
    
    U16          fw_angle ;
    short        gd_angle ;
    int          video_type ;
    int          old_video_type ;
    int          rec_mode ;
    int          pic_mode ;
    int          laster_state ;
    int          bc_state;
    int          bright_value ;
    int          contrast_value ;
    int          dome_state ;
    S16          target_x ;
    S16          target_y ;
    int          laser_dis;
    int          font_show ;
    int          pic_timer;
    int          fc_timer;
    int          zoom;
    int          current_zoom;
    int          track_state;
	int          dispaly_number;
    
    int          bitrate_mode;
    float        bitrate[4];
    float        coe[COE_NUM];


    U16          vi_resolution_x;
    U16          vi_resolution_y;


    U16          ir_resolution_x;
    U16          ir_resolution_y;

    
    U16          ir_detector_x;
    U16          ir_detector_y;
    U16          detector_actual_x[COE_NUM];
    U16          detector_actual_y[COE_NUM];
    U16          vi_sensor_x;
    U16          vi_sensor_y;
    U16          sensor_actual_x[COE_NUM];
    U16          sensor_actual_y[COE_NUM];
    U8           vi_frame_rate;
    U8           ir_frame_rate;
    int          vi_bitrate;
    int          ir_bitrate;
    U16          load_id;
    U8           latest_cmd;
    char         laser_result[32];
    int          laser_distance;
    char         laser_buffer[32];
    char         laser_count;
    int          laser_state;
    int          laser_ptr;
    int          laser_show;
    int          cm_set_unit_count;

    U8           plane_parameter[PLANE_PARAMETER_LEN];
    U8           fc_buffer[MAX_SPI_PKT_LEN+1] ;
    char         dome_buffer[54] ;
    U8           dome_count;
    U8           dome_last_cmd;

    int          dome_ack_count;
    int          send_mode;
    int          dome_old_x;
    int          dome_old_y;
    U32          frame_id;
    U16          free_storage;  // M
    U64          capacity;
    U64          used;

    S16          dome_azimuth_angle;
    S16          dome_azimuth_angle_rate;
    S16          dome_altitude_angle;
    S16          dome_altitude_angle_rate;
    S16          plane_pitch_angle;
    S16          plane_pitch_angle_rate;
    S16          plane_roll_angle;
    S16          plane_roll_angle_rate;
    U16          plane_course_angle;
    S16          plane_course_angle_rate;
    S16          ground_velocity;
    S8           fan_cmd;
    int          plane_longitude;
    int          plane_latitude;
    S16          plane_altitude;
    S16          plane_speed;

    int          target_longitude;
    int          target_latitude;
    S16          target_altitude;
    U8           usb_state;
    U8           ttl;


    double       vi_fov_x[COE_NUM];
    double       vi_fov_y[COE_NUM];

    double       ir_fov_x[COE_NUM];
    double       ir_fov_y[COE_NUM];    
        
    U16          len_ir;
    U16          len_vi;
    U16          vi_sensor_pixel;
    U16          ir_detector_pixel;


    // dome serial
    char         dome_name[32];
    int          dome_packet_state   ;
    int          dome_ptr         ;
    U8           dome_recv_buffer[DOME_LEN] ;
    U8           dome_roll_state;
    U8           dome_pitch_state;
    U8           dome_azimuth_state;
    S16          dome_pitch;
    U16          dome_azimuth;
    // ir serial
    char         ir_name[32];
    int          ir_packet_state   ;
    int          ir_ptr         ;
    U8           ir_buffer[64] ;
    int          ir_recv_count ;

    // fc serial
    char         name[32];
    int          packet_state   ;
    int          data_ptr;
    U8           data_buffer[96];
    int          fc_protocl_len;
    
    int          track_old_x;
    int          track_old_y;
    U8           ack_buffer[51] ;
    U8           gun_aim_state;
    S16        sight_pitch;
    U16        sight_azimuth;
    double          fc_longitude;
    double          fc_latitude;
    float           fc_height;
    char            fc_we ;
    char            fc_ns ;
    double          aim_longitude;
    double          aim_latitude;
    float           aim_height;
    char            aim_we ;
    char            aim_ns ;
    //****************************

    //SPI 
    int                 spi_fd ;
    U8                  spi_buffer[MAX_SPI_BUFFER] ;
    int                 spi_frame_idx   ;
    U8                  spi_prev_buffer[MAX_SPI_PKT_LEN] ;
    int                 spi_prev_count  ;
    
    // OSD
    GUI_PAGE         *  status_page_vi  ;
    GUI_PAGE         *  status_page_ir  ;   

    int          dyt_state;
    U8           dyt_buffer[32];
    int          dyt_ptr;
    U8           dyt_cmd;

    float        roll_speed;
    float        pitch_speed;
    float        azimuth_speed;

    float        dyt_distance;

    int          tracker_x;
    int          tracker_y;
    U8           ai_type;
    U8           image_enhance;
    U8           motor_state;
    U8           tracker_state;
    U8           electric_lock;
    int          multicast_addr;
    int          multicast_port;
    int          mul_socket;
    struct sockaddr_in   server;
    U8           laser_cmd;
    U8           ai_state;    
    int          vi_cross_x;
    int          vi_cross_y;
    int          ir_cross_x;
    int          ir_cross_y;
}DRONE_CONTEXT ;


typedef struct
{
    MESSAG_HEADER  head  ;    
    SYSENC_FRAME * fr    ;
    int          data[3] ;    //淇濈暀
}NET_STREAM_FRAME_MSG    ;



#define  IR_LEN         55
#define  IR_STATE_SYNC  0
#define  IR_STATE_LEN  1
#define  IR_STATE_DATA  2
#define  IR_STATE_CRC   3

#define  CORRECT_RESPONE      0x33
#define  POLARITY_RESPONE     0x2D
#define  ZOOM_RESPONE         0x2A
#define  AUTOCHECK_RESPONE    0x00

#define  YFR_LASER_SYNC0  0
#define  YFR_LASER_SYNC1  1
#define  YFR_LASER_LEN    2
#define  YFR_LASER_DATA   3
#define  YFR_LASER_END    4

#define DOME_MOTOR_TURN_OFF     0x00
#define DOME_MOTOR_TURN_ON      0x01
#define DOME_AUTO_TRACK_MODE    0x02
#define DOME_STOP_TRACK_MODE    0x12
#define DOME_TARGET_MASK        0x03
#define DOME_TARGET_LOST        0x04
#define DOME_SCAN_MODE          0x05
#define DOME_DATA_LEAD          0x06
#define DOME_PROTECT_MODE       0x07
#define DOME_VERTICAL_DOWN      0x08
#define DOME_HAND_TRACE         0x09
#define DOME_CENTRA_BACK        0x0a
#define DOME_HAND_SPEED         0x0b
#define DOME_GEOGROPY_LEAD      0x0C
#define DOME_NULL_CMD           0x0F
#define DOME_SUPPESS_DRIFT      0x1B

#define DOME_ELECTRIC_TRUN_ON   0x0E
#define DOME_ELECTRIC_TRUN_OFF  0x1E
#define DOME_TRACKER_TURN_ON    0x0D
#define DOME_TRACKER_TRUN_OFF   0x1D
#define DOME_LASER_TRUN_ON           0x15
#define DOME_LASER_TRUN_OFF          0x16

#define ANGLE_HORIZONTAL_IR  90
#define ANGLE_VERTICAL_IR    60
#define ANGLE_HORIZONTAL_VI  90
#define ANGLE_VERTICAL_VI    60

#define WORD_CMD(a,b,c,d) (d<<24|c<<16|b<<8|a)
#define MODE_SWITCH   0x03000110
#define INFO_GET      0x03000101
#define FILES_LIST    0x03000111
#define FILES_EXPORT  0x03000121
#define FILE_DELETE   0x03000131
#define FILES_DELETE  0x03000132

typedef struct
{
    U8  state_word0;
    U8  state_word1;
    U16 resolution_x;
    U16 resolution_y;
    U16 sensor_x;
    U16 sensor_y;
    U16 pixel;
    U16 len;
    U8  bitrate;
    U8  frame_rate;
    int rate;
    U16 actual_x;
    U16 actual_y;
}STATE_PARAMETER;

void *drone_main_loop( void * priv ) ;
//int  ir_serial_register_cb();
//int  dome_serial_register_cb();

//defined in drone-utils.c
int   INTF_load_sdcard_files    ( void * data , int * max_id ) ; 
void  INTF_delete_file  ( int video , int fid , int ch )  ;      
void  INTF_delete_file_by_name( char * name )  ;               
void  INTF_make_filename( int video , int fid , int ch , char * no_ext_name ) ; 
void  INTF_start_record ( int fid   , int ch ) ;  
void  INTF_stop_record  ( int ch ) ;  
void  INTF_snap_picture ( int fid , int ch , int count ) ;
int   INTF_parse_file_info( H264_JPG_FILE  * info , char * dir , char * name ) ; 
void  drone_switch_zoom(DRONE_CONTEXT *drone, int zoom);
void  drone_send_msg_update_cross ( DRONE_CONTEXT * p, int mode);


static const U16 crc16_tab[256]= {
    0x0000,0x1021,0x2042,0x3063,0x4084,0x50A5,0x60C6,0x70E7,
    0x8108,0x9129,0xA14A,0xB16B,0xC18C,0xD1AD,0xE1CE,0xF1EF,
    0x1231,0x0210,0x3273,0x2252,0x52B5,0x4294,0x72F7,0x62D6,
    0x9339,0x8318,0xB37B,0xA35A,0xD3BD,0xC39C,0xF3FF,0xE3DE,
    0x2462,0x3443,0x0420,0x1401,0x64E6,0x74C7,0x44A4,0x5485,
    0xA56A,0xB54B,0x8528,0x9509,0xE5EE,0xF5CF,0xC5AC,0xD58D,
    0x3653,0x2672,0x1611,0x0630,0x76D7,0x66F6,0x5695,0x46B4,
    0xB75B,0xA77A,0x9719,0x8738,0xF7DF,0xE7FE,0xD79D,0xC7BC,
    0x48C4,0x58E5,0x6886,0x78A7,0x0840,0x1861,0x2802,0x3823,
    0xC9CC,0xD9ED,0xE98E,0xF9AF,0x8948,0x9969,0xA90A,0xB92B,
    0x5AF5,0x4AD4,0x7AB7,0x6A96,0x1A71,0x0A50,0x3A33,0x2A12,
    0xDBFD,0xCBDC,0xFBBF,0xEB9E,0x9B79,0x8B58,0xBB3B,0xAB1A,
    0x6CA6,0x7C87,0x4CE4,0x5CC5,0x2C22,0x3C03,0x0C60,0x1C41,
    0xEDAE,0xFD8F,0xCDEC,0xDDCD,0xAD2A,0xBD0B,0x8D68,0x9D49,
    0x7E97,0x6EB6,0x5ED5,0x4EF4,0x3E13,0x2E32,0x1E51,0x0E70,
    0xFF9F,0xEFBE,0xDFDD,0xCFFC,0xBF1B,0xAF3A,0x9F59,0x8F78,
    0x9188,0x81A9,0xB1CA,0xA1EB,0xD10C,0xC12D,0xF14E,0xE16F,
    0x1080,0x00A1,0x30C2,0x20E3,0x5004,0x4025,0x7046,0x6067,
    0x83B9,0x9398,0xA3FB,0xB3DA,0xC33D,0xD31C,0xE37F,0xF35E,
    0x02B1,0x1290,0x22F3,0x32D2,0x4235,0x5214,0x6277,0x7256,
    0xB5EA,0xA5CB,0x95A8,0x8589,0xF56E,0xE54F,0xD52C,0xC50D,
    0x34E2,0x24C3,0x14A0,0x0481,0x7466,0x6447,0x5424,0x4405,
    0xA7DB,0xB7FA,0x8799,0x97B8,0xE75F,0xF77E,0xC71D,0xD73C,
    0x26D3,0x36F2,0x0691,0x16B0,0x6657,0x7676,0x4615,0x5634,
    0xD94C,0xC96D,0xF90E,0xE92F,0x99C8,0x89E9,0xB98A,0xA9AB,
    0x5844,0x4865,0x7806,0x6827,0x18C0,0x08E1,0x3882,0x28A3,
    0xCB7D,0xDB5C,0xEB3F,0xFB1E,0x8BF9,0x9BD8,0xABBB,0xBB9A,
    0x4A75,0x5A54,0x6A37,0x7A16,0x0AF1,0x1AD0,0x2AB3,0x3A92,
    0xFD2E,0xED0F,0xDD6C,0xCD4D,0xBDAA,0xAD8B,0x9DE8,0x8DC9,
    0x7C26,0x6C07,0x5C64,0x4C45,0x3CA2,0x2C83,0x1CE0,0x0CC1,
    0xEF1F,0xFF3E,0xCF5D,0xDF7C,0xAF9B,0xBFBA,0x8FD9,0x9FF8,
    0x6E17,0x7E36,0x4E55,0x5E74,0x2E93,0x3EB2,0x0ED1,0x1EF0
};

#endif
