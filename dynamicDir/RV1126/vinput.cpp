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

#include <memory>
#include <linux/videodev2.h>

//rkaiq for ISP
#include "rk_aiq_user_api_sysctl.h"
#include "rk_aiq_user_api_imgproc.h"
#include "mediactl/mediactl.h"

//rkmedia 
#include "buffer.h"
#include "filter.h"
#include "image.h"
#include "key_string.h"
#include "flow.h"
#include "control.h"
#include "stream.h"

#include "hal-private.h"

//#include "sys-core.h"
//#include "sys-service.h"
#include "sys-hal.h"

/* Private v4l2 event */
#define CIFISP_V4L2_EVENT_STREAM_START (V4L2_EVENT_PRIVATE_START + 1)
#define CIFISP_V4L2_EVENT_STREAM_STOP  (V4L2_EVENT_PRIVATE_START + 2)

//与硬件相关的定义，直接查找后固定下来
//可以通过dmesg 和 media-ctl 查询
#define  ISP_IQ_FILE_DIR     "/oem/etc/iqfiles/"
#define  ISP_CAM0_PATH       "/dev/video16"
#define  ISP_CAM1_PATH       "/dev/video22"
#define  CIF_CAM_PATH        "/dev/video0"
#define  CAM_INPUT_FMT       IMAGE_NV12


#define  MAX_VI_PORT         4  //0 - CIF(BT656) 1 - CSI0  2 - CSI1
#define  MAX_VI_CHANNEL      4


typedef struct
{
    int    width  ;
    int    height ;

    //回调帧
    std::mutex   cache_mutex  ;        
    std::shared_ptr<easymedia::MediaBuffer>  cache_images ;
    int  cache_count ;
 
    RV1126_HAL_BUFFER  holded ; //回调函数保存的        
    
    std::shared_ptr<easymedia::Flow>  channel_flow ;     
}HI_VI_CHANNEL  ;

//视频输入端口定义
typedef struct
{
    int    enabled      ;
    int    width        ;
    int    height       ;  

    int    fake_count   ;
    
    char   camera_path[128] ; 
    
    rk_aiq_sys_ctx_t * isp_context ;
    std::shared_ptr<easymedia::Flow>  input_flow  ;
    std::shared_ptr<easymedia::Flow>  dummy_flow  ; //让输入Flow工作起来，必须有一个连接的flow才行
               
    HI_VI_CHANNEL  channel[MAX_VI_CHANNEL] ;
    
}HI_VI_PORT ;


typedef struct
{
    HAL_DRIVER   hal ;
        
    HI_VI_PORT  port[MAX_VI_PORT] ;
}HI_VIDEO_INPUT ;


static void RV1126_vi_callback( void * handler , std::shared_ptr<easymedia::MediaBuffer> mb )
{
    HI_VI_CHANNEL * vi = ( HI_VI_CHANNEL *) handler ;
    
    vi->cache_mutex.lock()   ; 
    
    if( vi->cache_count >= 0 )
    {
        vi->cache_images.reset() ; //释放原来的帧         
        vi->cache_images = mb    ; //获取新的帧 
        vi->cache_count  = 1     ;        
    }
    
    vi->cache_mutex.unlock() ;
}


static void RV1126_isp_init( int idx , int normal , HI_VI_PORT * video )
{        
    int ret  = 0 ;

    rk_aiq_static_info_t info ;
    rk_aiq_uapi_sysctl_enumStaticMetas(idx, &info);
    
    //RK_AIQ_WORKING_MODE_NORMAL RK_AIQ_WORKING_MODE_ISP_HDR3
    rk_aiq_working_mode_t mode  = normal ? RK_AIQ_WORKING_MODE_NORMAL : RK_AIQ_WORKING_MODE_ISP_HDR2;     
    
    video->isp_context = rk_aiq_uapi_sysctl_init(info.sensor_info.sensor_name, ISP_IQ_FILE_DIR , NULL, NULL );
    if ( !video->isp_context ) 
    {
        printf("%s: ISP sysctl init fail \n", __FUNCTION__ );
        return ;
    }
    rk_aiq_uapi_sysctl_setMulCamConc(video->isp_context, true);
    
    
    ret  = rk_aiq_uapi_sysctl_prepare( video->isp_context , video->width, video->height, mode ) ; 
    if( ret )
    {
        rk_aiq_uapi_sysctl_deinit(video->isp_context);
        printf("%s: ISP Engine prepare failed !\n", __FUNCTION__);        
        return  ;
    }
    
    /////配置颜色，禁止黑白//////////
    rk_aiq_cpsl_cfg_t  cpsl_cfg ;
    
    memset( &cpsl_cfg , 0 , sizeof(cpsl_cfg) ) ;
    cpsl_cfg.mode = RK_AIQ_OP_MODE_AUTO;
    cpsl_cfg.gray_on = false;
    cpsl_cfg.u.a.sensitivity = 5;
    cpsl_cfg.u.a.sw_interval = 5;  
    cpsl_cfg.lght_src = RK_AIQ_CPSLS_MIX;
    rk_aiq_uapi_sysctl_setCpsLtCfg( video->isp_context , &cpsl_cfg );
    
    ret = rk_aiq_uapi_sysctl_start( video->isp_context );    
    if( ret )
    {
        rk_aiq_uapi_sysctl_deinit(video->isp_context);
        printf("%s: ISP Engine prepare failed !\n", __FUNCTION__);        
        return  ;
    }
    
    printf("ISP[%d]:%s start OK !\n", idx , info.sensor_info.sensor_name);
    return  ;
}


