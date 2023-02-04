/****************************************************************************
  Copyright (C), 2021, Jacky

  Histroy    :
              1)  Created by Jacky   2021/01/20
  Description:
****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <math.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//#include "message.h"

#include "sys-service.h"
#include "sys-core.h"
#include "sys-config.h"
#include "sys-hal.h"
#include "sys-gui.h"
#include "sys-list.h"
#include "sys-message.h"
#include "sys-hal.h"
#include "app-codec.h"
#include "app-sysctrl.h"
#include "app-vinput.h"

#include "drone.h"
#include "tracker.h"
#include "recorder.h"
#include "board.h"
#include "internal.h"
#include "app-serial.h"

#include "tracker.h"
#include "ai.h"
#include "iray.h"
// Private Variable
static DRONE_CONTEXT g_drone_context ;

#define WORD_CMD(a,b,c,d) (d<<24|c<<16|b<<8|a)
#define MODE_SWITCH   0x03000110
#define INFO_GET      0x03000101
#define FILES_LIST    0x03000111
#define FILES_EXPORT  0x03000121
#define FILE_DELETE   0x03000131
#define FILES_DELETE  0x03000132

#define VI_CODEC_WIDTH_HALF  (SYSAPP_CODEC_VI_PORT_W/2) 
#define VI_CODEC_HEIGHT_HALF (SYSAPP_CODEC_VI_PORT_H/2)

#define IR_CODEC_WIDTH_HALF  (SYSAPP_CODEC_IR_PORT_W/2) 
#define IR_CODEC_HEIGHT_HALF (SYSAPP_CODEC_IR_PORT_H/2)

#define  VI_CROSS_CENTER_POINT_X (SYSVIN_VPSS_VI_PORT_W - CROSS_OSD_SIZE_HD )/2  
#define  VI_CROSS_CENTER_POINT_Y (SYSVIN_VPSS_VI_PORT_H - CROSS_OSD_SIZE_HD )/2
#define  IR_CROSS_CENTER_POINT_X (SYSVIN_VPSS_IR_PORT_W - CROSS_OSD_SIZE_SD )/2
#define  IR_CROSS_CENTER_POINT_Y (SYSVIN_VPSS_IR_PORT_H - CROSS_OSD_SIZE_SD )/2
#define  PTS_TO_FRAME_ID(a)  (((a)/30000)%50)

/////////////////////////////////////////////////////////
//     Function : drone_make_crc16
//  Description :
//        Input :
//       Output :
static U16 drone_make_crc16(U8 *buf, int nlen)
{
    int i = 0;
    U16 wcrc = 0;
    if (!buf)
        return 0;

    for(i = 0; i < nlen; i++)
        wcrc = (wcrc<<8) ^ crc16_tab[((wcrc>>8) ^ *(unsigned char *)buf++)&0x00FF];
    return wcrc;
}
/////////////////////////////////////////////////////////
//     Function : drone_calc_checksum
//  Description : 
//        Input :
//       Output :
static U8  drone_calc_checksum( U8 * dat , int len )
{
    U8 crc ;

    crc = 0 ;
    while( len-- > 0 )
        crc  += *dat++ ;

    return crc ;
}

///////////////////////// FC Serial /////////////////////////////
extern int  core_send_message( KERNEL_MESSAGE * msg ) ;
extern int  core_do_command  ( int id , int op , int len , void * ibuf , void * obuf ) ;
extern void core_fill_message( MESSAG_HEADER * h , int sender , int receiver , int cmd , int total ) ;


/////////////////////////////////////////////////////////
//     Function : drone_dome_do_serial_protocol
//  Description :
//        Input :
//       Output :
static void drone_dome_do_serial_protocol( DRONE_CONTEXT* p )
{
    p->laster_state = (p->dome_recv_buffer[13] & 0x04) >> 2;
    p->dome_roll_state = (p->dome_recv_buffer[13] & 0x80) >> 7;
    p->dome_pitch_state = (p->dome_recv_buffer[13] & 0x40) >> 6;
    p->dome_azimuth_state = (p->dome_recv_buffer[13] & 0x20) >> 5;
    memcpy( &p->laser_distance, p->dome_recv_buffer+10, 2);

    memcpy(&p->dome_pitch, p->dome_recv_buffer + 8, 2);
    memcpy(&p->dome_azimuth, p->dome_recv_buffer + 4, 2);

    core_do_command(APP_DRONE, DRONE_DOME_INFO, 0, NULL, NULL);
}

/////////////////////////////////////////////////////////
//     Function : drone_dome_send_buffer
//  Description :
//        Input :
//       Output :
int drone_dome_send_buffer(DRONE_CONTEXT * drone , U8 dome_cmd, int flag )
{
    U16 wcrc;

    pthread_mutex_lock  ( &drone->comm_mutex );
/*
    if(drone->dome_last_cmd == DOME_HAND_TRACE)
    {
        drone->dome_azimuth_angle       = 0x7fff;
        drone->dome_azimuth_angle_rate  = 0x7fff;
        drone->dome_altitude_angle      = 0x7fff;
        drone->dome_altitude_angle_rate = 0x7fff;
        drone->dome_last_cmd            = DOME_IDLE_MODE;
    }
*/ 

    drone->dome_buffer[0]  = 0x7E;
    drone->dome_buffer[1]  = 0x16;
    drone->dome_buffer[2]  = drone->dome_count++;
    drone->dome_buffer[3]  = dome_cmd ;
  //  drone->dome_buffer[4]  = flag == 2 ? (drone->dome_org_azimuth_angle)&0xff:(drone->dome_azimuth_angle)&0xff;
  //  drone->dome_buffer[5]  = flag == 2 ? (drone->dome_org_azimuth_angle>>8)&0xff:(drone->dome_azimuth_angle>>8)&0xff;
    drone->dome_buffer[6]  = (drone->dome_azimuth_angle_rate)&0xff;
    drone->dome_buffer[7]  = (drone->dome_azimuth_angle_rate>>8)&0xff;
//  drone->dome_buffer[8]  = flag ==2 ? (drone->dome_org_altitude_angle)&0xff:(drone->dome_altitude_angle)&0xff;
//  drone->dome_buffer[9]  = flag ==2 ? (drone->dome_org_altitude_angle>>8)&0xff:(drone->dome_altitude_angle>>8)&0xff;
    drone->dome_buffer[10] = (drone->dome_altitude_angle_rate)&0xff;
    drone->dome_buffer[11] = (drone->dome_altitude_angle_rate>>8)&0xff;
    drone->dome_buffer[12] = (drone->plane_pitch_angle)&0xff;
    drone->dome_buffer[13] = (drone->plane_pitch_angle>>8)&0xff;
    drone->dome_buffer[14] = (drone->plane_pitch_angle)&0xff;
    drone->dome_buffer[15] = (drone->plane_pitch_angle_rate>>8)&0xff;
    
    drone->dome_buffer[16] = (drone->target_x)&0xff;
    drone->dome_buffer[17] = (drone->target_x>>8)&0xff;
    drone->dome_buffer[18] = (drone->target_y)&0xff;
    drone->dome_buffer[19] = (drone->target_y>>8)&0xff;

    drone->dome_buffer[20] = (drone->plane_course_angle)&0xff;
    drone->dome_buffer[21] = (drone->plane_course_angle>>8)&0xff;
    drone->dome_buffer[22] = (drone->plane_course_angle_rate)&0xff;
    drone->dome_buffer[23] = (drone->plane_course_angle_rate>>8)&0xff;
    drone->dome_buffer[24] = (drone->ground_velocity)&0xff;
    drone->dome_buffer[25] = (drone->ground_velocity>>8)&0xff;
    drone->dome_buffer[26] = drone->laser_cmd;
    drone->dome_buffer[27] = drone->fan_cmd;
    drone->dome_buffer[28] = (drone->plane_longitude)&0xff;
    drone->dome_buffer[29] = (drone->plane_longitude>>8)&0xff;
    drone->dome_buffer[30] = (drone->plane_longitude>>16)&0xff;
    drone->dome_buffer[31] = (drone->plane_longitude>>24)&0xff;
    drone->dome_buffer[32] = (drone->plane_latitude)&0xff;
    drone->dome_buffer[33] = (drone->plane_latitude>>8)&0xff;
    drone->dome_buffer[34] = (drone->plane_latitude>>16)&0xff;
    drone->dome_buffer[35] = (drone->plane_latitude>>24)&0xff;
    drone->dome_buffer[36] = (drone->plane_altitude)&0xff;
    drone->dome_buffer[37] = (drone->plane_altitude>>8)&0xff;
    drone->dome_buffer[38] = (drone->plane_speed)&0xff;
    drone->dome_buffer[39] = (drone->plane_speed>>8)&0xff;
    drone->dome_buffer[40] = (drone->target_longitude)&0xff;
    drone->dome_buffer[41] = (drone->target_longitude>>8)&0xff;
    drone->dome_buffer[42] = (drone->target_longitude>>16)&0xff;
    drone->dome_buffer[43] = (drone->target_longitude>>24)&0xff;
    drone->dome_buffer[44] = (drone->target_latitude)&0xff;
    drone->dome_buffer[45] = (drone->target_latitude>>8)&0xff;
    drone->dome_buffer[46] = (drone->target_latitude>>16)&0xff;
    drone->dome_buffer[47] = (drone->target_latitude>>24)&0xff;
    drone->dome_buffer[48] = (drone->target_altitude)&0xff;
    drone->dome_buffer[49] = (drone->target_altitude>>8)&0xff;
    drone->dome_buffer[50] = drone->usb_state;
    drone->dome_buffer[51] = drone->ttl;

    wcrc = drone_make_crc16((U8 *)&drone->dome_buffer[0], sizeof(drone->dome_buffer)-2);

    drone->dome_buffer[52] = wcrc & 0xff;
    drone->dome_buffer[53] = (wcrc>>8) & 0xff;

    SERIAL_SEND_DATA(1, drone->dome_buffer, sizeof(drone->dome_buffer));


    drone->dome_last_cmd = drone->dome_buffer[3];

    pthread_mutex_unlock  ( &drone->comm_mutex );

    return 0;
}

/////////////////////////////////////////////////////////
//     Function : drone_send_update_state
//  Description :
//        Input :
//       Output :
void drone_send_update_state( int cmd )
{
    MESSAG_HEADER msg  ;

    core_fill_message( (MESSAG_HEADER*)&msg, APP_DRONE, APP_DRONE, cmd, sizeof(MESSAG_HEADER) );
    core_send_message( (KERNEL_MESSAGE *)&msg ) ;
}

/////////////////////////////////////////////////////////
//     Function : drone_send_update_state
//  Description :
//        Input :
//       Output :
void drone_send_update_state_with_par( DRONE_CONTEXT * p , int cmd )
{
    KERNEL_MESSAGE msg  ;

    core_fill_message( (MESSAG_HEADER*)&msg, APP_DRONE, APP_DRONE, cmd, sizeof(MESSAG_HEADER) + 9 );

    msg.data[0] = p->dome_azimuth_angle&0xff; 
    msg.data[1] = (p->dome_azimuth_angle>>8)&0xff;
    msg.data[2] = p->dome_altitude_angle&0xff;
    msg.data[3] = (p->dome_altitude_angle>>8)&0xff;
    
    msg.data[4] = p->dome_recv_buffer[3]; // state
    msg.data[5] = p->dome_recv_buffer[4]; // azimuth_angle dome return
    msg.data[6] = p->dome_recv_buffer[5];
    msg.data[7] = p->dome_recv_buffer[8]; // altitude_angle dome return 
    msg.data[8] = p->dome_recv_buffer[9];

    core_send_message( (KERNEL_MESSAGE *)&msg ) ;
}

/////////////////////////////////////////////////////////
//     Function : drone_dome_serial_input
//  Description :
//        Input :
//       Output :
static void drone_dome_serial_input( U8 dat , DRONE_CONTEXT * p )
{
    U16  wcrc ;
    U8  a, b;
   // printf("%s %x state = %x\r\n", __FUNCTION__, dat, p->dome_packet_state);
    switch( p->dome_packet_state )
    {
        case DOME_STATE_SYNC :
            if ( dat == 0x7e )
            {
                p->dome_packet_state = DOME_STATE_TYPE ;
                p->dome_recv_buffer[0] = dat ;
            }
            break ;

        case DOME_STATE_TYPE :
            if( dat == 0x16 )
            {
                p->dome_recv_buffer[1] = dat ;
                p->dome_ptr = 2;
                p->dome_packet_state = DOME_STATE_DATA ;
            }
            break ;

        case DOME_STATE_DATA:
            p->dome_recv_buffer[p->dome_ptr++] = dat ;

            if( p->dome_ptr == DOME_LEN - 2 )
                p->dome_packet_state   = DOME_STATE_CRC1 ;
            break ;

        case DOME_STATE_CRC1 :
            p->dome_recv_buffer[DOME_LEN - 2] = dat ;
            p->dome_packet_state   = DOME_STATE_CRC2 ;
            break;

        case DOME_STATE_CRC2 :
            a = p->dome_recv_buffer[DOME_LEN - 2];
            b = dat;
            p->dome_recv_buffer[DOME_LEN - 1] = dat ;
            wcrc = drone_make_crc16( p->dome_recv_buffer, DOME_LEN - 2 ) ;
           // printf("crc=%x,b>>8|a=%x\n",wcrc,((b<<8)|a));
            if( wcrc == ((b<<8)|a) )
            {
               // printf("dome_do\n");
                drone_dome_do_serial_protocol( p ) ;
               // drone_send_update_state_with_par(p, DRONE_DOME_ACK);
            }
            p->dome_packet_state   = DOME_STATE_SYNC ;

            break ;
    }
}


int drone_dome_serial_receive_packet ( unsigned char  *dat, int len, void *priv )
{
    DRONE_CONTEXT *packet = (DRONE_CONTEXT *)priv ;

    while ( len-- > 0 )
    {
        drone_dome_serial_input( *dat++, packet );
    }

    return 1;
}

#if 1
#define  LASER_LEN   8
#define  LASER_SYNC  0
#define  LASER_DATA  1
#define  LASER_END   2


/////////////////////////////////////////////////////////
//     Function : drone_dome_serial_input
//  Description :
//        Input :
//       Output :
static void drone_laser_serial_input( U8 dat , DRONE_CONTEXT * p )
{
    int num, i;
    
    switch( p->laser_state )
    {
        case LASER_SYNC :

            if(dat >= '0' && dat <= '9')
            {
                p->laser_state = LASER_DATA ;
                p->laser_buffer[0] = dat ;
                p->laser_ptr++;
            }

            break ;

        case LASER_DATA:

            if( dat == 0x0d )
                p->laser_state = LASER_END ;

            p->laser_buffer[p->laser_ptr] = dat ;
            p->laser_ptr++;

            break ;

        case LASER_END:

            if( dat == 0x0a )
            {
                memset(p->laser_result, 0, sizeof(p->laser_result));
                memcpy(p->laser_result, p->laser_buffer, p->laser_ptr);
/*
                LOG_PRINTF("  %.2x \n", p->laser_buffer[p->laser_ptr-1]);
                LOG_PRINTF("  %.2x \n", p->laser_buffer[p->laser_ptr-2]);
                LOG_PRINTF("  %.2x \n", p->laser_buffer[p->laser_ptr-3]);
                LOG_PRINTF("  %.2x \n", p->laser_buffer[p->laser_ptr-4]);
                LOG_PRINTF("  %.2x \n", p->laser_buffer[p->laser_ptr-5]); 
*/
                LOG_PRINTF(" ---p->laser_ptr = %d---", p->laser_ptr); 

                p->laser_distance = 0;
                if(p->laser_buffer[p->laser_ptr-2] == 'm')
                {
                    if(p->laser_buffer[p->laser_ptr-4] == ',')
                    {
                        num = p->laser_ptr-4;

                        LOG_PRINTF(" num = %d \n", num);
                            
                        for(i=num-1; i>=0; i--)
                        {
                            p->laser_distance += (p->laser_buffer[i]-48)*(int)pow((double)10,(double)(num-i)); 

            //              LOG_PRINTF(" i = %d %.2x  %.2x \n", i,p->laser_buffer[i], (p->laser_buffer[i]-48));
                        }

                        p->laser_distance += (p->laser_buffer[p->laser_ptr-3]-48);

            //          LOG_PRINTF("  %.2x  %.2x \n", p->laser_buffer[p->laser_ptr-3], p->laser_buffer[p->laser_ptr-3]-48);

                    }else
                    {
                        num = p->laser_ptr-2;
                        
                        for(i=num-1; i>=0; i--)
                            p->laser_distance += (p->laser_buffer[i]-48)*(int)pow((double)10,(double)(num-i)); 
                    }   
                }

                LOG_PRINTF("--- p->laser_distance = %d, p->laser_result = %s ----", p->laser_distance, p->laser_result);

                core_do_command( APP_TRACKER , TRACKER_SET_OBJ_FRAME , sizeof(int) , (void*)&p->laser_distance , NULL ) ; 
               
                drone_send_update_state(DRONE_UPDATE_STATE);
            }

            p->laser_ptr = 0;
            p->laser_state = LASER_SYNC ;


            break ;

    }
}
#endif




