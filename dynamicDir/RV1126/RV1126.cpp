/**************************************************
 *
 *Description:
 *      RV1126的HAL驱动 -- 模块部分
 *
 *
**************************************************/


#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <math.h>


#include "sys-core.h"
//#include "sys-service.h"
#include "sys-hal.h"

//平台使用的内存池大小
#define   HI_POOL_LENGTH    (1024*1024*16)    //16M Bytes

void RV1126_register_video_input  ( void ) ;
void RV1126_register_video_output ( void ) ;
void RV1126_register_audio_input  ( void ) ;
void RV1126_register_vpss  ( void ) ;
void RV1126_register_acodec( void ) ;
void RV1126_register_vcodec( void ) ;
void RV1126_register_ive   ( void ) ;

void RV1126_unregister_video_input  ( void ) ;
void RV1126_unregister_video_output ( void ) ;
void RV1126_unregister_audio_input  ( void ) ;
void RV1126_unregister_vpss  ( void ) ;
void RV1126_unregister_acodec( void ) ;
void RV1126_unregister_vcodec( void ) ;
void RV1126_unregister_ive   ( void ) ;

MEM_POOL_ID  hi_pool_id  = NULL ;

int init_sdk()
{   
    hi_pool_id = core_pool_create( HI_POOL_LENGTH , NULL ) ;
    if( hi_pool_id == NULL )
        return 0 ;
        
    printf( "init sdk ok!\n");
    return 1;
}

//////////////////////////////////////////////////////////////////////////////
//模块标准接口
//
//模块初始化函数
extern "C" __attribute__((visibility("default"))) int  RV1126_init( ) 
{        
    printf("RV1126 Platform init ..." );
    
    if( !init_sdk() )
        return 0 ;

    RV1126_register_video_input(); 
    RV1126_register_video_output(); 
    RV1126_register_audio_input();    
    
    RV1126_register_vpss();   
    //RV1126_register_acodec();    
    RV1126_register_vcodec();   
    RV1126_register_ive();
    
    return 1 ;
}
//模块退出函数
extern "C"  __attribute__((visibility("default"))) int  RV1126_exit( )
{
    
    RV1126_unregister_video_input(); 
    RV1126_unregister_video_output(); 
    RV1126_unregister_audio_input();   
    
    RV1126_unregister_vpss(); 
    //RV1126_unregister_acodec();    
    RV1126_unregister_vcodec();    
    RV1126_unregister_ive();

    core_pool_destroy( hi_pool_id ) ;

    printf("RV1126 Platform Exit ...\n" );    
    
    return 1 ;
}