static void RV1126_isp_exit( HI_VI_PORT * video )
{
    if (video->isp_context ) 
    {
        rk_aiq_uapi_sysctl_stop(video->isp_context, false);
        rk_aiq_uapi_sysctl_deinit(video->isp_context);
    }
}


//////////////////////////////////////////////////////////////////////////////
//Live video input 定义
//初始化视频端口
int RV1126_vi_init( HAL_DRIVER * hal , int port , void * lpara )
{
    HI_VIDEO_INPUT   * drv   = ( HI_VIDEO_INPUT *)hal ;   
    LIVE_VIDEO_INPUT * vi    = ( LIVE_VIDEO_INPUT * ) lpara ;    
    

    if( port >= MAX_VI_PORT )
    {
        printf("RV1126_vi_init , port(%d) not supported !" , port ) ;
        return 0 ;
    }
    
    //
    switch ( vi->input_type )
    {
        //不需要ISP处理
        case VIDEO_INPUT_BT1120:    
        case VIDEO_INPUT_BT656 :
        case VIDEO_INPUT_P2050 :        
        case VIDEO_INPUT_DC :    
            drv->port[port].isp_context =  NULL ;   
            break ;            
        
        //需要启动ISP处理
        default :
            RV1126_isp_init( port -1,  1 , &drv->port[port] ) ;
            break ;
        
    }    
        
    drv->port[port].width       =  vi->frame_width ;
    drv->port[port].height      =  vi->frame_height ; 
    if( port == 0 )
        strcpy( drv->port[port].camera_path , CIF_CAM_PATH ) ;
    else 
        strcpy( drv->port[port].camera_path , port == 1 ? ISP_CAM0_PATH : ISP_CAM1_PATH ) ;    

    std::string flow_param = "";
    PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, vi->input_type == VIDEO_INPUT_DC ? IMAGE_NV16 : CAM_INPUT_FMT );
    auto dummy = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>( "dummy_flow" , flow_param.c_str() );
    if( !dummy  ) 
    {
        printf( "Port[%d] Create dummy flow failed\n" , port );
        return 0 ;
    }
    drv->port[port].dummy_flow = dummy   ;
    drv->port[port].input_flow = nullptr ;    
    
    flow_param = "" ;    
    PARAM_STRING_APPEND   (flow_param, KEY_NAME, "v4l2_capture_stream");
    PARAM_STRING_APPEND   (flow_param, KEK_THREAD_SYNC_MODEL, KEY_SYNC);
    PARAM_STRING_APPEND   (flow_param, KEK_INPUT_MODEL, KEY_DROPFRONT);
    PARAM_STRING_APPEND_TO(flow_param, KEY_INPUT_CACHE_NUM, 5);   //input cache 
    
    std::string  stream_param ;  
    PARAM_STRING_APPEND_TO(stream_param, KEY_USE_LIBV4L2, 1);
    PARAM_STRING_APPEND   (stream_param, KEY_DEVICE, drv->port[port].camera_path );
    PARAM_STRING_APPEND   (stream_param, KEY_V4L2_CAP_TYPE, KEY_V4L2_C_TYPE(VIDEO_CAPTURE));
    PARAM_STRING_APPEND   (stream_param, KEY_V4L2_MEM_TYPE, KEY_V4L2_M_TYPE(MEMORY_DMABUF));
    PARAM_STRING_APPEND_TO(stream_param, KEY_FRAMES, 5);  //4会导致丢帧 // V4L2的队列大小
    PARAM_STRING_APPEND   (stream_param, KEY_OUTPUTDATATYPE, vi->input_type == VIDEO_INPUT_DC ? IMAGE_NV16 : CAM_INPUT_FMT );
    if( vi->input_mode  == VIDEO_MODE_BYPASS )
    {   //BT1120 or BT656 or DC input , no ISP needed
        //or output full isp image
        PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_WIDTH , drv->port[port].width );
        PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_HEIGHT, drv->port[port].height);
    }else{
        //scale to max size 1920x1080
        PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_WIDTH , MAX_VPSS_OUT_WIDTH );
        PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_HEIGHT, MAX_VPSS_OUT_HEIGHT);        
    }
    flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);    
    
    auto cap = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>( "source_stream", flow_param.c_str());
    if( !cap ) 
    {
        printf( "Port[%d] Create v4l2_capture_stream %s failed\n" , port , drv->port[port].camera_path );
        return 0 ;
    }
    drv->port[port].input_flow = cap ;
    
    //增加下行FLOW，启动Capture Stream
    drv->port[port].input_flow->AddDownFlow( drv->port[port].dummy_flow , 0 , 0 ) ;      
    drv->port[port].enabled     =  1     ;
    printf("Vi device init OK !" );
        
    return 1 ;
}