/////////////////////////////////////////////////////////
//     Function : drone_dome_serial_input
//  Description :
//        Input :
//       Output :
static void drone_yfr_laser_serial_input( U8 dat , DRONE_CONTEXT * p )
{
    int i;
    
    switch( p->laser_state )
    {
        case YFR_LASER_SYNC0 :

            if(dat == 0xee)
            {
                p->laser_state = YFR_LASER_SYNC1 ;
                p->laser_buffer[0] = dat ;
                p->laser_ptr++;
            }
            else
            {
                p->laser_state = YFR_LASER_SYNC0 ;
                p->laser_ptr = 0;
            }
            

            break ;

        case YFR_LASER_SYNC1 :

            if(dat == 0x16)
            {
                p->laser_state = YFR_LASER_LEN ;
                p->laser_buffer[1] = dat ;
                p->laser_ptr++;
            }
            else
            {
                 p->laser_state = YFR_LASER_SYNC0 ;
                 p->laser_ptr = 0;
            }

            break ;


       case YFR_LASER_LEN :

            p->laser_state = YFR_LASER_DATA;
            p->laser_buffer[2] = dat ;
            p->laser_count = dat;
            p->laser_ptr++;

            break;
            
        case YFR_LASER_DATA:
            
            p->laser_buffer[p->laser_ptr] = dat ;
            p->laser_ptr++;

            if( p->laser_ptr == p->laser_count+3 )
            {
                p->laser_state = YFR_LASER_END ;
            }

            break ;

        case YFR_LASER_END:

            p->laser_buffer[p->laser_ptr] = dat ;

        
            printf("\n");
            for(i=0; i<=p->laser_buffer[2]+3; i++)
                printf(" %.2x", p->laser_buffer[i]);
            printf("\n");

            if(p->laser_buffer[5] != 0)
            {
                p->laser_distance = 0;
                strcpy(p->laser_result, "0.0m");
            }else
            {
                p->laser_distance = ((p->laser_buffer[6]<<8)| p->laser_buffer[7])*10+p->laser_buffer[8];
                sprintf(p->laser_result, "%.1fm", (float)p->laser_distance/10.0);
            }

            LOG_PRINTF(" ---p->laser_distance = %d, p->laser_result = %s---\n", p->laser_distance, p->laser_result); 
            core_do_command( APP_TRACKER , TRACKER_SET_OBJ_FRAME , sizeof(int) , (void*)&p->laser_distance , NULL ) ; 
            
            drone_send_update_state(DRONE_UPDATE_STATE);

            p->laser_ptr = 0;
            p->laser_state = YFR_LASER_SYNC0 ;

            break ;

    }


}

int drone_laser_serial_receive_packet ( unsigned char  *dat, int len, void *priv )
{
    DRONE_CONTEXT *packet = (DRONE_CONTEXT *)priv ;

    while ( len-- > 0 )
    {
        drone_laser_serial_input( *dat++, packet );
    }
    return 1;
}

int drone_yfr_laser_serial_receive_packet ( unsigned char  *dat, int len, void *priv )
{
    DRONE_CONTEXT *packet = (DRONE_CONTEXT *)priv ;

    while ( len-- > 0 )
    {
        drone_yfr_laser_serial_input( *dat++, packet );
    }
    return 1;
}

/////////////////////////////////////////////////////////
//     Function : serial_ir_data_loop
//  Description :
//        Input :
//       Output :
static void drone_ir_do_serial_input( U8 dat , DRONE_CONTEXT * p )
{
    switch( p->ir_packet_state )
    {
        case IR_STATE_SYNC :
            if ( dat == 0x55 )
            {
                p->ir_packet_state = IR_STATE_LEN ;
                p->ir_buffer[0] = dat ;
                p->ir_ptr = 1;
            }
            break ;

        case IR_STATE_LEN:
            p->ir_buffer[p->ir_ptr++] = dat ;

            if( dat == 0xEB )
                p->ir_packet_state   = IR_STATE_CRC;

            break ;

        case IR_STATE_CRC :
            p->ir_buffer[p->ir_ptr++] = dat ;

            if( 0xAA == dat )
            {
                LGC6122_ir_do_serial_protocol( p->ir_buffer ) ;
            }
            p->ir_packet_state   = IR_STATE_SYNC ;
            break ;
    }
}

/**********************************************
 IRAY串口
***********************************************/
int drone_ir_serial_receive_packet ( unsigned char  *dat, int len, void *priv )
{
    DRONE_CONTEXT *packet = (DRONE_CONTEXT *)priv ;

    while ( len-- > 0 )
    {
         printf("0x%x \n", *dat++ );
        drone_ir_do_serial_input( *dat++, packet );
    }
    return 1;
}



/////////////////////////////////////////////////////////
//     Function : fc_send_shutter_correction
//  Description :
//        Input :
//       Output :
void drone_fc_send_shutter_correction()
{
    U8 buffer[9];

    buffer[0] = 0xAA;
    buffer[1] = 0x05;
    buffer[2] = 0x01;
    buffer[3] = 0x02;
    buffer[4] = 0x02;
    buffer[5] = 0xC1;
    buffer[6] = 0x75;
    buffer[7] = 0xEB;
    buffer[8] = 0xAA;
    SERIAL_SEND_DATA(0, buffer, sizeof(buffer));
}

/////////////////////////////////////////////////////////
//     Function : fc_send_track_stop_cmd
//  Description :
//        Input :
//       Output :
void drone_fc_send_track_stop_cmd(DRONE_CONTEXT * p)
{
    KERNEL_MESSAGE msg  ;

    p->track_state = 0;
    p->target_x = 0;
    p->target_y = 0;

    MESSAG_HEADER *head = (MESSAG_HEADER *)&msg ;
    core_fill_message( head, APP_DRONE, APP_TRACKER, TRACKER_STOP_TRACK, sizeof(MESSAG_HEADER) );
    core_send_message( &msg ) ;
}

/////////////////////////////////////////////////////////
//     Function : drone_dome_hand_track_handle
//  Description :
//        Input :
//       Output :
int drone_dome_hand_track_handle(DRONE_CONTEXT * p, int cmd, S16 dotx, S16 doty)
{
    double a, b;
    if(p->drone_mode == 0)
    {
        a  = ((double)dotx/(double)VI_CODEC_WIDTH_HALF)*(double)p->vi_fov_x[0] *(double)100;
        b  = ((double)doty/(double)VI_CODEC_HEIGHT_HALF)*(double)p->vi_fov_y[0] *(double)100;


        p->dome_azimuth_angle = (S16)a;
        p->dome_altitude_angle = (S16)b;

    }else
    {
        a  = ((double)dotx/(double)IR_CODEC_WIDTH_HALF)*(double)p->ir_fov_x[0] *(double)100;
        b  = ((double)doty/(double)IR_CODEC_HEIGHT_HALF)*(double)p->ir_fov_y[0] *(double)100;

        p->dome_azimuth_angle = (S16)a;
        p->dome_altitude_angle = (S16)b;

    }

    drone_send_update_state_with_par(p, cmd);
  //drone_send_update_state(cmd, a, b);

    return 0;
}

void drone_cal_sight_value(DRONE_CONTEXT * p)
{
     S16 symbol;
     U8 temp;
    // S16 s16_temp;
    // U16 u16_temp;
     
     symbol =  p->data_buffer[3] & 0x80;
     symbol = symbol == 0 ? 1 : -1;
     temp = p->data_buffer[3] & 0x7f;
     temp = (U8)((float)temp / 60.0f * 100.0f); 
     p->sight_pitch = p->data_buffer[4] * 100 + temp; 
     p->sight_pitch = p->sight_pitch * symbol;


     symbol =  p->data_buffer[5] & 0x80;
     symbol = symbol == 0 ? 1 : -1;
     temp = p->data_buffer[5] & 0x7f;
     temp = (U8)((float)temp / 60.0f * 100.0f); 
     p->sight_azimuth = p->data_buffer[6] * 100 + temp;
     if(symbol == -1)
     {
        p->sight_azimuth += 18000;
     }

     printf("p=%d, z = %d\n",p->sight_pitch,p->sight_azimuth);
}
void drone_sight_fc_relative_position( DRONE_CONTEXT * p)
{
    KERNEL_MESSAGE msg  ;

    S16 pitch,azimuth;
    pitch = p->sight_pitch - p->plane_pitch_angle;
    azimuth = p->sight_azimuth - p->plane_course_angle;
    if(azimuth <0)
    {
        azimuth += 36000;
    }
    core_fill_message( (MESSAG_HEADER*)&msg, APP_DRONE, APP_DRONE, DRONE_DOME_DATA_LEAD, sizeof(MESSAG_HEADER) + 4 );
    msg.data[0] = azimuth & 0xff; 
    msg.data[1] = ( azimuth >> 8 )& 0xff;
    msg.data[2] = pitch & 0xff;
    msg.data[3] = ( pitch >> 8) & 0xff;
    core_send_message( (KERNEL_MESSAGE *)&msg ) ;
}
#define HEIGHT_SCALE 0.2f 
static void calculate_plane_angle_position(DRONE_CONTEXT * p)
{
       //1.cal angle
       p->plane_pitch_angle = (p->data_buffer[12]<<8)|p->data_buffer[11];
       p->plane_roll_angle = (p->data_buffer[10]<<8)|p->data_buffer[9];
       p->plane_course_angle = (p->data_buffer[14]<<8)|p->data_buffer[13];

       //2.cal position
       memcpy( &p->plane_longitude, p->data_buffer + 15, sizeof(int) );
       p->fc_longitude = (float)p->plane_longitude * 1e-7;
       if(p->plane_longitude > 0 )
       {
           p->fc_ns = 'N';
       }
       else
       {
           p->fc_ns = 'S';
           p->fc_longitude = p->fc_longitude * -1;
       }
       
       memcpy( &p->plane_latitude, p->data_buffer + 19, sizeof(int) );  
       p->fc_latitude = (double)(p->plane_latitude * 1e-7);
       if( p->plane_latitude > 0 )
       {
           p->fc_we = 'E'; //"E"
       }
       else
       {
           p->fc_we = 'W'; //"W";
           p->fc_latitude = p->fc_latitude * -1;
       }
       memcpy( &p->plane_altitude, p->data_buffer + 23, sizeof(short) );    
       p->fc_height = (float)p->plane_altitude * HEIGHT_SCALE ;


}

