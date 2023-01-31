/**************************************************
 *
 *Description:
 *      RV1126的HAL驱动 -- Live部分
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
#include <fcntl.h>
#include <sys/mman.h>


//rkmedia 
#include "buffer.h"
#include "filter.h"
#include "image.h"
#include "key_string.h"
#include "flow.h"
#include "control.h"
#include "stream.h"

#include "hal-private.h"
#include "hal-filter.h"

//#include "sys-core.h"
//#include "sys-service.h"
#include "sys-hal.h"

#define  DRM_DEVICE_PATH  "/dev/dri/card0"
#define  MAX_VO_PORT      1


//视频输出端口定义
typedef struct
{
    int    enabled ;   
    int    width   ;
    int    height  ;
                 
    std::shared_ptr<easymedia::Flow>  image_flow     ; 
    std::shared_ptr<easymedia::Flow>  display_flow   ;   
    std::string                       display_fmt    ;        
}HI_VIDEO_OUTPUT ;

typedef struct
{
    HAL_DRIVER   hal ;
        
    HI_VIDEO_OUTPUT  port[MAX_VO_PORT] ;
}HI_LIVE_VIDEO_OUTPUT ;


//////////////////////////////////////////////////////////////////////////////
//Live video Output 定义
//初始化视频端口
int RV1126_vo_init( HAL_DRIVER * hal , int port , void * lpara )
{
    HI_LIVE_VIDEO_OUTPUT * drv = ( HI_LIVE_VIDEO_OUTPUT *)hal ;
    LIVE_VIDEO_OUTPUT * output = ( LIVE_VIDEO_OUTPUT * ) lpara ;
    
    if( port >= MAX_VO_PORT )
    {
        printf("RV1126_vo_init , port(%d) not supported !" , port ) ;
        return 0 ;
    }
                
    drv->port[port].enabled          =  1      ;
    
    switch( output->output_type )
    {
        case VIDEO_OUTPUT_BT656 :
        case VIDEO_OUTPUT_BT1120:
            drv->port[port].display_fmt = IMAGE_NV12 ;
            break ;  
            
        case VIDEO_OUTPUT_VGA :
        case VIDEO_OUTPUT_LCD :
            drv->port[port].display_fmt =  IMAGE_RGB888 ;
            break ;

          
        default :
            drv->port[port].display_fmt =  "UNKONW" ;
            break ;
    }
        
    return 1 ;
}

//关闭视频端口
int RV1126_vo_exit(  HAL_DRIVER * hal , int port )
{
    HI_LIVE_VIDEO_OUTPUT * drv = ( HI_LIVE_VIDEO_OUTPUT *)hal ;
    
    if( port >= MAX_VO_PORT )
        return 0 ;
    
    if( !drv->port[port].enabled)
        return 0 ;
    
    drv->port[port].enabled =  0  ;
    
    ////rkmedia资源不释放/////
    return 1 ;
}

//创建视频通道
int RV1126_vo_create( HAL_DRIVER * hal , int port , int ch , int data , void * lpara )
{
    HI_LIVE_VIDEO_OUTPUT * drv = ( HI_LIVE_VIDEO_OUTPUT *)hal ;
    LIVE_VIDEO_CHANNEL * video = ( LIVE_VIDEO_CHANNEL*)lpara ;    
   
        
    if( port >= MAX_VO_PORT  )
    {
        printf("RV1126_vo_create , port(%d) not supported !" , port  ) ;
        return 0 ;
    } 
    
    if( !drv->port[port].enabled )
    {
        printf("RV1126_vo_create , port(%d) disabeled !" , port ) ;
        return 0 ;
    }
    
    std::string  flow_param ;
    std::string  stream_param ;  
        
    PARAM_STRING_APPEND(flow_param, KEY_NAME, "image-filter"); 
    PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE , INTERNAL_PIX_FMT); //input always ARGB
    //Set output buffer type.
    PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, drv->port[port].display_fmt );
    //Set output buffer size.
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_WIDTH , video->width );
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_HEIGHT, video->height);
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_WIDTH , video->width );
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_HEIGHT, video->height);
    if( video->fps )
        PARAM_STRING_APPEND_TO(flow_param, KEY_FPS, video->fps ); //ASYNC mode , 30fps
    
    //buffer pool
    PARAM_STRING_APPEND   (flow_param, KEY_MEM_TYPE, KEY_MEM_HARDWARE);
    PARAM_STRING_APPEND_TO(flow_param, KEY_MEM_CNT, 3);        
        
    ImageRect src_rect = {0,0,0,0}; //full image from source input
    PARAM_STRING_APPEND(stream_param , KEY_BUFFER_RECT, easymedia::ImageRectToString(src_rect).c_str());
    //PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_ROTATE, 90);  //current LCD output need rotate  
    
    flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);
        
    auto image_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>("filter" , flow_param.c_str());
    if (!image_flow) 
    {
        printf("Create image filter flow failed\n" );
        return 0 ;
    }

    flow_param   = "" ;
    stream_param = "" ;  
    
    PARAM_STRING_APPEND(flow_param, KEY_NAME, "drm_output_stream");
    //PARAM_STRING_APPEND(flow_param, KEK_THREAD_SYNC_MODEL, KEY_SYNC);
    PARAM_STRING_APPEND(flow_param, KEK_INPUT_MODEL, KEY_DROPFRONT);    
    
    PARAM_STRING_APPEND(stream_param, KEY_DEVICE , DRM_DEVICE_PATH );
    PARAM_STRING_APPEND(stream_param, KEY_OUTPUTDATATYPE  , drv->port[port].display_fmt);
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_WIDTH , video->width );
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_HEIGHT, video->height);
    PARAM_STRING_APPEND_TO(stream_param, KEY_FPS, 30);
    PARAM_STRING_APPEND(stream_param, KEY_PLANE_TYPE, KEY_PRIMARY);
    //PARAM_STRING_APPEND_TO(param, KEY_ZPOS, zindex);
    flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);

    auto display_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>("output_stream", flow_param.c_str());
    if (!display_flow) 
    {
        printf("Create drm stream flow failed\n" );
        return 0 ;
    }
    image_flow->AddDownFlow( display_flow , 0 , 0 ) ;
        
    drv->port[port].width        = video->width  ;
    drv->port[port].height       = video->height ;  
    drv->port[port].image_flow   = image_flow    ;  
    drv->port[port].display_flow = display_flow  ;  

    return 1 ;
}


//ioctl
int RV1126_vo_ioctl( HAL_DRIVER * hal , int port , int ch , int op   , int data , void * lpara )
{

    int ret ;
    
    if( port >= MAX_VO_PORT  )
    {
        printf("RV1126_vo_ioctl , port(%d) not supported !" , port  ) ;
        return 0 ;
    } 
    
    ret = 0 ;
    switch( op )
    {    
    case OP_GET_FRAME :        
    case OP_RELEASE_FRAME :                      
        break ;       
    }
    
    return ret ;
}



//视频通道使能
int RV1126_vo_start( HAL_DRIVER * hal , int port , int ch , int flag )
{
    HI_LIVE_VIDEO_OUTPUT * drv = ( HI_LIVE_VIDEO_OUTPUT *)hal ;
    
    if( port >= MAX_VO_PORT )
    {
        printf("RV1126_vo_start , port(%d) not supported !" , port ) ;
        return 0 ;
    } 
    
    if( !drv->port[port].enabled )
    {
        printf("RV1126_vo_start , port(%d) disabeled !" , port ) ;
        return 0 ;
    }

    return 1 ;    
}


//绑定视频口
int RV1126_vo_bind( HAL_DRIVER * hal , int port , int ch , int flag , void * obj )
{
    HI_LIVE_VIDEO_OUTPUT * drv = ( HI_LIVE_VIDEO_OUTPUT *)hal ;
    HAL_OBJECT * src = ( HAL_OBJECT * ) obj ;    
    RV1126_HAL_PRIV    priv ;
    
    if( port >= MAX_VO_PORT  )
    {
        printf("RV1126_vo_bind , paras not supported !") ;
        return 0 ;
    }         
    
    priv.flow = nullptr ;    
    src->drv->ioctl( src->drv , src->port , src->channel , OP_GET_PRIVATE , 0 , &priv ) ; 
    
    if( priv.flow != nullptr )   
        priv.flow->AddDownFlow( drv->port[port].image_flow , 0 , 0 ) ;
    
    
    return 1 ;    
}  





static HI_LIVE_VIDEO_OUTPUT   HI_live_video_output = {
    {
    "VideoOutput"  , "RV1126 VO" , 1 ,
    RV1126_vo_init ,
    RV1126_vo_exit ,
    RV1126_vo_create ,
    NULL             ,
    RV1126_vo_ioctl  ,
    RV1126_vo_bind   ,
    RV1126_vo_start  ,
    NULL             ,
    }
};


void RV1126_unregister_video_output( void )
{
    unregister_driver("VideoOutput");    
}

void RV1126_register_video_output( void )
{
    register_driver( (HAL_DRIVER *)&HI_live_video_output , -1 , NULL ) ;    
}