//关闭视频端口
int RV1126_vi_exit(  HAL_DRIVER * hal , int port )
{
    HI_VIDEO_INPUT * drv = ( HI_VIDEO_INPUT *)hal ;
        
    if( port >= MAX_VI_PORT )
        return 0 ;
    
    if( !drv->port[port].enabled)
        return 0 ;

    drv->port[port].input_flow->RemoveDownFlow( drv->port[port].dummy_flow ) ;  
       
    drv->port[port].dummy_flow.reset() ;    
    drv->port[port].input_flow.reset() ;
     
    RV1126_isp_exit( &drv->port[port] ) ;
    
    drv->port[port].enabled =  0  ;    
    return 1 ;   
}

//创建视频通道
int RV1126_vi_create( HAL_DRIVER * hal , int port , int ch , int data , void * lpara )
{
    LIVE_VIDEO_CHANNEL * live = ( LIVE_VIDEO_CHANNEL * )lpara ;
    HI_VIDEO_INPUT * drv    = ( HI_VIDEO_INPUT *)hal ;    
    HI_VI_PORT  * vi ;        
    
    if( port >= MAX_VI_PORT || ch >= MAX_VI_CHANNEL )
    {
        printf("RV1126_vi_create , port(%d) ch(%d) not supported !" , port , ch ) ;
        return 0 ;
    } 
    
    vi = &drv->port[port] ;
    if( !vi->enabled )
    {
        printf("RV1126_vi_create , port(%d) disabeled !" , port ) ;
        return 0 ;
    }
    
    std::string  flow_param ;
    std::string  stream_param ;  
        
    PARAM_STRING_APPEND(flow_param, KEY_NAME, "image-filter"); 
    PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE , live->type == VIDEO_INPUT_DC ? IMAGE_NV16 : CAM_INPUT_FMT );
    PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, CAM_INPUT_FMT );
    //Set output buffer size.
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_WIDTH , live->width );
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_HEIGHT, live->height);
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_WIDTH , live->width );
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_HEIGHT, live->height);
    
    
    //buffer pool
    PARAM_STRING_APPEND   (flow_param, KEY_MEM_TYPE, KEY_MEM_HARDWARE);
    PARAM_STRING_APPEND_TO(flow_param, KEY_MEM_CNT, data );   
    
    //必须至少设置一个stream参数     
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_ROTATE, live->mode );
    flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);   
    auto image_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>("filter" , flow_param.c_str());
    if (!image_flow) 
    {
        printf("Create image filter flow failed\n" );
        return 0 ;
    }

    //设置回调函数
    image_flow->SetOutputCallBack( &vi->channel[ch] , RV1126_vi_callback ) ;

    vi->channel[ch].width    = live->width  ;
    vi->channel[ch].height   = live->height ;
    vi->channel[ch].cache_count  = 0 ;        
    vi->channel[ch].channel_flow = image_flow ;      
    vi->input_flow->AddDownFlow( image_flow , 0 , 0 ) ;    

    return 1 ;
}