/////////////////////////////////////////////////////////
//     Function : drone_fc_do_serial_protocol_demo
//  Description :
//        Input :
//       Output :
static void drone_fc_do_serial_protocol_demo( DRONE_CONTEXT * p )
{
    KERNEL_MESSAGE msg  ;
    MESSAG_HEADER *head = (MESSAG_HEADER *)&msg ;
    ENCODER_H264_PARAMETER_MSG* enc = (ENCODER_H264_PARAMETER_MSG*)&msg;
    TRACKER_OBJ_OPERATION_MSG * tracker = (TRACKER_OBJ_OPERATION_MSG *)&msg ;

   // char buffer[13];
    short x, y;
    int mode,  step=0;
    float dotx, doty;

    calculate_plane_angle_position(p);

    switch(p->data_buffer[2])
    {

    case FC_VIDEO_SWITCH_VI :
        printf("switch vi\n");
        if( p->drone_mode == 0 )
            break;
        drone_fc_send_track_stop_cmd(p);
        core_do_command( APP_DRONE , DRONE_MODE_SWITCH, 0 , NULL , NULL ) ;
        break;

    case FC_VIDEO_SWITCH_IR :
        if( p->drone_mode == 1)
            break;
        drone_fc_send_track_stop_cmd(p);
        core_do_command( APP_DRONE , DRONE_MODE_SWITCH, 0 , NULL , NULL ) ;
        break;
        
    case FC_POLARITY_BLACK:
        if(p->latest_cmd == p->data_buffer[2])
                  break;

        if(p->drone_mode ==0)
            core_do_command( APP_DRONE , DRONE_MODE_SWITCH, 0 , NULL , NULL ) ;
        
        printf("switch ir blacke\n");
        drone_fc_send_track_stop_cmd(p);
        core_do_command( APP_DRONE , DRONE_IR_BLACK_HEAT , sizeof(int) , &mode , NULL ) ;
        break;     
        
    case FC_POLARITY_WHITE:
        if(p->latest_cmd == p->data_buffer[2])
            break;
        
        if(p->drone_mode ==0)
            core_do_command( APP_DRONE , DRONE_MODE_SWITCH, 0 , NULL , NULL ) ;

        printf("switch ir white\n");
        drone_fc_send_track_stop_cmd(p);
        core_do_command( APP_DRONE , DRONE_IR_WHITE_HEAT , sizeof(int) , &mode , NULL ) ;
        break;
    case FC_IMAGE_ENHANCE_TURN_ON:
        if(p->drone_mode == 1)
            LGC6122_operation_word_0x02(0x1a,0x01);
        else
             HAL_drv_ioctl( p->vidrv , SYSVIN_VI_PORT , 0 , OP_DEHAZE_AUTO_MODE,60, NULL ) ;
        break;
        
    case FC_IMAGE_ENHANCE_TURN_OFF:
        
        if(p->drone_mode == 1)
            LGC6122_operation_word_0x02(0x1a,0x00);
        else
             HAL_drv_ioctl( p->vidrv , SYSVIN_VI_PORT , 0 , OP_DEHAZE_DISABLE,50, NULL ) ;
        break;

    case FC_AI_TURN_ON:
        p->ai_state = 1;
        printf("---ai start--\n");
        drone_fc_send_track_stop_cmd(p);
        core_do_command( APP_AI, AI_START, sizeof(float), &p->coe[p->zoom], NULL ) ;
        break;

    case FC_AI_TURN_OFF:
        p->ai_state = 0;
        printf("---ai stop--\n");
        core_do_command( APP_AI, AI_STOP, 0, 0, NULL ) ;
        break;
        
    case FC_AI_SELF_ADAPTION:
        p->ai_type = 0x00;
        mode = OBJ_ALL;
        core_do_command( APP_AI, AI_SET_OBJECT_CLASS, sizeof(int),&mode , NULL ) ;
        break;
        
    case FC_AI_PEOPLE:
        mode = p->drone_mode == 0 ? OBJ_PERSON_VI : OBJ_PERSON_IR;
        printf("---ai person--\n");
        p->ai_type = 0x01;
        core_do_command( APP_AI, AI_SET_OBJECT_CLASS, sizeof(int),&mode , NULL ) ;
        break;

    case FC_AI_CAR:
        mode = p->drone_mode == 0 ? OBJ_CAR_VI : OBJ_CAR_IR;
        printf("---ai car--\n");
        p->ai_type = 0x10;
        core_do_command( APP_AI, AI_SET_OBJECT_CLASS, sizeof(int),&mode , NULL ) ;
        break;    
        
    case FC_START_REC:
        core_do_command( APP_DRONE , DRONE_RECORDER_START , 0 , NULL, NULL ) ;
        break;

    case FC_STOP_REC:
        core_do_command( APP_DRONE , DRONE_RECORDER_STOP , 0 , NULL, NULL ) ;
        break;
    
    case FC_DOME_AUTO_TRACK:
        x = (p->data_buffer[4]<<8)| (p->data_buffer[3]);
        y = (p->data_buffer[6]<<8)| (p->data_buffer[5]);
        y = -y;
        if(p->track_old_x == x && p->track_old_y == y )
            break;

        drone_fc_send_track_stop_cmd(p);
        core_fill_message( head, APP_DRONE, APP_TRACKER, TRACKER_START_TRACK_0, sizeof(TRACKER_OBJ_OPERATION_MSG) );

        p->track_old_x = x;
        p->track_old_y = y;

        dotx = (float)x/p->coe[p->zoom];
        doty = (float)y/p->coe[p->zoom];
    
        printf("FC_DOME_AUTO_TRACK CMD(0X0d): x = %d, y = %d , zoom = %d dotx : %f  doty: %f \n", x, -y, p->zoom+1, dotx, doty);
    
        p->track_state = 1;
        tracker->x = (int)dotx;
        tracker->y = (int)doty;

        tracker->w = 0 ;
        tracker->h = 0 ;
        tracker->frame_id = p->data_buffer[7];
        tracker->zoom = p->coe[p->zoom];
        core_send_message( &msg ) ;

        p->dome_state = 0;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;
        
    case FC_DOME_STOP_TRACK:
        drone_fc_send_track_stop_cmd(p);
        drone_send_update_state(DRONE_DOME_STOP_TRACK_MODE);
        p->track_old_x = 10000;
        p->track_old_y = 10000;
        LOG_PRINTF("------FC_STOP_TRACK----");
        break;

        
    case FC_DOME_ZOOM:
        mode = 0;
        mode = p->data_buffer[8] / 10 - 1;
        if( mode > COE_NUM || mode <0)
            break;
        printf("---zoom = %d --\n",mode);
        core_do_command( APP_DRONE , DRONE_ZOOM_SET , sizeof(int) , &mode , NULL ) ;
        break;

    case FC_DOME_HAND_ANGLE_MODE:
        drone_fc_send_track_stop_cmd(p);
        p->dome_azimuth_angle_rate = (p->data_buffer[4]<<8)|p->data_buffer[3];
        p->dome_altitude_angle_rate = (p->data_buffer[6]<<8)|p->data_buffer[5];

        drone_send_update_state(DRONE_DOME_HAND_SPEED);

        p->dome_state = 1;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;
    case FC_DOME_ZOOM_RATE:
        step = p->data_buffer[8];
        step = step + (p->data_buffer[8] << 8);
        printf("step = %d\n",step);
        core_do_command(APP_TRACKER, TRACKER_SET_SHAPE_SIZE, sizeof(int), &step, NULL);
        break;
        
    case FC_DOME_DATA_LEAD_MODE:
        drone_fc_send_track_stop_cmd(p);
        p->dome_azimuth_angle = (p->data_buffer[4]<<8)|p->data_buffer[3];
        p->dome_altitude_angle= (p->data_buffer[6]<<8)|p->data_buffer[5];


        LOG_PRINTF(" CMD(0X26)  dome_altitude_angle = %d, dome_azimuth_angle = %d  ", p->dome_altitude_angle, p->dome_azimuth_angle);
        drone_send_update_state_with_par(p, DRONE_DOME_DATA_LEAD);
        
        p->dome_state = 5;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;
    case FC_DOME_SUPPESS_GYRO_DRIFT:
         drone_send_update_state(DRONE_DOME_SUPPRESS_GYRO_DIRFT);
        break;
    case FC_MOTOR_TRUN_ON:
        p->motor_state = 1;
        drone_send_update_state(DRONE_DOME_MOTOR_TURN_ON);
        break;
    case FC_MOTOR_TRUN_OFF:
        p->motor_state = 0;
        drone_send_update_state(DRONE_DOME_MOTOR_TURN_OFF);
        break;
    
    case FC_TRACKER_TRUN_ON:
        p->tracker_state = 1;
        drone_send_update_state(DRONE_DOME_TRACKER_TRUN_ON);
        break;
    case FC_TRACKER_TRUN_OFF:
        p->tracker_state = 0;
        drone_send_update_state(DRONE_DOME_TRACKER_TRUN_OFF);
        break;

    case FC_ELECTRIC_TURN_ON:
        p->electric_lock = 1;
        drone_fc_send_track_stop_cmd(p);
        p->track_old_x = 10000;
        p->track_old_y = 10000;
        printf("electric turn on\n");
        drone_send_update_state(DRONE_DOME_ELECTRIC_TRUN_ON);
        break;
    case FC_ELECTRIC_TURN_OFF:
        p->electric_lock = 0;
        drone_send_update_state(DRONE_DOME_ELECTRIC_TRUN_OFF);
        break;
    case FC_GUN_AIM_FOLLOW_OPEN://0x41
        p->gun_aim_state = 1;
        drone_fc_send_track_stop_cmd(p);
        p->track_old_x = 10000;
        p->track_old_y = 10000;
        drone_cal_sight_value(p);
        drone_sight_fc_relative_position(p);
        
        break;
    case FC_GUN_AIM_FOLLOW_CLOSE://0x42
        p->gun_aim_state = 0;

        break;
    case FC_TRACKER_BOX_ADD:
        core_do_command(APP_TRACKER, TRACKER_BOX_ADD, 0, NULL, NULL);
        break;
    
    case FC_TRACKER_BOX_SUB:
        core_do_command(APP_TRACKER, TRACKER_BOX_SUB, 0, NULL, NULL);
        break;
    
    case FC_TRACKER_BOX_MANUAL_SET:
        step = p->data_buffer[3];
        step = step + (p->data_buffer[5] << 8);
        printf("step = %x\n",step);
        core_do_command(APP_TRACKER, TRACKER_SET_SHAPE_SIZE, sizeof(int), &step, NULL);
        break;
        
    case FC_TRACKER_BOX_MOVE_LEFT:
        core_do_command(APP_TRACKER, TRACKER_BOX_MOVE_LEFT, 0, NULL, NULL);
        break;
    case FC_TRACKER_BOX_MOVE_RIGHT:
        core_do_command(APP_TRACKER, TRACKER_BOX_MOVE_RIGHT, 0, NULL, NULL);
        break;
    case FC_TRACKER_BOX_MOVE_UP:
        core_do_command(APP_TRACKER, TRACKER_BOX_MOVE_UP, 0, NULL, NULL);
        break;
    case FC_TRACKER_BOX_MOVE_DOWN:
        core_do_command(APP_TRACKER, TRACKER_BOX_MOVE_DOWN, 0, NULL, NULL);
        break;

    case FC_CROSS_MOVE_LEFT:
        core_do_command(APP_DRONE, DRONE_CROSS_LEFT, 0, NULL, NULL);
        break;
    case FC_CROSS_MOVE_RIGHT:
        core_do_command(APP_DRONE, DRONE_CROSS_RIGHT, 0, NULL, NULL);
        break;
    case FC_CROSS_MOVE_UP:
        core_do_command(APP_DRONE, DRONE_CROSS_UP, 0, NULL, NULL);
        break;
    case FC_CROSS_MOVE_DOWN:
        core_do_command(APP_DRONE, DRONE_CROSS_DOWN, 0, NULL, NULL);
        break;
    case FC_CROSS_SAVE:
        core_do_command(APP_DRONE, DRONE_CROSS_SAVE, 0, NULL, NULL);
        break;
    case FC_IR_IMAGE_ADJUST://0x71
        LGC6122_adjust(0x01);
        break;
    
    case FC_BRIGHT_INCREASE:
        step = 5;
        core_do_command( APP_DRONE , DRONE_BRIGHT_ADJUST , sizeof(int) , &step , NULL ) ;
        break;

    case FC_BRIGHT_DECREASE:
        step = -5;
        core_do_command( APP_DRONE , DRONE_BRIGHT_ADJUST , sizeof(int) , &step , NULL ) ;
        break;

    case FC_CONTRAST_INCRESE:
        step = 5;
        core_do_command( APP_DRONE , DRONE_CONTRAST_ADJUST , sizeof(int) , &step , NULL ) ;
        break;

    case FC_CONTRAST_DECRESE:
        step = -5;
        core_do_command( APP_DRONE , DRONE_CONTRAST_ADJUST , sizeof(int) , &step , NULL ) ;
        break;

    case FC_CORRECT_CONTROL:
        drone_fc_send_shutter_correction();
        break;

    case FC_START_SERIES_REC:
        core_do_command( APP_DRONE , DRONE_START_SERIES_PIC , 0 , NULL, NULL ) ;
        break;

    case FC_STOP_SERIES_REC:
        core_do_command( APP_DRONE , DRONE_STOP_SERIES_PIC , 0 , NULL, NULL ) ;
        break;
    
    case FC_ZOOM_INCREASE:
        if(p->current_zoom == COE_NUM)
            p->zoom = 0;
        else
            p->zoom++;

        p->current_zoom = p->zoom;

        core_do_command( APP_DRONE , DRONE_ZOOM_SET , sizeof(int) , &p->zoom , NULL ) ;
        break;



    case FC_DIGI_ZOOM_1X:
        p->zoom = 0;
        p->current_zoom = 0;
        core_do_command( APP_DRONE , DRONE_ZOOM_SET , sizeof(int) , &p->zoom , NULL ) ;
        break;

    case FC_DIGI_ZOOM_2X:
        p->zoom = 1;
        p->current_zoom = 1;
        core_do_command( APP_DRONE , DRONE_ZOOM_SET , sizeof(int) , &p->zoom , NULL ) ;
        break;

    case FC_DIGI_ZOOM_3X:
        p->zoom = 2;
        p->current_zoom = 2;
        core_do_command( APP_DRONE , DRONE_ZOOM_SET , sizeof(int) , &p->zoom , NULL ) ;
        break;

    case FC_DIGI_ZOOM_4X:
        p->zoom = 3;
        p->current_zoom = 3;
        core_do_command( APP_DRONE , DRONE_ZOOM_SET , sizeof(int) , &p->zoom , NULL ) ;
        break;

    case FC_DIGI_ZOOM_5X:
        p->zoom = 4;
        p->current_zoom = 4;
        core_do_command( APP_DRONE , DRONE_ZOOM_SET , sizeof(int) , &p->zoom , NULL ) ;
        break;

    case FC_FONT_SHOW:
        core_do_command( APP_DRONE , DRONE_FONT_SHOW , 0 , NULL, NULL ) ;
        break;

    case FC_VIDEO_MODE_2M:
        p->bitrate_mode   = 0;
        enc->channel      = VENC_VI_CHANNEL  ;
        enc->h264.bitrate = (int)p->bitrate[0];
        core_fill_message( head, APP_USER1, SYS_CODEC, SYSENC_SET_H264_PARAM, sizeof(ENCODER_H264_PARAMETER_MSG) );
        core_send_message( &msg ) ;
        enc->channel = VENC_IR_CHANNEL ;
        core_send_message( &msg ) ;

        if(p->drone_mode==0)
        {
            p->vi_bitrate = enc->h264.bitrate;
            shmcfg_set_integer( p->shmcfg, "VIDEO_VISABLE","Bitrate", enc->h264.bitrate );
        }else
        {
            p->ir_bitrate = enc->h264.bitrate;
            shmcfg_set_integer( p->shmcfg, "VIDEO_IRIMAGE","Bitrate", enc->h264.bitrate );
        }

        shmcfg_flush( p->shmcfg );
        break;

    case FC_VIDEO_MODE_4M:
        p->bitrate_mode   = 1;
        enc->channel      = VENC_VI_CHANNEL  ;
        enc->h264.bitrate = (int)p->bitrate[1];
        core_fill_message( head, APP_USER1, SYS_CODEC, SYSENC_SET_H264_PARAM, sizeof(ENCODER_H264_PARAMETER_MSG) );
        core_send_message( &msg ) ;
        enc->channel = VENC_IR_CHANNEL ;
        core_send_message( &msg ) ;

        if(p->drone_mode==0)
        {
            p->vi_bitrate = enc->h264.bitrate;
            shmcfg_set_integer( p->shmcfg, "VIDEO_VISABLE","Bitrate", enc->h264.bitrate );
        }else
        {
            p->ir_bitrate = enc->h264.bitrate;
            shmcfg_set_integer( p->shmcfg, "VIDEO_IRIMAGE","Bitrate", enc->h264.bitrate );
        }

        shmcfg_flush( p->shmcfg );

        break;

    case FC_VIDEO_MODE_8M:
        p->bitrate_mode   = 2;
        enc->channel      = VENC_VI_CHANNEL  ;
        enc->h264.bitrate = (int)p->bitrate[2];
        core_fill_message( head, APP_USER1, SYS_CODEC, SYSENC_SET_H264_PARAM, sizeof(ENCODER_H264_PARAMETER_MSG) );
        core_send_message( &msg ) ;
        enc->channel = VENC_IR_CHANNEL ;
        core_send_message( &msg ) ;

        if(p->drone_mode==0)
        {
            p->vi_bitrate = enc->h264.bitrate;
            shmcfg_set_integer( p->shmcfg, "VIDEO_VISABLE","Bitrate", enc->h264.bitrate );
        }else
        {
            p->ir_bitrate = enc->h264.bitrate;
            shmcfg_set_integer( p->shmcfg, "VIDEO_IRIMAGE","Bitrate", enc->h264.bitrate );
        }

        shmcfg_flush( p->shmcfg );
        break;

    case FC_VIDEO_MODE_12M:
        p->bitrate_mode   = 3;
        enc->channel      = VENC_VI_CHANNEL  ;
        enc->h264.bitrate = (int)p->bitrate[3];
        core_fill_message( head, APP_USER1, SYS_CODEC, SYSENC_SET_H264_PARAM, sizeof(ENCODER_H264_PARAMETER_MSG) );
        core_send_message( &msg ) ;
        enc->channel = VENC_IR_CHANNEL ;
        core_send_message( &msg ) ;

        if(p->drone_mode==0)
        {
            p->vi_bitrate = enc->h264.bitrate;
            shmcfg_set_integer( p->shmcfg, "VIDEO_VISABLE","Bitrate", enc->h264.bitrate );
        }else
        {
            p->ir_bitrate = enc->h264.bitrate;
            shmcfg_set_integer( p->shmcfg, "VIDEO_IRIMAGE","Bitrate", enc->h264.bitrate );
        }

        shmcfg_flush( p->shmcfg );

        break;

    case FC_ID_SET:
        p->load_id = p->data_buffer[4]<<8| p->data_buffer[3];
        shmcfg_set_integer( p->shmcfg, "DRONE", "LoadID", p->load_id );
        shmcfg_flush( p->shmcfg );
        break;

   // dome control
    case FC_DOME_HAND_POSITION_MODE:
        drone_fc_send_track_stop_cmd(p);
        y = (p->data_buffer[4]<<8)|p->data_buffer[3];
        x = (p->data_buffer[6]<<8)|p->data_buffer[5];
        p->dome_azimuth_angle_rate  = 0;
        p->dome_altitude_angle_rate = 0;

        dotx = (float)x/p->coe[p->zoom];
        doty = (float)y/p->coe[p->zoom];

        LOG_PRINTF(" FC_DOME_HAND_POSITION_MODE CMD(0X18)  dotx = %d, doty = %d  ", (S16)dotx, (S16)doty);
        drone_dome_hand_track_handle(p, DRONE_DOME_HAND_TRACE, (S16)dotx, (S16)doty);

        p->dome_state = 1;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;


    case FC_DOEM_SCAN_MODE:  // only command
        drone_fc_send_track_stop_cmd(p);
        drone_send_update_state(DRONE_DOME_SCAN_MODE);

        p->dome_state = 2;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case FC_DOME_CENTRE_MODE: // only command
        drone_fc_send_track_stop_cmd(p);


        p->dome_azimuth_angle       = 0;
        p->dome_azimuth_angle_rate  = 0;
        p->dome_altitude_angle      = 0;
        p->dome_altitude_angle_rate = 0;
        
        drone_send_update_state(DRONE_DOME_CENTRA_BACK);
        
        LOG_PRINTF("----FC_DOME_CENTRE_MODE------");

        p->dome_state = 3;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case FC_DOME_BIRD_VIEW_MODE: // only command
        drone_fc_send_track_stop_cmd(p);
        
        drone_send_update_state(DRONE_DOME_VERTICAL_DOWN);

        p->dome_state = 4;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;



    case FC_DOME_GEOGRAPY_MODE:
        drone_fc_send_track_stop_cmd(p);

        drone_send_update_state(DRONE_DOME_GEOGROPY_LEAD);

        p->dome_state = 5;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case FC_DOME_PROTECT_MODE: // only command
        drone_fc_send_track_stop_cmd(p);

        drone_send_update_state(DRONE_DOME_PROTECT_MODE);

        p->dome_state = 6;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;



    case FC_LASER_TURN_ON:
        core_do_command( APP_DRONE , DRONE_LASER_TURN_ON , 0 , NULL , NULL ) ;
        p->laser_cmd = 1;
        drone_send_update_state_with_par(p, DRONE_DOME_LASER_TURN_ON);
        LOG_PRINTF("------FC_LASER_TURN_ON----");
        break;

    case FC_LASER_TURN_OFF:
        core_do_command( APP_DRONE , DRONE_LASER_TURN_OFF , 0 , NULL , NULL ) ;
        p->laser_cmd = 0;
        drone_send_update_state_with_par(p, DRONE_DOME_LASER_TURN_OFF);
        LOG_PRINTF("----FC_LASER_TURN_OFF--------");
        break;

    case FC_NULL_CMD:
 //     drone_dome_send_buffer( p , DOME_HAND_SPEED, 0 ) ;
        break;

    case FC_DEBUG_CONTROL_CMD:
        core_do_command( p->data_buffer[3] , (p->data_buffer[3] << 16) + p->data_buffer[4] ,
                         2 , p->data_buffer + 5 , NULL ) ;
        break;

    default:
        break;
    }
    
    p->latest_cmd = p->data_buffer[2];
    
}

/////////////////////////////////////////////////////////
//     Function : drone_get_state_words
//  Description :
//        Input :
//       Output :
U16 drone_get_state_words(DRONE_CONTEXT * p, U8* word0, U8* word1)
{
    char zoom[] = {0b001,0b010, 0b011,0b100,0b101};
    char bitrate_mode[] = {0b00, 0b01, 0b10, 0b11};
    char track[] = {0b000, 0b100};
    char video[] = {0b101, 0b110, 0b111};
    char laser[] = {0b10, 0b01, 0x00, 0x11}; // off/on/preserved/fault

    *word0 = (video[p->video_type] << 5) | (p->rec_mode << 3) | track[p->track_state];
    *word1 = (laser[p->laster_state] << 6) |(zoom[p->zoom] << 3) | bitrate_mode[p->bitrate_mode];

    return 0;
}

/////////////////////////////////////////////////////////
//     Function : drone_get_state_value
//  Description :
//        Input :
//       Output :
int drone_get_state_value(DRONE_CONTEXT * drone, STATE_PARAMETER* t )
{
    int rate;

    drone_get_state_words(drone, &t->state_word0, &t->state_word1);

    if(drone->drone_mode == 0) // vi
    {
        t->resolution_x = drone->vi_resolution_x;
        t->resolution_y = drone->vi_resolution_y;
        t->pixel        = drone->vi_sensor_pixel;  
        t->sensor_x     = t->pixel * drone->vi_sensor_x/1000;
        t->sensor_y     = t->pixel * drone->vi_sensor_y/1000;
        t->len          = drone->len_vi;
        t->frame_rate   = drone->vi_frame_rate;
        rate            = drone->vi_bitrate*10/1024/1024;
        t->bitrate      = rate;
        t->actual_x     = (float)drone->sensor_actual_x[drone->zoom];
        t->actual_y     = (float)drone->sensor_actual_y[drone->zoom];
    }else  // ir
    {
        t->resolution_x = drone->ir_resolution_x;
        t->resolution_y = drone->ir_resolution_y;
        t->pixel           = drone->ir_detector_pixel;
        t->sensor_x     = t->pixel * drone->ir_detector_x/1000;
        t->sensor_y     = t->pixel * drone->ir_detector_y/1000;
        t->len          = drone->len_ir;
        t->frame_rate   = drone->ir_frame_rate;
        rate            = drone->ir_bitrate*10/1024/1024;
        t->bitrate      = rate;
        t->actual_x     = (float)drone->detector_actual_x[drone->zoom];
        t->actual_y     = (float)drone->detector_actual_y[drone->zoom];
    }

    return 0;
}

#define  PI  0.01745329  //pai/180
void drone_cal_target_Info(DRONE_CONTEXT* p) 
{
//#define DEBUG
#ifdef DEBUG   
        p->plane_pitch_angle = -1000;
        p->plane_course_angle = 1000;
        p->fc_longitude = 100;
        p->fc_latitude = 100;
        p->fc_height = 100;
        p->laser_distance = 2000;
        p->fc_ns = 'N';
        p->fc_we = 'E';
#endif    
    float lat_temp, lon_temp, dis, fy, fw;
    dis = (float)p->laser_distance / 10.0;
    fy = (float)(p->plane_pitch_angle + p->dome_pitch) / 100.0f;
    if(p->plane_course_angle > p->dome_azimuth)
    {
        fw = (float)(p->plane_course_angle - p->dome_azimuth) / 100.0f;    
    }
    else
    {
        fw = (float)(p->dome_azimuth - p->plane_course_angle) / 100.0f;   
    }
    //1.计算目标海拔高度
	p->aim_height = p->fc_height + dis * sin( fy * PI);
  

    //2.把目标在经、纬度上的距离值转化为经纬度数
    //经线等长：纬度111km约为1° 
	lat_temp = ( dis * cos( fy * PI) * cos((fw) * PI)) / (111 * 1000);     
    //纬线渐短，纬度A上的精度：111*cosA km 约为1°；
	lon_temp = ( dis * cos( fy * PI) * sin((fw) * PI)) / (111 * cos(p->fc_latitude * PI) * 1000); 

    //根据拿设备的人所在半球  计算目标位置
    if( p->fc_we == 'E' )
    {
        //东半球  北半球
		if (p->fc_ns == 'N') 
        {    
			p->aim_latitude = p->fc_latitude + lat_temp;
			p->aim_longitude = p->fc_longitude + lon_temp;
            
            //判断目标位置是否超过半球分界线
			if (p->aim_latitude < 0) //纬度0-90，只存在小于的情况
            {
				p->aim_ns = 'S';
				p->aim_latitude = 0 - p->aim_latitude;
			}
			else
			{
				p->aim_ns = 'N';
			}

        }
        else if (p->fc_ns == 'S') //东半球  南半球
        {
			p->aim_latitude = p->fc_latitude - lat_temp;
			p->aim_longitude = p->fc_longitude + lon_temp;
			
            //判断目标位置是否超过半球分界线
			if (p->aim_latitude < 0) { //纬度0-90，只存在小于的情况
				p->aim_ns = 'N';
				p->aim_latitude = 0 - p->aim_latitude;
			}
			else
			{
				p->aim_ns = 'S';
			}
		}        

         //判断目标位置是否超过半球分界线
		if (p->aim_longitude > 180)  //超过180°经线
		{
			p->aim_we = 'W';
			p->aim_longitude = 360 - p->aim_longitude;
		}
		else if (p->aim_longitude < 0)   //越过0°经线
		{
			p->aim_we = 'W';
			p->aim_longitude = 0 - p->aim_longitude;
		}
		else
		{
			p->aim_we = 'E';
		}
    }
    else if ( p->fc_we == 'W') //西半球
    {
		if ( p->fc_ns == 'N') 
		{
			p->aim_latitude = p->fc_latitude + lat_temp;
			p->aim_longitude = p->fc_longitude - lon_temp;

            //判断目标位置是否超过半球分界线
			if (p->aim_longitude < 0)  //纬度0-90，只存在小于的情况
			{
				p->aim_ns = 'S';
				p->aim_latitude = 0 - p->aim_latitude;
			}
			else
			{
				p->aim_ns = 'N';
			}
		}
		else if (p->fc_ns == 'S') //西半球  南半球
		{
			p->aim_latitude = p->fc_latitude - lat_temp;
			p->aim_longitude = p->fc_longitude - lon_temp;

            //判断目标位置是否超过半球分界线
			if (p->aim_latitude < 0)  //纬度0-90，只存在小于的情况
			{
				p->aim_ns = 'N';
				p->aim_latitude = 0 - p->aim_latitude;
			}
			else
			{
				p->aim_ns = 'S';
			}
		}
        //判断目标位置是否超过半球分界线
		if (p->aim_longitude > 180)    // 超过180°经线
		{
			p->aim_we = 'E';
			p->aim_longitude = 360 - p->aim_longitude;
		}
		else if (p->aim_longitude < 0)  //越过0°经线
		{
			p->aim_we = 'E';
			p->aim_longitude = 0 - p->aim_longitude;
		}
		else
		{
			p->aim_we = 'W';
		}

    }

}

#define AIM_HEIGHT_RATIO    2
/////////////////////////////////////////////////////////
//     Function : drone_send_fc
//  Description :
//        Input :
//       Output :
void drone_send_fc_msg(DRONE_CONTEXT* p)
{
    int symbol = 1;
    U8 buffer[50] = {0};
    U8 temp = 0;
    
    buffer[0] = 0xee;
    buffer[1] = 0x16;
    if(p->drone_mode != 0)
    {
        temp = p->ir_mode == 0 ? 0x02 : 0x03; 
        buffer[2] = buffer[2] | ( temp << 6 );
    }
     
    buffer[2] = buffer[2] | (p->ai_type << 4);
    
    buffer[3] = buffer[3] | (p->image_enhance << 7);
    buffer[3] = buffer[3] | (p->rec_mode << 5) | (p->track_state << 2);
    buffer[3] = buffer[3] | (p->motor_state << 3);
    buffer[3] = buffer[3] | (p->tracker_state << 2);
    buffer[3] = buffer[3] | (p->electric_lock << 1);
    buffer[3] = buffer[3] | (p->gun_aim_state & 0x01);

    buffer[4] = (U8)(p->zoom+1);

    buffer[5]= p->dome_roll_state << 7;
    buffer[5]=  buffer[5] | p->dome_pitch_state << 6;
    buffer[5]=  buffer[5] | p->dome_azimuth_state << 5;
    buffer[5]=  buffer[5] | p->laster_state << 2;

    memcpy(buffer + 6, &p->target_x, 2);
    memcpy(buffer + 8, &p->target_y, 2);
    
    buffer[10] = p->dome_recv_buffer[6]; //roll frame angle
    buffer[11] = p->dome_recv_buffer[7];
    buffer[12] = p->dome_recv_buffer[8]; //pitch frame angle memcpy(buffer + 12, &p->gd_angle, 2);
    buffer[13] = p->dome_recv_buffer[9];
    buffer[14] = p->dome_recv_buffer[4]; //azimuth frame angle memcpy(buffer + 14, &p->fw_angle, 2);
    buffer[15] = p->dome_recv_buffer[5]; 
    
    buffer[16] = p->dome_recv_buffer[22]; //roll attitude angle  memcpy(buffer + 16, &p->plane_roll_angle, 2);
    buffer[17] = p->dome_recv_buffer[23];
    buffer[18] = p->dome_recv_buffer[24]; //pitch attitude angle memcpy(buffer + 18, &p->plane_pitch_angle, 2);
    buffer[19] = p->dome_recv_buffer[25];
   // buffer[20] = p->dome_recv_buffer[20]; //azimuth attitude angle memcpy(buffer + 20, &p->plane_course_angle, 2);
  //  buffer[21] = p->dome_recv_buffer[21]; 

    buffer[20] = p->dome_recv_buffer[16]; //roll angular velocity
    buffer[21] = p->dome_recv_buffer[17];
    buffer[22] = p->dome_recv_buffer[18]; //pitch angular velocity
    buffer[23] = p->dome_recv_buffer[19];
    buffer[24] = p->dome_recv_buffer[14]; //azimuth angular velocity
    buffer[25] = p->dome_recv_buffer[15]; 
 
    if(p->dome_recv_buffer[10] == 0xff && p->dome_recv_buffer[11] == 0x7f)
    {
       memset(buffer + 26 , 0, 2);
    }
    else
    {
        memcpy(buffer + 26, p->dome_recv_buffer + 10, 2);//Laser ranging value
    }
    //Target Dimension buffrt[]
    if( p->laser_cmd == 1 ){
        drone_cal_target_Info(p); 
        if(p->aim_ns == 'N')
        {
            symbol = 1;
        }else
        {
            symbol = -1;
        }
        p->target_longitude = (int)(p->aim_longitude * 1e7) * symbol;
        memcpy( buffer + 28, &p->target_longitude, 4);
        
        if(p->aim_we == 'E')
        {
             symbol = 1;
        }
        else
        {
             symbol = -1;
        }
        p->target_latitude = (int)(p->aim_latitude * 1e7)* symbol;
        memcpy( buffer + 32, &p->target_latitude, 4);
        
        p->target_altitude = (int)(p->aim_height * AIM_HEIGHT_RATIO);
        memcpy( buffer + 36, &p->target_altitude, 2);
        
        printf("longitude=%lf, latitude=%lf, height=%lf\n",p->aim_longitude,p->aim_latitude,p->aim_height);
    }
    
    //Self inspection results
    buffer[47] = 1;

    //crc sum
    buffer[49] = drone_calc_checksum(buffer, 49);
    
    SERIAL_SEND_DATA(0, (void*)buffer, sizeof(buffer));
}

/////////////////////////////////////////////////////////
//     Function : drone_fc_serial_input_demo
//  Description :
//        Input :
//       Output :
static void drone_fc_serial_input_demo( U8 dat , DRONE_CONTEXT * p )
{
    U8  crc ;
   // printf("%s %x state = %x\r\n", __FUNCTION__, dat, p->packet_state);
    switch( p->packet_state )
    {
        case PACKET_STATE_SYNC1 :
            if ( dat == 0xeb )
            {
                p->packet_state = PACKET_STATE_TYPE2 ;
                p->data_buffer[0] = dat ;
            }
            break ;

        case PACKET_STATE_TYPE2 :
            if( dat == 0x90 )
            {
                p->data_buffer[1] = dat ;
                p->packet_state = PACKET_STATE_DATA1 ;
                p->data_ptr = 2;
            }else
            {
                p->packet_state = PACKET_STATE_SYNC1 ;
            }
            break ;

        case PACKET_STATE_DATA1:
            p->data_buffer[p->data_ptr++] = dat ;

            if( p->data_ptr == p->fc_protocl_len - 1 )
                p->packet_state   = FC_PACKET_STATE_CRC ;
            break ;

        case FC_PACKET_STATE_CRC :

            crc = drone_calc_checksum( p->data_buffer,p->fc_protocl_len - 1) ;
            //printf("---crc = %x, len = %d--\n",crc,p->data_ptr);
            if( crc == dat )
            {
                p->data_buffer[p->data_ptr] = dat ;
                drone_fc_do_serial_protocol_demo( p ) ;
            }
            p->packet_state   = PACKET_STATE_SYNC1 ;
            break ;
    }
}

/**********************************************
    锟斤拷锟节斤拷锟杰回碉拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷锟杰癸拷锟斤拷锟斤拷锟斤拷锟斤拷
***********************************************/
int drone_fc_serial_receive_packet ( unsigned char  *dat, int len, void *priv )
{
    DRONE_CONTEXT *packet = (DRONE_CONTEXT *)priv ;

    while ( len-- > 0 )
    {
        drone_fc_serial_input_demo( *dat++, packet );
    }
    return 1;
}

/////////////////////// SPI  ////////////////////////////

/////////////////////////////////////////////////////////
//     Function : drone_spi_transfer
//  Description :
//        Input :
//       Output :
static int drone_spi_transfer( int fd , U8* buf, int len)
{
    int ret;
    ret = write( fd , buf  , len );
    if( ret != len )
    {
        LOG_PRINTF("SPI Error , ret = %d , len = %d" , ret , len ) ;
    }
    return ret ;
}

/////////////////////////////////////////////////////////
//     Function : drone_make_spi_parameter
//  Description :
//        Input :
//       Output :
static int  drone_make_spi_parameter( DRONE_CONTEXT * drone , U8* buffer , int width, int height )
{
    U16 wcrc = 0;
    
    STATE_PARAMETER t;
    drone_get_state_value(drone, &t);

    buffer[1]  = 0xeb ; // A
    buffer[2]  = 0x90 ;
    buffer[3]  = 0x00 ; // B
    buffer[4]  = 0xff ;
    buffer[5]  = 0x00 ; // C
    buffer[6]  = 0xf2 ;
    buffer[7]  = 0x2d ; // D
    buffer[8]  = drone->spi_frame_idx ; //  ?
    buffer[9]  = 0x00 ; // F
    buffer[10] = 0x00 ;
    buffer[11] = 0x00 ;
    buffer[12] = 0x00 ; // G
    buffer[13] = 0x00 ;

    buffer[14] = 0xAA ; // G
    buffer[15] = 0x55 ;
    buffer[16]  = t.state_word0;
    buffer[17]  = t.state_word1;
    buffer[18]  = t.len & 0xff;
    buffer[19]  = (t.len >> 8) & 0xff;
    buffer[20]  = drone->gd_angle & 0xff;
    buffer[21]  = (drone->gd_angle >> 8) & 0xff;
    buffer[22]  = drone->fw_angle & 0xff;
    buffer[23]  = (drone->fw_angle >> 8) & 0xff;

    buffer[24]  = t.pixel & 0xff ;
    buffer[25]  = (t.pixel >> 8) & 0xff;

    buffer[26]  = width & 0xff ;
    buffer[27]  = (width >> 8) & 0xff;

    buffer[28]  = height & 0xff ;
    buffer[29]  = (height >> 8) & 0xff;

    buffer[30]  = t.actual_x & 0xff ;
    buffer[31]  = (t.actual_x >> 8) & 0xff;
    buffer[32]  = t.actual_y & 0xff ;
    buffer[33]  = (t.actual_y >> 8) & 0xff;

    buffer[34]  = t.sensor_x & 0xff ;
    buffer[35]  = (t.sensor_x >> 8) & 0xff;
    buffer[36]  = t.sensor_y & 0xff ;
    buffer[37]  = (t.sensor_y >> 8) & 0xff;

    buffer[38]  = t.frame_rate*2 ;
    buffer[39]  = t.bitrate ;

    buffer[40]  = drone->load_id & 0xff;
    buffer[41]  = (drone->load_id >> 8) & 0xff;

    buffer[42]  = drone->target_x & 0xff;
    buffer[43]  = (drone->target_x >> 8) & 0xff;
    buffer[44]  = drone->target_y & 0xff;
    buffer[45]  = (drone->target_y >> 8) & 0xff;

    
    buffer[46]  = drone->spi_frame_idx & 0xff;
    buffer[47]  = (drone->spi_frame_idx >> 8) & 0xff;
    buffer[48]  = (drone->spi_frame_idx >> 16) & 0xff;
    buffer[49]  = (drone->spi_frame_idx >> 24) & 0xff;


    buffer[50] = drone->laser_distance&0xFF;
    buffer[51] = (drone->laser_distance>>8)&0xFF;
    buffer[52] = (drone->laser_distance>>16)&0xFF;
    buffer[53] = (drone->laser_distance>>24)&0xFF;


    buffer[54]  = drone->free_storage & 0xff;
    buffer[55]  = (drone->free_storage >> 8) & 0xff;



    
    buffer[61]  = drone->latest_cmd;

    memcpy((void*)&buffer[62], &drone->plane_parameter[0], PLANE_PARAMETER_LEN);

  // buffer 62 ~ 96 : plane parameter
    wcrc = drone_make_crc16(&buffer[16], 96);

    buffer[112] = wcrc & 0xff;
    buffer[113] = (wcrc >> 8)&0xff;

    return 0;
}