//销毁视频通道
int RV1126_vi_destroy( HAL_DRIVER * hal , int port , int ch , void * lpara )
{
    HI_VIDEO_INPUT * drv    = ( HI_VIDEO_INPUT *)hal ;    
    HI_VI_PORT  * vi ;        
    
    if( port >= MAX_VI_PORT || ch >= MAX_VI_CHANNEL )
    {
        printf("RV1126_vi_create , port(%d) ch(%d) not supported !" , port , ch ) ;
        return 0 ;
    } 
    
    vi = &drv->port[port] ;
    if( !vi->enabled )
    {
        printf("RV1126_vi_destroy , port(%d) disabeled !" , port ) ;
        return 0 ;
    }
    
  
        
    /////
    vi->channel[ch].cache_mutex.lock()   ;      
    if( vi->channel[ch].cache_count == 1 )
        vi->channel[ch].cache_images.reset( ) ; 
    vi->channel[ch].cache_count  = -1 ; 
    vi->channel[ch].cache_mutex.unlock() ;

    /////
    vi->input_flow->RemoveDownFlow( vi->channel[ch].channel_flow ) ; 
    vi->channel[ch].channel_flow.reset( ) ; 

    return 1 ;         
}


int RV1126_vi_get_chn_frame( HAL_DRIVER * hal , int port , int ch, AV_STREAM_FRAME *av )
{
    HI_VIDEO_INPUT * drv  = ( HI_VIDEO_INPUT *)hal ;
    HI_VI_CHANNEL * channel ;
    int valid ;
    
    channel = &drv->port[port].channel[ch] ;
    valid    = 0 ;
    channel->cache_mutex.lock() ;
    if( channel->holded.valid == 0 && channel->cache_count > 0 )
    {
        channel->holded.valid  = 1 ; 
        channel->holded.buffer = std::static_pointer_cast<easymedia::ImageBuffer>(channel->cache_images) ; 
        valid = 1 ;
    }
    channel->cache_mutex.unlock() ;
    
    //有数据
    if( valid )
    {
        auto img       = channel->holded.buffer ;
        
        av->channel = ch ;
        av->type    = VIDEO_YUV_VI_FRAME ;
        av->pts     = img->GetUSTimeStamp() ;
        av->length  = img->GetValidSize();
        av->width   = img->GetWidth () ;
        av->height  = img->GetHeight() ;
        av->fps     = 0 ;
        av->ref     = &channel->holded ;
        av->stream  = img->GetPtr()  ;
        av->phy_addr[0] = img->GetFD() ;
        av->vir_addr[0] = img->GetPtr() ;
        av->stride[0]   = img->GetVirWidth () ;
        return 1 ;
    }
    
    av->width   = 0 ;
    av->height  = 0 ;
    av->length  = 0 ;
    av->type    = -1;
    av->ref     = NULL ;
    av->stream  = NULL ;
    return 0 ;
}

//配置ISP去雾
//idx = enable / disable
int RV1126_vi_set_defog( HAL_DRIVER * hal , int port , int flag , int strength )
{
    return 1 ;
}


//tricky way 
int RV1126_vi_query_status( HAL_DRIVER * hal , int port , int * output )
{
    HI_VIDEO_INPUT * drv  = ( HI_VIDEO_INPUT *)hal ;
    
    drv->port[port].fake_count++ ;
    *output = drv->port[port].fake_count ;
    
    return 1 ;
}

int RV1126_vi_get_private ( HAL_DRIVER * hal , int port , int ch , void * lpara )
{
    RV1126_HAL_PRIV * priv = ( RV1126_HAL_PRIV * )lpara ;
    HI_VIDEO_INPUT  * drv  = (HI_VIDEO_INPUT *)hal ;
    
    if( ch < 0 )    
        priv->flow = drv->port[port].input_flow;
    else
        priv->flow = drv->port[port].channel[ch].channel_flow ;
    
    return 1 ;    
}

int RV1126_vi_config_channel( HAL_DRIVER * hal , int port , int ch , int pattern )
{    
    HI_VIDEO_INPUT * drv  = (HI_VIDEO_INPUT *)hal ;
    
    drv->port[port].channel[ch].channel_flow->Control( VO_OP_SET_PATTERN , &pattern );
    return 1 ;
}