//static char test_pattern = 0;
/////////////////////////////////////////////////////////
//     Function : drone_spi_send_stream
//  Description :
//        Input :
//       Output :
static int  drone_spi_send_stream( DRONE_CONTEXT * drone ,  SYSENC_FRAME * fr )
{
    AV_STREAM_FRAME * av = &fr->frame ;
    U8 * ptr ;

    int  count , payload_len , head_len ;

    U8   spi_header[MAX_SPI_PKT_LEN];
    int  idx = 0 , max ;

    
    if( drone->drone_mode == DRONE_MODE_VIDEO_VI && av->channel != VENC_VI_CHANNEL )
        return 0 ;

    if( drone->drone_mode == DRONE_MODE_VIDEO_IR && av->channel != VENC_IR_CHANNEL )
        return 0 ;
 
    drone->spi_frame_idx = PTS_TO_FRAME_ID(av->pts);
   //  printf("drone pts = %lld, id = %d \n", av->pts,drone->spi_frame_idx);
    //eb 90
    spi_header[idx++] = 0xeb ; // A
    spi_header[idx++] = 0x90 ;
    spi_header[idx++] = 0x00 ; // B
    spi_header[idx++] = 0xff ;
    spi_header[idx++] = 0x00 ; // C
    spi_header[idx++] = 0xf2 ;
    spi_header[idx++] = drone->drone_mode == DRONE_MODE_VIDEO_VI ? 0x21 : 0x23 ; // D
    spi_header[idx++] = drone->spi_frame_idx ; // E
   // printf("spi_frame_id = %d\n", spi_header[7]);
    spi_header[idx++] = 0x00 ; // F
    spi_header[idx++] = 0x00 ;
    spi_header[idx++] = 0x00 ;
    spi_header[idx++] = 0x00 ; // G
    spi_header[idx++] = 0x00 ;

    count        = av->length ;
    head_len     = idx ;
    payload_len  = MAX_SPI_PKT_LEN - head_len ;

    idx      = 0   ;  
    max      = MAX_SPI_BUFFER ; 
    ptr      = (U8*)av->stream ;

    while( count > 0  )
    {

        //copy spi header
        memcpy( drone->spi_buffer + idx , spi_header , head_len ) ;
        idx += head_len ;

        //copy stream data
        if( count >= payload_len )
        {
            memcpy( drone->spi_buffer + idx , ptr , payload_len  ) ;
            
            idx   += payload_len ; 
            count -= payload_len ; 
            ptr   += payload_len ;

            if( idx >= max )
            {
                 drone_spi_transfer( drone->spi_fd , drone->spi_buffer , idx ) ;            
                 idx = 0 ;
            }
            
            continue ;

        }
        
        memset( drone->spi_buffer + idx  , 0 , payload_len ) ;
        memcpy( drone->spi_buffer + idx  , ptr , count  ) ;

        idx   += payload_len ;  //
        drone_spi_transfer( drone->spi_fd , drone->spi_buffer , idx ) ;   
        
        count -= payload_len ;  //
        
    }
    //printf("frame_idx = %d, max_buff = %d, buf_len = %d \n ",drone->spi_frame_idx,MAX_SPI_BUFFER,MAX_SPI_PKT_LEN);   
    
    // 拼接飞控参数进行发送 
    drone_make_spi_parameter(drone,  drone->fc_buffer , av->width , av->height ) ;
    memcpy( drone->spi_buffer , (void*)&drone->fc_buffer[1] , MAX_SPI_PKT_LEN) ;
    drone_spi_transfer( drone->spi_fd , drone->spi_buffer , MAX_SPI_PKT_LEN ) ;

    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : drone_spi_send_info
//  Description :
//        Input :
//       Output :
static int  drone_spi_send_info( DRONE_CONTEXT * drone  )
{
    U8 * ptr ;
    U16 wcrc = 0;
    int  idx = 0 ;

    memset(drone->spi_buffer, 0, 128);

    ptr = drone->spi_buffer;

    ptr[idx++] = 0xeb ; // A
    ptr[idx++] = 0x90 ;
    ptr[idx++] = 0x00 ; // B
    ptr[idx++] = 0xff ;
    ptr[idx++] = 0xA5 ; // C
    ptr[idx++] = 0x5A ;
    ptr[idx++] = 0xF1 ;
    ptr[idx++] = 0x64 ; // 100

    wcrc = drone_make_crc16(ptr, idx);

    ptr[idx++] = wcrc & 0xff ; // CRC16.L
    ptr[idx++] = (wcrc >> 8) & 0xff ; // CRC16.H

    strcpy((char*)&ptr[12], "test");
    strcpy((char*)&ptr[44], "0001");

    drone_spi_transfer( drone->spi_fd , drone->spi_buffer , 128 ) ;

    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : drone_spi_send_ack
//  Description :
//        Input :
//       Output :
static int  drone_spi_send_ack( DRONE_CONTEXT * drone )
{
    U8 * ptr ;
    U16 wcrc = 0;
    U8   spi_header[MAX_SPI_PKT_LEN];

    memset(spi_header, 0, 128);

    ptr = spi_header;

    ptr[0]  = 0xeb ;
    ptr[1]  = 0x90 ;
    ptr[2]  = 0x00 ;
    ptr[3]  = 0xff ;
    ptr[4]  = 0xA5 ;
    ptr[5]  = 0x5A ;
    ptr[6]  = 0xF0 ;
    ptr[7]  = 0x01 ;
    ptr[8]  = 0xCC ;
    ptr[9]  = 0xCC ;
    wcrc = drone_make_crc16(ptr, 10);
    ptr[10] = wcrc & 0xff ; // CRC16.L
    ptr[11] = (wcrc >> 8) & 0xff ; // CRC16.H
    ptr[12] = drone->work_mode;

    drone_spi_transfer( drone->spi_fd , spi_header , 128 ) ;

    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : drone_spi_send_packet
//  Description :
//        Input :
//       Output :
static int  drone_spi_send_packet( DRONE_CONTEXT * drone, int from , int count  )
{
    U8 * ptr ;
    U16 wcrc = 0;
    U8   spi_header[MAX_SPI_PKT_LEN];

    int i, j;
    H264_JPG_SPI_FILE *p;
    H264_JPG_FILE* q;
    U16* total;

    memset(spi_header, 0, MAX_SPI_PKT_LEN);

    ptr = spi_header;

    ptr[0]  = 0xeb ;
    ptr[1]  = 0x90 ;
    ptr[2]  = 0x00 ;
    ptr[3]  = 0xff ;
    ptr[4]  = 0xA5 ;
    ptr[5]  = 0x5A ;
    ptr[6]  = 0xF2 ;

    if(count == 3)
        ptr[7]  = 114 ;  // length
    else if(count == 2)
        ptr[7]  = 82 ;  // length
    else if(count == 1)
        ptr[7]  = 50 ;  // length
    else
        ptr[7]  = 18 ;  // length

    ptr[8]  = 0xCC ;
    ptr[9]  = 0xCC ;

    wcrc = drone_make_crc16(ptr, 10);

    ptr[10] = wcrc & 0xff ; // CRC16.L
    ptr[11] = (wcrc >> 8) & 0xff ; // CRC16.H

    ptr[12] = drone->used && 0xff;  // used space
    ptr[13] = (drone->used>>8) && 0xff;
    ptr[14] = (drone->used>>16) && 0xff;
    ptr[15] = (drone->used>>24) && 0xff;
    ptr[16] = (drone->used>>32) && 0xff;
    ptr[17] = (drone->used>>40) && 0xff;
    ptr[18] = (drone->used>>48) && 0xff;
    ptr[19] = (drone->used>>56) && 0xff;

    ptr[20] = drone->capacity && 0xff;  // storage total space
    ptr[21] = (drone->capacity>>8) && 0xff;
    ptr[22] = (drone->capacity>>16) && 0xff;
    ptr[23] = (drone->capacity>>24) && 0xff;
    ptr[24] = (drone->capacity>>32) && 0xff;
    ptr[25] = (drone->capacity>>40) && 0xff;
    ptr[26] = (drone->capacity>>48) && 0xff;
    ptr[27] = (drone->capacity>>56) && 0xff;

    total  = (U16*)&ptr[28];
    *total = drone->max_file_count;

    j = 0;
    for( i = from ; i < drone->current_file_pos ; i++)
    {
        q = drone->h264_jpg_files + i;
        if(q->fid == INVALID_FID)
            continue;

        p = (H264_JPG_SPI_FILE*)&ptr[30+32*j++];

        INTF_make_filename( q->video , q->fid , q->channel , (char*)p->name ) ;

        if(q->video)
            strcpy((char*)p->ext, "264");
        else
            strcpy((char*)p->ext, "jpg");

        p->reserved1 = 1;
        p->reserved2 = 0;
        p->reserved3[0] = 0;
        p->reserved3[1] = 0;
        p->length = q->length ;

        count-- ;
        if( count == 0 )
            break ;
    }

    drone_spi_transfer( drone->spi_fd , spi_header , 128 ) ;

    return i + 1  ;
}

/////////////////////////////////////////////////////////
//     Function : drone_spi_send_over
//  Description :
//        Input :
//       Output :
static int  drone_spi_send_over( DRONE_CONTEXT * drone, int ack_word )
{
    U8 * ptr ;
    U16 wcrc = 0;
    U8   spi_header[MAX_SPI_PKT_LEN];

    memset(spi_header, 0, 128);

    ptr = spi_header;

    ptr[0]  = 0xeb ;
    ptr[1]  = 0x90 ;
    ptr[2]  = 0x00 ;
    ptr[3]  = 0xff ;
    ptr[4]  = 0xA5 ;
    ptr[5]  = 0x5A ;
    ptr[6]  = ack_word ;
    ptr[7]  = 0x01 ;  // length
    ptr[8]  = 0xCC ;
    ptr[9]  = 0xCC ;

    wcrc = drone_make_crc16(ptr, 10);

    ptr[10] = wcrc & 0xff ; // CRC16.L
    ptr[11] = (wcrc >> 8) & 0xff ; // CRC16.H
    ptr[12] = 0x25;

    drone_spi_transfer( drone->spi_fd , spi_header , 128 ) ;
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : drone_spi_send_list
//  Description :
//        Input :
//       Output :
static int  drone_spi_send_list( DRONE_CONTEXT * drone )
{
    int num , from ;

    num  = drone->max_file_count;
    from = 0;

    while( num > 0 )
    {
        if(num >= 3)
        {
            from = drone_spi_send_packet(drone, from , 3);
            num -= 3;
        }
        else if(num == 2)
        {
            from = drone_spi_send_packet(drone, from , 2);
            num -= 2;
        }
        else if(num == 1)
        {
            from = drone_spi_send_packet(drone, from , 1);
            num -= 1;
        }
    }

    drone_spi_send_over(drone, 0xF3);

    return 0;
}

/////////////////////////////////////////////////////////
//     Function : drone_spi_send_slice
//  Description :
//        Input :
//       Output :
static int  drone_spi_send_slice( DRONE_CONTEXT * drone, char* buffer, int size )
{
    U8 * ptr ;
    U16 wcrc = 0;
    U8   spi_header[MAX_SPI_PKT_LEN];

    memset(spi_header, 0, 128);

    ptr = spi_header;

    ptr[0]  = 0xeb ;
    ptr[1]  = 0x90 ;
    ptr[2]  = 0x00 ;
    ptr[3]  = 0xff ;
    ptr[4]  = 0xA5 ;
    ptr[5]  = 0x5A ;
    ptr[6]  = 0xF4 ;
    ptr[7]  = size ;
    ptr[8]  = 0xCC ;
    ptr[9]  = 0xCC ;

    wcrc = drone_make_crc16(ptr, 10);

    ptr[10] = wcrc & 0xff ; // CRC16.L
    ptr[11] = (wcrc >> 8) & 0xff ; // CRC16.H

    memcpy( &ptr[12], buffer, size );
    drone_spi_transfer( drone->spi_fd , spi_header , 128 ) ;

    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : drone_spi_send_file
//  Description :
//        Input :
//       Output :
static int drone_spi_send_file(DRONE_CONTEXT * drone, char* dir, char* file, char* ext)
{
    struct stat  statbuf ;
    int size;
    FILE* fp;
    char name[128];
    char buffer[128];
    int ret;

   if(dir == NULL)
        sprintf( name , "%s.%s" ,  file, ext ) ;
    else
        sprintf( name , "%s/%s.%s" , dir , file, ext ) ;

    if( stat( name , &statbuf ) != 0 )
        return -1;

    size = statbuf.st_size;

    fp = fopen( name, "r" );
    if( fp == NULL )
    {
        LOG_PRINTF("Open file err!");
        return -1;
    }

    while( size > 0 )
    {
        ret = fread(buffer, 1, 116, fp);
        if(ret == 0)
            break;
        drone_spi_send_slice(drone, buffer, ret);
        size -= ret;
    }
    drone_spi_send_over(drone, 0xF5);

    if(fp != NULL)
        fclose(fp);

    return 0;
}

/////////////////////////////////////////////////////////
//     Function : drone_create_status_page
//  Description :
//        Input :
//       Output :
static void drone_create_status_page ( DRONE_CONTEXT * drone , int grp )
{
    GUI_STATIC_TEXT * text ;
    GUI_PAGE  * page ;

    int w , h , x , y , font_size ;
    int j, k, u, v, t=0, bais;

    font_size = grp == SYSVIN_VPSS_GRP_VI ?  48 : 23;
  
    w  = grp == SYSVIN_VPSS_GRP_VI ? SYSVIN_VPSS_VI_PORT_W : SYSVIN_VPSS_IR_PORT_W;
    h  = grp == SYSVIN_VPSS_GRP_VI ? 64 : 48;
    y  = grp == SYSVIN_VPSS_GRP_VI ? 16 : 8;
    x  = 0;
    
    page = (GUI_PAGE *)create_gui_object( GUI_PAGE_TYPE , x , y , w , h , 0 , 0 , 0 ) ;
       
    j = grp == SYSVIN_VPSS_GRP_VI ? 240 : 40;
    k = grp == SYSVIN_VPSS_GRP_VI ? 180 : 80;
    u = grp == SYSVIN_VPSS_GRP_VI ? 90  : 40;
    v = grp == SYSVIN_VPSS_GRP_VI ? 180 : 80;

    text = (GUI_STATIC_TEXT *)create_gui_object( GUI_TEXT_TYPE , j , 0 , v , h , 0 , OSD_RGB_GREEN , OSD_COLOR_TRANS ) ;
    text->obj.font = font_size ;
    text->obj.attribute = STATIC_TEXT_RECT ;
    strcpy( text->text , "A030" ) ;
    page->children[ page->count++ ] = (GUI_OBJECT*)text ;

    text = (GUI_STATIC_TEXT *)create_gui_object( GUI_TEXT_TYPE , j + 1*k , 0 , v , h , 0 , OSD_RGB_GREEN , OSD_COLOR_TRANS ) ;
    text->obj.font      = font_size ;
    text->obj.attribute = STATIC_TEXT_RECT ;
    strcpy( text->text , "E+020" ) ;
    page->children[ page->count++ ] = (GUI_OBJECT*)text ;

    text = (GUI_STATIC_TEXT *)create_gui_object( GUI_TEXT_TYPE , j + 2*k + 10, 0 , v/2 , h , 0 , OSD_RGB_GREEN , OSD_COLOR_TRANS ) ;
    text->obj.font      = font_size ;
    text->obj.attribute = STATIC_TEXT_RECT ;
    strcpy( text->text , "1X" ) ;
    page->children[ page->count++ ] = (GUI_OBJECT*)text ;

    text = (GUI_STATIC_TEXT *)create_gui_object( GUI_TEXT_TYPE , j + 3*k - u + 10, 0 , v/2 , h , 0 , OSD_RGB_GREEN , OSD_COLOR_TRANS ) ;
    text->obj.font = font_size ;
    text->obj.attribute = STATIC_TEXT_RECT ;
    strcpy( text->text , "K" ) ;
    page->children[ page->count++ ] = (GUI_OBJECT*)text ;

    text = (GUI_STATIC_TEXT *)create_gui_object( GUI_TEXT_TYPE , j + 4*k - u*2, 0 , v/2 , h , 0 , OSD_RGB_GREEN , OSD_COLOR_TRANS ) ;
    text->obj.font      = font_size ;
    text->obj.attribute = STATIC_TEXT_RECT ;
    strcpy( text->text , "NL" ) ;
    page->children[ page->count++ ] = (GUI_OBJECT*)text ;

    text = (GUI_STATIC_TEXT *)create_gui_object( GUI_TEXT_TYPE , j + 5*k - u*3 + 10, 0 , v , h , 0 , OSD_RGB_GREEN , OSD_COLOR_TRANS ) ;
    text->obj.font      = font_size ;
    text->obj.attribute = STATIC_TEXT_RECT ;
    strcpy( text->text , "X+000" ) ;
    page->children[ page->count++ ] = (GUI_OBJECT*)text ;

    bais = grp == SYSVIN_VPSS_GRP_VI ? 120 : 60 ;
    text = (GUI_STATIC_TEXT *)create_gui_object( GUI_TEXT_TYPE , j + 6*k - u*4 + bais, 0 , v , h , 0 , OSD_RGB_GREEN , OSD_COLOR_TRANS ) ;
    text->obj.font      = font_size ;
    text->obj.attribute = STATIC_TEXT_RECT ;
    strcpy( text->text , "Y+000" ) ;
    page->children[ page->count++ ] = (GUI_OBJECT*)text ;

    
    t = grp == SYSVIN_VPSS_GRP_VI ? 0 : 55;
    text = (GUI_STATIC_TEXT *)create_gui_object( GUI_TEXT_TYPE , j + 7*k - u*5 + bais * 2 + 10 - t , 0 , v+70 , h , 0 , OSD_RGB_GREEN , OSD_COLOR_TRANS ) ;
    text->obj.font      = font_size ;
    text->obj.attribute = STATIC_TEXT_RECT ;
    strcpy( text->text , "0000" ) ;
    page->children[ page->count++ ] = (GUI_OBJECT*)text ;

    text = (GUI_STATIC_TEXT *)create_gui_object( GUI_TEXT_TYPE , j + 8*k - u*6 + bais * 4 - 85 , 0 , v , h , 0 , OSD_RGB_GREEN , OSD_COLOR_TRANS ) ;
    text->obj.font      = font_size ;
    text->obj.attribute = STATIC_TEXT_RECT ;
    strcpy( text->text , "1000" ) ;
    page->children[ page->count++ ] = (GUI_OBJECT*)text ;

    if( grp == SYSVIN_VPSS_GRP_VI )
        drone->status_page_vi = page ;
    else if( grp == SYSVIN_VPSS_GRP_IR )
        drone->status_page_ir = page ;

    HAL_drv_ioctl( drone->vpss , grp , 0 , OP_ADD_GUI_PAGE , 0 , page ) ;
}

/////////////////////////////////////////////////////////
//     Function : drone_update_status_page
//  Description :
//        Input :
//       Output :
static void drone_update_status_page ( DRONE_CONTEXT * drone, int visible )
{
    int i;
    GUI_STATIC_TEXT * text ;
    GUI_PAGE  * page;
    char *type[] = {"K", "B", "H"};
    char *rec[] = {"V", "R", "P", "PR"};
    char *laser[] = {"LG", "LK"};
    char *bright[] = {"A", "M"};
    char *state[] = {"ZD", "SD", "SM", "GZ", "XS", "SY", "BH"};
    int grp = SYSVIN_VPSS_GRP_VI ;

    for(i = 0; i < 2; i++)
    {
        page = i == 0 ? drone->status_page_vi : drone->status_page_ir ;
        grp = i == 0 ? SYSVIN_VPSS_GRP_VI : SYSVIN_VPSS_GRP_IR ;

        HAL_drv_ioctl( drone->vpss , grp , 0 , OP_LOCK_GUI_OBJ , 0 , NULL ) ;

        page->obj.visible = visible;

        if(visible)
        {
#if 1
            text = ( GUI_STATIC_TEXT * )page->children[0] ;
            sprintf( text->text , "A%.3d" , drone->dome_azimuth/100) ;

            text = ( GUI_STATIC_TEXT * )page->children[1] ;
            sprintf( text->text , "E%+.3d" , drone->dome_pitch/100) ;
#endif
            text = ( GUI_STATIC_TEXT * )page->children[2] ;
            sprintf( text->text , "%dX" , drone->zoom + 1) ;

            text = ( GUI_STATIC_TEXT * )page->children[3] ;
            if(drone->video_type >=0 && drone->video_type <=2)
                sprintf( text->text , "%s" , type[drone->video_type]) ;

            text = ( GUI_STATIC_TEXT * )page->children[4] ;
            if(drone->dome_state >= 0 && drone->dome_state <= 6)
                sprintf( text->text , "%s" , state[drone->dome_state]) ;

            text = ( GUI_STATIC_TEXT * )page->children[5] ;
            sprintf( text->text , "X%+.3d" , drone->target_x) ;

            text = ( GUI_STATIC_TEXT * )page->children[6] ;
            sprintf( text->text , "Y%+.3d" , drone->target_y) ;
          

            if(drone->laser_show)
            {
                text = ( GUI_STATIC_TEXT * )page->children[7] ;
                sprintf( text->text , "L%.1f" ,(float)drone->laser_distance / 10.0) ;
            }
            else
            {
                text = ( GUI_STATIC_TEXT * )page->children[7] ;
                memset( text->text , 0 ,  sizeof(text->text)) ;
            }

            text = ( GUI_STATIC_TEXT * )page->children[8] ;
            drone->dispaly_number ++ ;
            drone->dispaly_number = drone->dispaly_number > 1000 ? 0 : drone->dispaly_number ;
            sprintf( text->text , "%d" , drone->dispaly_number) ;
        }

        HAL_drv_ioctl( drone->vpss , grp , 0 , OP_UNLOCK_GUI_OBJ , 0 , NULL ) ;
    }
}


/////////////////////////////////////////////////////////
//     Function : drone_callback
//  Description :
//        Input :
//       Output :
static int  drone_callback ( SYSENC_FRAME *fr , void * param )
{
#if 0
    static U64 last_time   = 0 ;
    static int frame_data_len = 0 ;

    struct timeval  tv ;
    U64  current_time ;
    int  val ;
    
    gettimeofday( &tv, NULL );
    current_time = tv.tv_sec * 1000 + tv.tv_usec / 1000 ;    

    val = current_time - last_time ;
    if(fr->frame.channel == 2 && fr->frame.type != VIDEO_JPEG_FRAME){    
        frame_data_len +=  fr->frame.length;
    }
    //printf("fr len =%d,sum=%d\n",fr->frame.length,frame_data_len,fr->frame.channel);
    if( val >= 1000 )
    {
        printf("Len=%d , time=%d , bps = %d\n", frame_data_len , val , frame_data_len * 8 ) ;
        last_time = current_time ;
        frame_data_len = 0 ;
    }
        
#endif     
    DRONE_CONTEXT * drone = (DRONE_CONTEXT *)param;
    NET_STREAM_FRAME_MSG  msg ;

    msg.head.sender   = 0 ;
    msg.head.receiver = APP_DRONE ;
    msg.head.command  = fr->frame.type == VIDEO_JPEG_FRAME ? DRONE_FRAME_JPEG : DRONE_FRAME_H264 ;
    msg.head.length   = sizeof(msg) - sizeof(MESSAG_HEADER) ;
    msg.fr            = fr ;

    if(drone->work_mode == DRONE_FIELS_TRANSFER_MODE)
        return 1;

    SYSENC_FR_ADDREF( fr ) ;

    if( send_message_nowait( drone->queue , sizeof(msg) , &msg ) != VX_OK )
    {
        LOG_PRINTF("SUB REF %s", __FUNCTION__);
        SYSENC_FR_SUBREF( fr ) ;
    }
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : drone_switch_zoom
//  Description : 
//        Input :
//       Output :
void drone_switch_zoom(DRONE_CONTEXT *drone, int zoom)
{
    int grp;
    LIVE_VIDEO_ZOOM attri_zoom;
    float x, y, width, height;

    if(zoom >= COE_NUM || zoom < 0)
        return ;
    drone->zoom = zoom;
    x = (drone->drone_mode == DRONE_MODE_VIDEO_VI ? 1920.0f:720.0f) / 2.0 - ((drone->drone_mode == DRONE_MODE_VIDEO_VI ? 1920.0f:640.0f)/drone->coe[zoom]) / 2.0;
    y = (drone->drone_mode == DRONE_MODE_VIDEO_VI ? 1080.0f:576.0f) / 2.0 - ((drone->drone_mode == DRONE_MODE_VIDEO_VI ? 1080.0f:512.0f)/drone->coe[zoom]) / 2.0;
    width = (drone->drone_mode == DRONE_MODE_VIDEO_VI ? 1920.0f:640.0f ) / drone->coe[zoom];
    height = (drone->drone_mode == DRONE_MODE_VIDEO_VI ? 1080.0f:512.0f ) / drone->coe[zoom];


    attri_zoom.x      = (int)x;
    attri_zoom.y      = (int)y;
    attri_zoom.width  = (int)width;
    attri_zoom.height = (int)height;
    printf("switch zoom = %d,(x,y,w,h)(%f,%f,%f,%f)\n",zoom, x, y, width, height);

    grp = drone->drone_mode == DRONE_MODE_VIDEO_VI ? SYSVIN_VPSS_GRP_VI : SYSVIN_VPSS_GRP_IR;
    HAL_drv_ioctl( drone->vpss , grp , 0 , OP_ZOOM_PARA , 0 , &attri_zoom ) ;

    drone_send_update_state(DRONE_UPDATE_STATE);
    core_do_command(APP_AI, AI_SWITCH_ZOOM, sizeof(float), &drone->coe[drone->zoom], NULL);
    core_do_command(APP_TRACKER, TRACKER_SWITCH_ZOOM, sizeof(float), &drone->coe[drone->zoom], NULL);
}

/////////////////////////////////////////////////////////
//     Function : drone_switch_mode
//  Description : 切换模式
//        Input : 
//       Output :
void drone_switch_mode(DRONE_CONTEXT *drone, int mode)
{
    int cmd;
    KERNEL_MESSAGE msg  ;
    MESSAG_HEADER *head = (MESSAG_HEADER *)&msg ;

    drone->drone_mode = mode;

    if(drone->drone_mode == 0)
    {
        drone->video_type = 0;
    }else
    {
        if(drone->ir_mode == 0)
            drone->video_type = 1;
        else
            drone->video_type = 2;
    }

    cmd = (drone->drone_mode == 0? SYSVIN_SWITCH_VIDEO_VI : SYSVIN_SWITCH_VIDEO_IR ) ;

    core_fill_message( head, APP_DRONE, SYS_VINPUT, cmd, sizeof(MESSAG_HEADER) );
    core_send_message( &msg ) ;

    drone->zoom = 0;
    drone_send_update_state(DRONE_ZOOM_SET);
    drone_send_update_state(DRONE_UPDATE_STATE);
    drone_send_msg_update_cross ( drone, 0);

}

/////////////////////////////////////////////////////////
//     Function : drone_main_loop
//  Description :
//        Input :
//       Output :
void * drone_fc_loop( void * priv )
{
    DRONE_CONTEXT * drone = (DRONE_CONTEXT *)priv;

    KERNEL_MESSAGE msg;


    LOG_PRINTF("drone fc thread running ...");

    drone->fc_timer = core_timer_message( APP_DRONE , DRONE_TIME_FC_ACK , 4 , TIMER_PERIOD ) ;

    while ( drone->running )
    {
        if ( receive_message( drone->fc_queue, sizeof(KERNEL_MESSAGE), (char *)&msg ) < 0 )
            continue ;

        switch( msg.command )
        {
            case DRONE_TIME_FC_ACK :
                if(drone->work_mode == 0){
                    drone_send_fc_msg(drone);
                }
                break;
            default :
                break ;
        }
    }

    return NULL ;
}

/////////////////////////////////////////////////////////
//     Function : drone_main_loop
//  Description :
//        Input :
//       Output :
void * drone_dome_loop( void * priv )
{
    DRONE_CONTEXT * drone = (DRONE_CONTEXT *)priv;
    KERNEL_MESSAGE msg;
//  char state_ack;
//  char buf[4] = {0};
   // core_timer_message( APP_DRONE , DRONE_DOME_ACK , 4 , TIMER_PERIOD ) ;

    LOG_PRINTF("drone dome thread running ...");

    while ( drone->running )
    {
        if ( receive_message( drone->dome_queue, sizeof(KERNEL_MESSAGE), (char *)&msg ) < 0 )
            continue ;

        switch( msg.command )
        {
            case DRONE_DOME_ACK :
                if(drone->old_cmd == DOME_HAND_TRACE)
                {
                    drone->dome_buffer[4]  = 0xff;
                    drone->dome_buffer[5]  = 0x7f;
                    drone->dome_buffer[8]  = 0xff;
                    drone->dome_buffer[9]  = 0x7f;
                }
                drone_dome_send_buffer(drone , drone->new_cmd, 1); 

                break;

            case DRONE_DOME_HAND_SPEED :
                drone->new_cmd = DOME_HAND_SPEED;
                drone_dome_send_buffer(drone , DOME_HAND_SPEED, 0 ) ;
                break;
            case DRONE_DOME_VERTICAL_DOWN :
                drone->new_cmd = DOME_VERTICAL_DOWN;
                drone_dome_send_buffer(drone , DOME_VERTICAL_DOWN, 0 ) ;
                break;

            case DRONE_DOME_HAND_TRACE:  // ok
            
                drone->dome_ack_count= 0;
                drone->dome_buffer[4]  = msg.data[0];
                drone->dome_buffer[5]  = msg.data[1];
                drone->dome_buffer[8]  = msg.data[2];
                drone->dome_buffer[9]  = msg.data[3];
                
                drone->new_cmd = DOME_HAND_TRACE;
                drone_dome_send_buffer(drone, DOME_HAND_TRACE, 0 ) ;
                drone->old_cmd = drone->new_cmd;
                break;
              
            case DRONE_DOME_SCAN_MODE :  ///? 
                drone->new_cmd = DOME_SCAN_MODE;
                drone_dome_send_buffer(drone, DOME_SCAN_MODE, 0 ) ;
                
                break;

            case DRONE_DOME_CENTRA_BACK:  // ok 
                drone->new_cmd = DOME_CENTRA_BACK;
                drone_dome_send_buffer(drone , DOME_CENTRA_BACK, 0 ) ;
                printf("dome send centra back!\n");
                break;

            case DRONE_DOME_DATA_LEAD:  // ok
                drone->dome_buffer[4]  = msg.data[0];
                drone->dome_buffer[5]  = msg.data[1];
                drone->dome_buffer[8]  = msg.data[2];
                drone->dome_buffer[9]  = msg.data[3];
                
                drone->new_cmd = DOME_DATA_LEAD;
                drone_dome_send_buffer(drone, DOME_DATA_LEAD, 0 ) ;
                break;
                
            case DRONE_DOME_GEOGROPY_LEAD:
                drone->new_cmd = DOME_GEOGROPY_LEAD;
                drone_dome_send_buffer(drone, DOME_GEOGROPY_LEAD, 0 ) ;
                break;
                
            case DRONE_DOME_PROTECT_MODE:  // ok
                drone->new_cmd = DOME_PROTECT_MODE;
                drone_dome_send_buffer(drone, DOME_PROTECT_MODE, 0 ) ;
                break;

            case DRONE_DOME_AUTO_TRACK_MODE:  // ok
                drone->new_cmd = DOME_AUTO_TRACK_MODE;
                
            #ifdef OFFSET_MODE    
                memcpy(drone->dome_buffer+4, drone->tracker_x,2);
                memcpy(drone->dome_buffer+8, drone->tracker_x,2);
            #else
                drone->dome_buffer[4]  = msg.data[0];
                drone->dome_buffer[5]  = msg.data[1];
                drone->dome_buffer[8]  = msg.data[2];
                drone->dome_buffer[9]  = msg.data[3];
            #endif
                drone_dome_send_buffer(drone, DOME_AUTO_TRACK_MODE, 0 ) ;
                break;
            case DRONE_DOME_STOP_TRACK_MODE:  // ok
                drone->new_cmd = DOME_STOP_TRACK_MODE;
                drone_dome_send_buffer(drone, DOME_STOP_TRACK_MODE, 0 ) ;
                break;

            case DRONE_DOME_TARGET_LOST:
                drone->new_cmd = DOME_TARGET_LOST;
                drone_dome_send_buffer(drone, DOME_TARGET_LOST, 0 ) ;
                break;
                
            case DRONE_DOME_MOTOR_TURN_ON:
                drone->new_cmd =DOME_MOTOR_TURN_ON;
                drone_dome_send_buffer(drone, DOME_MOTOR_TURN_ON , 0);
                break;

            case DRONE_DOME_MOTOR_TURN_OFF:
                  drone->new_cmd =DOME_MOTOR_TURN_OFF;
                drone_dome_send_buffer(drone, DOME_MOTOR_TURN_OFF , 0);
                break;
            case DRONE_DOME_SUPPRESS_GYRO_DIRFT:
                  drone->new_cmd =DOME_SUPPESS_DRIFT;
                drone_dome_send_buffer(drone, DOME_SUPPESS_DRIFT , 0);
                break;
            
            case DRONE_DOME_ELECTRIC_TRUN_ON:
                  drone->new_cmd =DOME_ELECTRIC_TRUN_ON;
                drone_dome_send_buffer(drone, DOME_ELECTRIC_TRUN_ON , 0);
                break;
            case DRONE_DOME_ELECTRIC_TRUN_OFF:
                  drone->new_cmd =DOME_ELECTRIC_TRUN_OFF;
                drone_dome_send_buffer(drone, DOME_ELECTRIC_TRUN_OFF , 0);
                break;
            
            case DRONE_DOME_TRACKER_TRUN_ON:
                  drone->new_cmd =DOME_TRACKER_TURN_ON;
                drone_dome_send_buffer(drone, DOME_TRACKER_TURN_ON , 0);
                break;
            case DRONE_DOME_TRACKER_TRUN_OFF:
                 drone->new_cmd =DOME_TRACKER_TRUN_OFF;
                drone_dome_send_buffer(drone, DOME_TRACKER_TRUN_OFF , 0);
                break;

            case DRONE_DOME_LASER_TURN_ON:  // ok
                drone->new_cmd = DOME_LASER_TRUN_ON;
                drone_dome_send_buffer(drone, DOME_LASER_TRUN_ON, 0 ) ;
                break;
            case DRONE_DOME_LASER_TURN_OFF:  // ok
                drone->new_cmd = DOME_LASER_TRUN_OFF;
                drone_dome_send_buffer(drone, DOME_LASER_TRUN_OFF, 0 ) ;
                break;
            default :
                break ;
        }
    }

    return NULL ;
}
int drone_mul_send_stream(DRONE_CONTEXT* gb, U8* frame_data, int frame_len)
{
    U8 spi_header[MAX_MUL_BUFFER];
    int idx = 0;
    int ret = 0;
    //ee16
    spi_header[idx++] = 0xee ;
    spi_header[idx++] = 0x16 ;
    spi_header[idx++] = 0x01 ;
    spi_header[idx++] = 0x03 ;
    spi_header[idx++] = 0x15 ;
    spi_header[idx++] = 0xf8 ;
    spi_header[idx++] = 0xf6 ;
    spi_header[idx++] = 0x1c ;
    //eb90
    spi_header[idx++] = 0xeb ; // A
    spi_header[idx++] = 0x90 ;
    spi_header[idx++] = 0x00 ; // B
    spi_header[idx++] = 0xff ;
    spi_header[idx++] = 0x00 ; // C
    spi_header[idx++] = 0xf2 ;
    spi_header[idx++] = gb->drone_mode == DRONE_MODE_VIDEO_VI ? 0x21 : 0x23 ; // D
    spi_header[idx++] = gb->spi_frame_idx ; // E
    spi_header[idx++] = 0x00 ; // F
    spi_header[idx++] = 0x00 ;
    spi_header[idx++] = 0x00 ;
    spi_header[idx++] = 0x00 ; // G
    spi_header[idx++] = 0x00 ;
   
    U8 * ptr ;
    int count, head_len, max, payload_len;
    count = frame_len ;
    head_len     = idx ;
    payload_len  = MAX_MUL_BUFFER - head_len ;

    idx      = 0   ; 
    max      = MAX_MUL_BUFFER ;
    ptr      = frame_data;
    while( count > 0 )
    {
        memcpy( gb->spi_buffer + idx , spi_header , head_len ) ;
        idx += head_len ;
         //copy stream data
        if( count >= payload_len )
        {
            memcpy( gb->spi_buffer + idx , ptr , payload_len  ) ;
            
            idx   += payload_len ;  //
            count -= payload_len ;  //
            ptr   += payload_len ;  //


            
            if( idx >= max )
            {
                 ret = sendto( gb->mul_socket , gb->spi_buffer , idx , 0 , (struct sockaddr *)&gb->server, sizeof(gb->server) );            
                 idx = 0 ;
            }
            
            continue ;

        }
            
        memset( gb->spi_buffer + idx  , 0 , payload_len ) ;
        memcpy( gb->spi_buffer + idx  , ptr , count  ) ;

        idx   += payload_len ;  
        ret = sendto( gb->mul_socket , gb->spi_buffer , idx , 0 , (struct sockaddr *)&gb->server, sizeof(gb->server) );  
        count -= payload_len ;  
    }
    return ret;
}

/////////////////////////////////////////////////////////
//     Function : drone_main_loop
//  Description :
//        Input :
//       Output :
void * drone_main_loop( void * priv )
{
    DRONE_CONTEXT * drone = (DRONE_CONTEXT *)priv;

    KERNEL_MESSAGE msg;
    NET_STREAM_FRAME_MSG  *stream_msg = (NET_STREAM_FRAME_MSG*)&msg;
    RECORDER_REC_FILE_MSG  *info      = (RECORDER_REC_FILE_MSG*)&msg ;
    MESSAG_HEADER               *head = (MESSAG_HEADER *)&msg ;
    RECORDER_SD_STATE_MSG *state_msg  = (RECORDER_SD_STATE_MSG*)&msg ;

    SYSENC_CALLBACK cb ;
    int id ;
    H264_JPG_FILE  * ptr ;


    cb.channel  = -1;
    cb.type     = FR_VIDEO | FR_PICTURE;
    cb.param    = priv ;
    cb.callback = drone_callback ;
    id = SYSENC_CALLBACK_REG( &cb ) ;

    LOG_PRINTF("Drone main thread running ...");

    //////////////////////////////////////////////////////////////////////////////////
    //锟斤拷锟斤拷STATUS锟斤拷
    drone_create_status_page( drone , SYSVIN_VPSS_GRP_VI );
    drone_create_status_page( drone , SYSVIN_VPSS_GRP_IR );
    drone_update_status_page( drone, drone->font_show ) ;

    drone->max_file_count   = INTF_load_sdcard_files( drone->h264_jpg_files , &drone->current_file_id ) ;
    drone->current_file_pos = drone->max_file_count ;
    while ( drone->running )
    {
        if ( receive_message( drone->queue, sizeof(KERNEL_MESSAGE), (char *)&msg ) < 0 )
            continue ;

        switch( msg.command )
        {
            case DRONE_WORK_MODE_SWITCH :
                drone->work_mode = msg.data[0];
                printf("---- DRONE_WORK_MODE_SWITCH  %d ------\n", drone->work_mode);
                drone_spi_send_ack(drone);
                break;
            case DRONE_BOARD_INFO:
                if(drone->work_mode == DRONE_LIVE_VIEW_MODE)
                    break;
                drone_spi_send_info(drone);
                break;
            case DRONE_FILES_LIST:
               if(drone->work_mode == DRONE_LIVE_VIEW_MODE)
                    break;
                drone_spi_send_list(drone);
                break;
            case DRONE_FILES_EXPORT:

                if(drone->work_mode == DRONE_LIVE_VIEW_MODE)
                    break;
                drone_spi_send_file(drone, STORAGE_DATA_DIR, (char*)&msg.data[0], (char*)&msg.data[9]);
                break;

            //H264/H265 Stream
            case DRONE_FRAME_H264 :
                if(drone->work_mode == DRONE_LIVE_VIEW_MODE)
                {
                    if( drone->drone_mode == DRONE_MODE_VIDEO_VI || drone->drone_mode == DRONE_MODE_VIDEO_IR )
                    {
                            
                            drone_spi_send_stream( drone , stream_msg->fr );
                           
                    }
                }
                //Release it
                SYSENC_FR_SUBREF( stream_msg->fr ) ;
				drone_send_update_state(DRONE_UPDATE_STATE);
                break ;

            case DRONE_FRAME_JPEG :  // ?
                //Release it
                SYSENC_FR_SUBREF( stream_msg->fr ) ;
                break ;

            case DRONE_UPDATE_STATE :
                drone_update_status_page( drone, drone->font_show) ;
                break;

            case DRONE_TIME_SERIES_PIC :
                LOG_PRINTF("-----DRONE_TIME_SERIES_PIC------");
                INTF_snap_picture(drone->current_file_id++, drone->drone_mode == 0 ? VENC_VI_CHANNEL:  VENC_IR_CHANNEL, 1);
                drone->pic_mode = 1;
                drone_send_update_state(DRONE_UPDATE_STATE);
                break;

            case RECORDER_FILE_FINISHED :

                printf("--RECORDER_FILE_FINISHED!(%s) drone->max_file_count = %d --\n", info->name, drone->max_file_count);
                ptr = drone->h264_jpg_files+drone->current_file_pos;
                INTF_parse_file_info(ptr, NULL, info->name);
                drone->current_file_pos++;

                if(ptr->video == 1)
                    drone->rec_mode = 0;
                else
                    drone->pic_mode = 0;

                drone_send_update_state(DRONE_UPDATE_STATE);
                break;

            case SYSVIN_UPDATE_REC_STATE :
                core_fill_message( head, APP_DRONE, APP_DRONE, DRONE_UPDATE_STATE, sizeof(MESSAG_HEADER) );
                core_send_message( &msg ) ;
                break;
            case SYSVIN_UPDATE_ALARM_STATE :
                break;
            case SYSVIN_UPDATE_SNAP_STATE :
                break;
            case SYSVIN_UPDATE_SD_STATE :
                drone->free_storage = state_msg->capacity - state_msg->used;
                drone->capacity     = (U64)state_msg->capacity;
                drone->used         = (U64)state_msg->used;
                drone->capacity     = drone->capacity << 10; // Unit: byte
                drone->used         = drone->capacity << 10; // Unit: byte
                break ;

            default :
                break ;
        }

    }

    SYSENC_CALLBACK_UNREG( id ) ;
    return NULL ;
}

/////////////////////////////////////////////////////////
//     Function : drone_setup
//  Description :
//        Input :
//       Output :
static int drone_setup( DRONE_CONTEXT * drone )
{
    HAL_DRIVER  *vpssdrv ;
    HAL_DRIVER  *vidrv ;
    int  ret ;
    int i;
    double x, y;
    double len;

    drone->shmcfg =  (SHM_CONFIG *)GET_SYS_SHMCFG();

    drone->drone_mode     = shmcfg_get_integer( drone->shmcfg, "SYSTEM", "VIFormat",  0  );
    drone->ir_mode        = shmcfg_get_integer( drone->shmcfg, "DRONE", "IRMode",     0  );
    drone->len_ir         = shmcfg_get_integer( drone->shmcfg, "DRONE", "IRLen",      IR_FOCAL_LEN);  // 25mm  X*100
    drone->len_vi         = shmcfg_get_integer( drone->shmcfg, "DRONE", "VILen",      VI_FOCAL_LEN);  // 12mm   X*100

    drone->ir_detector_x     = shmcfg_get_integer( drone->shmcfg, "DRONE", "IrDectorX",     IR_WIDTH );
    drone->ir_detector_y     = shmcfg_get_integer( drone->shmcfg, "DRONE", "IrDectorY",     IR_HEIGHT );
    drone->ir_detector_pixel = shmcfg_get_integer( drone->shmcfg, "DRONE", "IrDectorPixel", IR_PIEXL ); //  *1000

    drone->vi_sensor_x       = shmcfg_get_integer( drone->shmcfg, "DRONE", "SensorX",     VI_WIDTH );
    drone->vi_sensor_y       = shmcfg_get_integer( drone->shmcfg, "DRONE", "SensorY",     VI_HEIGHT );
    drone->vi_sensor_pixel   = shmcfg_get_integer( drone->shmcfg, "DRONE", "SensorPixel", VI_PIEXL ); //  *1000
    drone->load_id        = shmcfg_get_integer( drone->shmcfg, "DRONE", "LoadID", 0 );
    drone->ai_state = 0;
    
    drone->vi_cross_x = shmcfg_get_integer( drone->shmcfg, "DRONE", "ViCrossX", VI_CROSS_CENTER_POINT_X );
    drone->vi_cross_y = shmcfg_get_integer( drone->shmcfg, "DRONE", "ViCrossY", VI_CROSS_CENTER_POINT_Y );
    drone->ir_cross_x = shmcfg_get_integer( drone->shmcfg, "DRONE", "IrCrossX", IR_CROSS_CENTER_POINT_X );
    drone->ir_cross_y = shmcfg_get_integer( drone->shmcfg, "DRONE", "IrCrossY", IR_CROSS_CENTER_POINT_Y );
    drone_send_msg_update_cross ( drone, 0);

    LOG_PRINTF("  drone->len_ir = %d, drone->len_vi = %d   \n\n\n", drone->len_ir, drone->len_vi );
    drone->fc_protocl_len = shmcfg_get_integer( drone->shmcfg, "DRONE", "FcLenght", FC_PACKET_LENGHT);
    drone->dome_state     = 3;
    drone->bitrate_mode   = 0 ;
    drone->track_state    = 0 ;
    drone->zoom           = 0 ;
    drone->dome_count     = 0 ;
    drone->running        = 1 ;
    drone->font_show      = 1 ;
    drone->bright_value   = 128 ;
    drone->contrast_value = 128 ;
    drone->fw_angle       = 30 ;
    drone->gd_angle       = 20 ;
    drone->target_x       = 0 ;
    drone->target_y       = 0 ;
    drone->dome_old_x     = -1;
    drone->dome_old_y     = -1;
    drone->work_mode      = DRONE_LIVE_VIEW_MODE;
    drone->font_show      = 1;
    drone->latest_cmd     = 0;
    drone->vi_frame_rate  = 60;
    drone->ir_frame_rate  = 25;
    drone->dome_last_cmd  = 0;
    drone->pic_timer      = 0;
    drone->cm_set_unit_count = 0;
    drone->dispaly_number = 1 ;
    drone->track_old_x    = 10000;
    drone->track_old_y    = 10000;

    drone->laser_ptr = 0;
    drone->laser_state = 0 ;
    memset(drone->laser_result, 0, sizeof(drone->laser_result));

    drone->coe[0]   = 1;
    drone->coe[1]   = 1.11;
    drone->coe[2]   = 1.22;
    drone->coe[3]   = 1.33;
    drone->coe[4]   = 1.44;
    drone->coe[5]   = 1.55;
    drone->coe[6]   = 1.66;
    drone->coe[7]   = 1.77;
    drone->coe[8]   = 1.88;
    drone->coe[9]   = 1.99;
 
    drone->bitrate[0] = (float)2.5*(float)1024*(float)1024;
    drone->bitrate[1] = (float)3*(float)1024*(float)1024;
    drone->bitrate[2] = (float)3.5*(float)1024*(float)1024;
    drone->bitrate[3] = (float)5*(float)1024*(float)1024;

    drone->vi_bitrate     = shmcfg_get_integer( drone->shmcfg, "VIDEO_VISABLE", "Bitrate", (int)drone->bitrate[0] ) ;
    drone->ir_bitrate     = shmcfg_get_integer( drone->shmcfg, "VIDEO_IRIMAGE", "Bitrate", (int)drone->bitrate[0] ) ;

    for(i = 0; i < 4; i++)
        LOG_PRINTF("------- bitrate[%d] = %f ---------", i, drone->bitrate[i]);

    if(drone->drone_mode == 0)
        drone->video_type = 0;
    else
    {
        if(drone->ir_mode == 0)
        {
            drone->video_type = 1;

        }else
        {
            drone->video_type = 2;
        }
    }

    for(i = 0; i < COE_NUM; i++)
    {
        drone->sensor_actual_x[i] = (float)drone->vi_sensor_x/drone->coe[i];
        drone->sensor_actual_y[i] = (float)drone->vi_sensor_y/drone->coe[i];

        x = ((double)drone->sensor_actual_x[i]/(double)1000)*((double)drone->vi_sensor_pixel/(double)1000)/(double)2;
        y = ((double)drone->sensor_actual_y[i]/(double)1000)*(((double)drone->vi_sensor_pixel)/(double)1000)/(double)2;
        len = (double)drone->len_vi/(double)100;

        drone->vi_fov_x[i] = (double)57.3*atan(x/len);
        drone->vi_fov_y[i] = (double)57.3*atan(y/len);

        drone->detector_actual_x[i] = (float)drone->ir_detector_x/drone->coe[i];
        drone->detector_actual_y[i] = (float)drone->ir_detector_y/drone->coe[i];

        x = ((double)drone->detector_actual_x[i]/(double)1000)*((double)drone->ir_detector_pixel/(double)1000)/(double)2;
        y = ((double)drone->detector_actual_y[i]/(double)1000)*(((double)drone->ir_detector_pixel)/(double)1000)/(double)2;
        len = (double)drone->len_ir/(double)100;

        drone->ir_fov_x[i] = (double)57.3*atan(x/len);
        drone->ir_fov_y[i] = (double)57.3*atan(y/len);
    }

    vpssdrv = find_driver( "VPSS" );
    if( vpssdrv == NULL )
        return 0;

    drone->vpss = vpssdrv;

    vidrv   = find_driver( "VideoInput" ) ;
    if( vidrv == NULL )
        return 0;

    drone->vidrv = vidrv;

    drone->h264_jpg_files = ( H264_JPG_FILE * )malloc( MAX_H264_JPG_FILES * sizeof(H264_JPG_FILE) ) ;
    if( drone->h264_jpg_files == NULL )
    {
        LOG_PRINTF("drone : Out of memory .");
        return 0 ;
    }

    /////SPI
    drone->spi_fd = open("/dev/spidev1.0",O_RDWR);
    if( drone->spi_fd < 0)
        LOG_PRINTF("Can't Open SPI device !");
    ret = 8 ;
    ioctl( drone->spi_fd , SPI_IOC_WR_BITS_PER_WORD,&ret );// change spi to 8bits mode

    LOG_PRINTF("drone setup ok!\r\n");
    return 1;
}

int drone_creat_mul_socket(DRONE_CONTEXT * drone)
{
    char* p_char;
    bzero(&drone->server,sizeof(drone->server));
    p_char = shmcfg_get_str_def( drone->shmcfg, (char*)"DRONE", "MulAddr",   "224.0.1.1");
    drone->multicast_port = shmcfg_get_integer( drone->shmcfg, (char*)"DRONE", "MulPort",   6000);
    drone->server.sin_family = AF_INET;
    drone->server.sin_addr.s_addr = inet_addr( p_char );
    drone->server.sin_port = htons( drone->multicast_port );
    drone->mul_socket = socket( AF_INET , SOCK_DGRAM, 0 );
    return 1;
}
/////////////////////////////////////////////////////////
//     Function : drone_start
//  Description :
//        Input :
//       Output :
int drone_start ( void * priv )
{
    DRONE_CONTEXT * drone = ( DRONE_CONTEXT * )priv ;
    memset( drone , 0 , sizeof(DRONE_CONTEXT) );

    if( !drone_setup( drone ) )
        return 0 ;

    pthread_mutex_init( &drone->comm_mutex, NULL );
   
    SERIAL_CALLBACK_REG( 0, drone_fc_serial_receive_packet, drone);
    SERIAL_CALLBACK_REG( 4, drone_ir_serial_receive_packet, drone);
    SERIAL_CALLBACK_REG( 1, drone_dome_serial_receive_packet, drone); 
    
    drone->queue  = msgQCreate( 64 , sizeof(KERNEL_MESSAGE), 0 ) ;
    if( drone->queue == NULL )
        return 0 ;

    drone->fc_queue  = msgQCreate( 32 , sizeof(KERNEL_MESSAGE), 0 ) ;
    if( drone->fc_queue == NULL )
        return 0 ;

    drone->dome_queue  = msgQCreate( 64 , sizeof(KERNEL_MESSAGE), 0 ) ;
    if( drone->dome_queue == NULL )
        return 0 ;

    drone->dome_ack_count = 0;
    drone->send_mode = shmcfg_get_integer( drone->shmcfg, "NETWORK", "SendMode", 0); 
   
    core_create_thread( "Drone loop" , drone_main_loop ,  priv ) ;
    core_create_thread( "Drone fc loop" , drone_fc_loop ,  priv ) ;
    core_create_thread( "Drone dome loop" , drone_dome_loop ,  priv ) ;
    
    LOG_PRINTF("Board Service start ... ");

    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : drone_stop
//  Description :
//        Input :
//       Output :
int drone_stop ( void * priv )
{
    DRONE_CONTEXT * drone = ( DRONE_CONTEXT * )priv ;

    pthread_mutex_destroy( &drone->comm_mutex ) ;
    SERIAL_CALLBACK_UNREG( 0, drone_fc_serial_receive_packet, drone);
    SERIAL_CALLBACK_UNREG( 4, drone_ir_serial_receive_packet, drone);
    SERIAL_CALLBACK_UNREG( 1, drone_dome_serial_receive_packet, drone);

    drone->running = 0;

    LOG_PRINTF("Drone Service exit ... ");
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : serial_message
//  Description :
//        Input :
//       Output :
int drone_message( void *priv , int sender, void *msg )
{
    DRONE_CONTEXT *drone = (DRONE_CONTEXT *)priv ;
    MESSAG_HEADER *header = (MESSAG_HEADER *)msg ;

    if( (header->command >= DRONE_TIME_FC_ACK) && (header->command < DRONE_DOME_ACK))
        send_message_nowait( drone->fc_queue , header->length + sizeof(MESSAG_HEADER) , msg ) ;
    else if( (header->command >= DRONE_DOME_ACK) && (header->command <= DRONE_DOME_MAX_CMD) )
        send_message_nowait( drone->dome_queue , header->length + sizeof(MESSAG_HEADER) , msg ) ;
    else
        send_message_nowait( drone->queue , header->length + sizeof(MESSAG_HEADER) , msg ) ;

    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : drone_track_methd_handle
//  Description :
//        Input :
//       Output :
int drone_track_methd_handle(DRONE_CONTEXT * drone, int x, int y, int status)
{
    short dotx, doty;
    int offset_x,offset_y;

    if(drone->drone_mode == 0)
    {
        offset_x = drone->vi_cross_x - VI_CROSS_CENTER_POINT_X;
        offset_y = drone->vi_cross_y - VI_CROSS_CENTER_POINT_Y;
        dotx = (S16)x - VI_CODEC_WIDTH_HALF - offset_x;
        doty = (S16)y - VI_CODEC_HEIGHT_HALF - offset_y;
    }else
    {
        offset_x = drone->ir_cross_x - IR_CROSS_CENTER_POINT_X;
        offset_y = drone->ir_cross_y - IR_CROSS_CENTER_POINT_Y;
        dotx = (S16)x - IR_CODEC_WIDTH_HALF - offset_x;
        doty = (S16)y - IR_CODEC_HEIGHT_HALF - offset_y;
    }

    doty = -doty;

    drone->target_x = dotx;
    drone->target_y = doty;

    printf("dot_x = %d, dot_y = %d\n", dotx, doty);

	if(abs((int)dotx) < 32)
		dotx /= 4;
	else if(abs((int)dotx) < 64)
		dotx /= 2;

	if(abs((int)doty) < 32)
		doty /= 4;
	else if(abs((int)doty) < 64)
		doty /= 2;


//  LOG_PRINTF("   drone->target_x = %d, drone->target_y = %d    ", drone->target_x, drone->target_y);
    drone_dome_hand_track_handle(drone, status == 1? DRONE_DOME_AUTO_TRACK_MODE: DRONE_DOME_TARGET_LOST, dotx, doty);
    return 0;
}

void ctl_osd_x_y_range(VINPUT_USER_GUI_MSG *osd )
{
    if(osd->channel == SYSVIN_VPSS_IR_CHN ){
         if( osd->x > (IR_CROSS_CENTER_POINT_X * 2))
             osd->x = IR_CROSS_CENTER_POINT_X * 2;

         if( osd->x < 0 )
             osd->x = 0;
    
         if( osd->y > (IR_CROSS_CENTER_POINT_Y * 2))
             osd->y = IR_CROSS_CENTER_POINT_Y * 2;
         
         if( osd->y < 0)
             osd->y = 0;
    }
    else
    {
        if( osd->x > (VI_CROSS_CENTER_POINT_X * 2))
            osd->x = VI_CROSS_CENTER_POINT_X * 2;
        
        if( osd->x < 0 )
            osd->x = 0;
           
        if( osd->y > (VI_CROSS_CENTER_POINT_Y * 2))
            osd->y = VI_CROSS_CENTER_POINT_Y * 2;
                
        if( osd->y < 0)
            osd->y = 0;

    }
    

}

void send_to_sysvin_msg( VINPUT_USER_GUI_MSG *osd , int cross_type)
{
    ctl_osd_x_y_range(osd);
    core_fill_message(&osd->head , APP_DRONE, SYS_VINPUT, cross_type , sizeof(VINPUT_USER_GUI_MSG));
    core_send_message((KERNEL_MESSAGE*)osd ) ;
}


void  drone_send_msg_update_cross ( DRONE_CONTEXT * p, int mode)
{
    int left,right,up,down;
    VINPUT_USER_GUI_MSG* osd;
    KERNEL_MESSAGE msg;
    osd = ( VINPUT_USER_GUI_MSG *)&msg ;
    osd->bcolor = 0   ;
    osd->alpha  = 255 ;
    osd->enable = 1 ;
    osd->channel = p->drone_mode == 0 ? SYSVIN_VPSS_VI_CHN : SYSVIN_VPSS_IR_CHN ; 

    left = mode == DRONE_CROSS_LEFT ? -1 : 0; 
    right = mode == DRONE_CROSS_RIGHT ? 1 : 0;
    up = mode == DRONE_CROSS_UP ? -1 : 0;
    down = mode == DRONE_CROSS_DOWN ? 1 : 0;
    
    if(osd->channel == SYSVIN_VPSS_VI_CHN)
    {
        osd->width     = CROSS_OSD_SIZE_HD ;
        osd->height    = CROSS_OSD_SIZE_HD ;  
        osd->x = p->vi_cross_x + left + right; 
        osd->y = p->vi_cross_y + up + down;
        send_to_sysvin_msg(osd, SYSVIN_SET_CROSS_OSD);
        p->vi_cross_x  =  osd->x; 
        p->vi_cross_y  =  osd->y; 

    }
    else
    {
        osd->width  = CROSS_OSD_SIZE_SD ;
        osd->height = CROSS_OSD_SIZE_SD ;  
        osd->x      = p->ir_cross_x + left + right; 
        osd->y      = p->ir_cross_y + up + down;
        send_to_sysvin_msg(osd, SYSVIN_SET_CROSS_OSD);
        p->ir_cross_x = osd->x;
        p->ir_cross_y = osd->y;

    }
}

/////////////////////////////////////////////////////////
//     Function : drone_command
//  Description :
//        Input :
//       Output :
int drone_command ( void *priv ,  int op , int len , void * ibuf , void * obuf )
{
    int ret = 0 ;
    DRONE_CONTEXT *drone = (DRONE_CONTEXT *)priv ;
    int  grp;
    char name[64];
    H264_JPG_FILE  info, *ptr;
    int i;
    char *p;
    int zoom = 0;
    int mode;

    char cm1[] = {0x01, 0x6d, 0x11, 0x22,0x11};
    char cm2[] = {0x02, 0x6d, 0x0d, 0x0a,0x11};
    char cm3[] = {0x03, 0x6d, 0x0d, 0x0a,0x11};
    char cm4[] = {0x04, 0x6d, 0x0d, 0x0a,0x11};

    switch( op )
    {
    case DRONE_MODE_SWITCH_TO_VI:
        drone_switch_mode(drone, 0);
        break;

    case DRONE_MODE_SWITCH_TO_IR:
        drone_switch_mode(drone, 1);
        break;

    case DRONE_MODE_SWITCH:
        drone_switch_mode(drone, !drone->drone_mode);
        break;

    case DRONE_IR_WHITE_HEAT:
        if(drone->drone_mode == 0)
            break;

        drone->ir_mode = 0;
        drone->video_type = 1;

        LGC6122_black_white(0x00);
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_IR_BLACK_HEAT:
        if(drone->drone_mode == 0)
            break;

        drone->ir_mode = 1;
        drone->video_type = 2;

        LGC6122_black_white(0x01);
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_MODE_GET:
        SERIAL_SEND_DATA(1, cm1, sizeof(cm1));
        SERIAL_SEND_DATA(2, cm2, sizeof(cm2));
        SERIAL_SEND_DATA(3, cm3, sizeof(cm3));
        SERIAL_SEND_DATA(4, cm4, sizeof(cm4));
        printf("send 4\n");
        break;

    case DRONE_GET_FLIGHT_DATA :
        ret = (int)drone->fc_buffer + 14 ;
        break ;

    case DRONE_REPORT_TRACK_INFO :
        {
            if(drone->track_state != 1)
                break;
            TRACKER_OBJ_REPORT * report = ( TRACKER_OBJ_REPORT * )ibuf ;
            drone->tracker_x = report->x;
            drone->tracker_y = report->y;
            //  LOG_PRINTF("DRONE : OBJ => %d,%d %dx%d , %d" , report->x , report->y , report->w , report->h , report->state );
        #ifdef OFFSET_MODE
            drone_send_update_state_with_par( drone ,  report->state == 1? DRONE_DOME_AUTO_TRACK_MODE: DRONE_DOME_TARGET_LOST );
        #else
            drone_track_methd_handle(drone, report->x , report->y, report->state);
        #endif 
        }
        break ;

    case DRONE_ADJUST_AUTO:
        grp = drone->drone_mode == DRONE_MODE_VIDEO_VI ? SYSVIN_VPSS_GRP_VI : SYSVIN_VPSS_GRP_IR;
        drone->bright_value   = 128;
        drone->contrast_value = 128;
        HAL_drv_ioctl( drone->vpss , grp , 0 , OP_CSC_BRIGHT , drone->bright_value, NULL ) ;
        HAL_drv_ioctl( drone->vpss , grp , 0 , OP_CSC_CONTRAST , drone->contrast_value, NULL ) ;

        drone->bc_state = 0;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_BRIGHT_ADJUST:
        grp = drone->drone_mode == DRONE_MODE_VIDEO_VI ? SYSVIN_VPSS_GRP_VI : SYSVIN_VPSS_GRP_IR;
        drone->bright_value += *((int *)ibuf);
        HAL_drv_ioctl( drone->vpss , grp , 0 , OP_CSC_BRIGHT , drone->bright_value, NULL ) ;

        drone->bc_state = 1;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_CONTRAST_ADJUST:
        grp = drone->drone_mode == DRONE_MODE_VIDEO_VI ? SYSVIN_VPSS_GRP_VI : SYSVIN_VPSS_GRP_IR;
        drone->contrast_value += *((int *)ibuf);
        HAL_drv_ioctl( drone->vpss , grp , 0 , OP_CSC_CONTRAST , drone->contrast_value, NULL ) ;

        drone->bc_state = 1;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_FILE_DELETE:
        p = (char*)ibuf;
        sprintf(name, "%s.%s", (char*)&p[0], (char*)&p[9]);
        LOG_PRINTF("--- name = %s ---", name);

        INTF_parse_file_info( (H264_JPG_FILE*)&info , STORAGE_DATA_DIR , name );

        for(i = 0; i < drone->current_file_pos ; i++)
        {
            ptr = drone->h264_jpg_files + i;
            if(ptr->fid == info.fid)
            {
                INTF_delete_file(info.video, info.fid, info.channel);
                ptr->fid = INVALID_FID ;

                drone->max_file_count-- ;
                break ;
            }
        }

        break;
    case DRONE_FILES_DELETE:

        for(i = 0; i < drone->current_file_pos ; i++)
        {
            ptr = drone->h264_jpg_files + i;
            if(ptr->fid == INVALID_FID )
                continue ;

            INTF_delete_file(ptr->video, ptr->fid, ptr->channel);
            ptr->fid = INVALID_FID ;
        }
        drone->max_file_count   = 0 ;
        drone->current_file_pos = 0 ;
        break;

    case DRONE_TAKE_PHOTO :
        drone->pic_mode = 1;
        INTF_snap_picture(drone->current_file_id++, drone->drone_mode == 0 ? VENC_VI_CHANNEL:  VENC_IR_CHANNEL, 1);
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_ZOOM_SET:
        zoom = *((int *)ibuf);
        printf("zoom = %d \n",zoom);
        drone_switch_zoom(drone, zoom);
        break; 
        
    case DRONE_FONT_SHOW:
        drone->font_show = !drone->font_show;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_DOME_INFO:
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_RECORDER_START :
        LOG_PRINTF("-------DRONE_RECORDER_START-------");
        INTF_start_record(drone->current_file_id++, drone->drone_mode == 0 ? VENC_VI_CHANNEL:  VENC_IR_CHANNEL );
        drone->rec_mode = 1;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_RECORDER_STOP :
        LOG_PRINTF("-------DRONE_RECORDER_STOP-------");
        INTF_stop_record(drone->drone_mode == 0 ? VENC_VI_CHANNEL:  VENC_IR_CHANNEL);
        break;

    case DRONE_START_SERIES_PIC :
        drone->pic_mode = 1;

        if( drone->pic_timer > 0 )
        {
            core_timer_pause(drone->pic_timer, 0);
        }else
        {
            drone->pic_timer = core_timer_message( APP_DRONE , DRONE_TIME_SERIES_PIC , 300 , TIMER_PERIOD ) ;
        }
        LOG_PRINTF("DRONE_START_SERIES_PIC (drone->pic_timer = %d) ", drone->pic_timer);
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_STOP_SERIES_PIC :
        if( drone->pic_timer == 0 )
            break;

        LOG_PRINTF("DRONE_STOP_SERIES_PIC(drone->pic_timer = %d)", drone->pic_timer );
        core_timer_pause(drone->pic_timer, 1);
        break;
        
    case DRONE_STOP_TRACK:
        drone->dome_old_x = -1;
        drone->dome_old_y = -1;
        break;
        
    case DRONE_LASER_TURN_ON:
        drone->laser_show = 1;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;

    case DRONE_LASER_TURN_OFF:
        drone->laser_show = 0;
        drone_send_update_state(DRONE_UPDATE_STATE);
        break;
        
    case DRONE_AI_SET_CMD:
        mode = OBJ_PERSON_VI;
        core_do_command( APP_AI, AI_SET_OBJECT_CLASS, sizeof(int),&mode , NULL ) ;
        printf("set ai mode\n");
        break;
        
    case DRONE_SET_AI_METHOD:
        mode = *((int*)ibuf);
        core_do_command( APP_AI, AI_SET_METHOD, sizeof(int),&mode , NULL ) ;
        printf("set ai mode\n");
        break;

    case DRONE_DOME_TEXT_CMD:
        mode  = *(int*)ibuf;
        drone_dome_send_buffer( drone, mode, 0 ) ;
        printf( " DOME mode=%d\n", mode );
        break;
    case DRONE_CROSS_LEFT:
    case DRONE_CROSS_RIGHT:
    case DRONE_CROSS_UP:
    case DRONE_CROSS_DOWN:
        drone_send_msg_update_cross( drone, op);
        break;
    
    case DRONE_CROSS_SAVE:
        shmcfg_set_integer( drone->shmcfg , "DRONE", "ViCrossX",  drone->vi_cross_x );
        shmcfg_set_integer( drone->shmcfg , "DRONE", "ViCrossY",  drone->vi_cross_y );
        shmcfg_set_integer( drone->shmcfg , "DRONE", "IrCrossX",  drone->ir_cross_x );
        shmcfg_set_integer( drone->shmcfg , "DRONE", "IrCrossY",  drone->ir_cross_y );
        shmcfg_flush( drone->shmcfg ); 
        break;
        
    default:
        break;
    }

    return ret ;
}

// Private struct of the module
static CORE_SERVICE  drone_service = {
    "Drone Ver 0.0.01"  , APP_DRONE    ,
     drone_start        , drone_stop   ,
     drone_message      , drone_command ,
     (void*)&g_drone_context
} ;

/////////////////////////////////////////////////////////
//     Function : drone_init
//  Description :
//        Input :
//       Output :
int  drone_init( )
{
    register_service( &drone_service ) ;
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : drone_exit
//  Description :
//        Input :
//       Output :
int  drone_exit( )
{
    unregister_service( APP_DRONE ) ;
    return 1 ;
}