int RV1126_vi_set_zoom( HAL_DRIVER * hal , int port , int ch , void * para )
{    
    HI_VIDEO_INPUT   *  drv  = (HI_VIDEO_INPUT *)hal ;
    LIVE_VIDEO_ZOOM * zoom ;
    IMAGE_ZOOM_INFO   info ;
    

    zoom = (LIVE_VIDEO_ZOOM*)para ;

    info.x = (zoom->x + 3 )/4*4;
    info.y = (zoom->y + 3 )/4*4;
    info.width  = (zoom->width  + 3 )/4*4;
    info.height = (zoom->height + 3 )/4*4;
    drv->port[port].channel[ch].channel_flow->Control( IMG_OP_ZOOM , &info );
    return 1 ;
}
int RV1126_isp_dehaze_disable(HAL_DRIVER * hal , int port)
{
    HI_VIDEO_INPUT * drv  = ( HI_VIDEO_INPUT *)hal ;
    if( rk_aiq_uapi_disableDhz(drv->port[port].isp_context) )
    {
        printf(" dehaze disable failed !\n");
    }
   // rk_aiq_uapi_setMDhzStrth(drv->port[port].isp_context, false, 0);
    
    return 1;
}

//ioctl timeout单位毫秒
int RV1126_vi_ioctl( HAL_DRIVER * hal , int port , int ch , int op   , int idx , void * lpara )
{
    HI_VIDEO_INPUT * drv = ( HI_VIDEO_INPUT *)hal ;  
    AV_STREAM_FRAME    * av ;
    RV1126_HAL_BUFFER  * buf; 
    int ret ;
          
    if( port >= MAX_VI_PORT  || ch >= MAX_VI_CHANNEL  )
    {
        printf("RV1126_vi_ioctl , port(%d) ch(%d) not supported !" , port , ch ) ;
        return 0 ;
    } 

    if( !drv->port[port].enabled )
    {
  //    printf("RV1126_vi_ioctl , port(%d) disabeled !" , port ) ;
        return 0 ;
    }    
    opMode_t op_mode;    
    ret = 0 ;
    
    switch( op )
    {  
    case OP_GET_PRIVATE : //返回指定channel的flow
        ret = RV1126_vi_get_private( hal , port , ch , lpara ) ;
        break ;
    
    case OP_GET_FRAME :
        av  = ( AV_STREAM_FRAME * ) lpara ;
        ret = RV1126_vi_get_chn_frame( hal , port, ch, av );
        break ;    
    
    case OP_RELEASE_FRAME :
        ret = 1 ;
        av  = ( AV_STREAM_FRAME *)lpara ;
        if( av->ref == NULL )
            break ;
            
        buf= ( RV1126_HAL_BUFFER*)av->ref ;
        if( buf->valid )
        {
            buf->valid = 0 ;
            buf->buffer.reset() ; //clear it
        }        
        break ; 
         
    case OP_CTRL_DEFOG :
        ret = RV1126_vi_set_defog( hal ,port , idx , *((int *)lpara) );
        break ;
 
    case OP_GET_VI_INTCNT :
        ret = RV1126_vi_query_status( hal , port , (int*)lpara ) ;
        break ;

    case OP_CONFIG_CHANNEL :  //配置通道的DropPattern
        ret = RV1126_vi_config_channel( hal , port , ch , idx );     
        break ;

    case OP_ZOOM_PARA :
        ret = RV1126_vi_set_zoom( hal , port , ch , lpara ) ;
        break ;
    case OP_DEHAZE_DISABLE:
        printf("  dehaze disable \n");
        ret = RV1126_isp_dehaze_disable(hal, port);
        break;
    case OP_DEHAZE_AUTO_MODE:
        printf("  dehaze AUTO \n");
       
        op_mode =  OP_AUTO ;
        ret = rk_aiq_uapi_setDhzMode(drv->port[port].isp_context, op_mode);
        if(ret)
             printf("[ set auto failed ! ]\n") ;
        break;

    default :                  
        printf("OP[%X] not support !" , op ) ;
        break ;    
    }

    return ret ;
}





//视频通道使能
int RV1126_vi_start( HAL_DRIVER * hal , int port , int ch , int flag )
{
    UNUSED(hal );  
    UNUSED(flag);
    
    if( port >= MAX_VI_PORT || ch >= MAX_VI_CHANNEL )
    {
        printf("RV1126_vi_start , port(%d) ch(%d) not supported !" , port , ch ) ;
        return 0 ;
    } 
          
    return 1 ;    
}


static HI_VIDEO_INPUT   hi_video_input = {
    {
    "VideoInput"  , "RV1126 VI" , 1 ,
    RV1126_vi_init ,
    RV1126_vi_exit ,
    RV1126_vi_create ,
    RV1126_vi_destroy,
    RV1126_vi_ioctl  ,
    NULL             ,
    RV1126_vi_start  ,
    NULL             ,
    }
};



void RV1126_unregister_video_input( void )
{
    unregister_driver("VideoInput");    
}

void RV1126_register_video_input( void )
{
    register_driver( (HAL_DRIVER *)&hi_video_input  , -1 , NULL ) ;    
}

