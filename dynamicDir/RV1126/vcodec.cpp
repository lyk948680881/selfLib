/**************************************************
 *
 *Description:
 *      RV1126的HAL驱动 -- 视频CODEC部分
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

#include <vector>

//rkmedia 
#include "media_config.h"
#include "filter.h"
#include "key_string.h"
#include "control.h"
#include "stream.h"

#include "hal-private.h"

#include "sys-core.h"
//#include "sys-service.h"
#include "sys-hal.h"

#define  H264_PROFILE     66  // 66-base 77-main 100-high

#define  MAX_HI_VENC      4

#define  HI_H26X_TYPE     0
#define  HI_JPEG_TYPE     1

typedef struct
{
    void  * stream  ;
    U64     pts     ;   //时间戳
    int     length  ;
    int     i_frame ;
}CODEC_PACKET ;

//////////////////VIDEO ENC////////////////////
typedef struct
{
    int   enabled      ;
    VIDEO_ENCODER_CHANNEL para ; 
    
    std::mutex                 codec_mutex ;
    std::vector<CODEC_PACKET>  codec_pkt   ;  
        
    std::shared_ptr<easymedia::Flow> encoder ;
}VENC_CHANNEL  ;

typedef struct
{
    HAL_DRIVER   hal  ;
    int         type  ;
    VENC_CHANNEL  channel[MAX_HI_VENC] ;           
}HI_VIDEO_ENC ;


////////////////////////////////////////////////////////////////////////////////////////
//VIDEO ENC
////////////////////////////////////////////////////////////////////////////////////////
extern MEM_POOL_ID  hi_pool_id  ;       //全局的内存池定义

void encoder_output_callback(void *handle, std::shared_ptr<easymedia::MediaBuffer> mb) 
{
    VENC_CHANNEL  * channel = ( VENC_CHANNEL * )handle ;
    CODEC_PACKET  pkt ;

    pkt.length = mb->GetValidSize() ;
    pkt.pts    = mb->GetUSTimeStamp() ;
    pkt.i_frame= 0 ;
    pkt.stream = core_pool_alloc( hi_pool_id , pkt.length , 100 ) ; //1000ms
 
    if( pkt.stream == NULL ) 
        return ;
    memcpy( pkt.stream , mb->GetPtr() , pkt.length ) ;  //save to our pool buffer
    
    //check frame flag 
    if( mb->GetUserFlag() & easymedia::MediaBuffer::kIntra )
        pkt.i_frame = 1 ;
                 
    channel->codec_mutex.lock() ;
    channel->codec_pkt.push_back( pkt ) ;
    channel->codec_mutex.unlock() ;
}



int hi_venc_get_frame( HI_VIDEO_ENC *hi , int ch , AV_STREAM_FRAME * av )
{
    VENC_CHANNEL  * channel ;
    CODEC_PACKET  pkt ;
    int  flag = 0 ;
    
    channel = &hi->channel[ch] ;
    
    channel->codec_mutex.lock()   ;
    if( channel->codec_pkt.size() )
    {
        pkt = channel->codec_pkt[0] ;
        channel->codec_pkt.erase( channel->codec_pkt.begin() ) ;
        flag = 1 ;
    }        
    channel->codec_mutex.unlock() ;
    
    if( !flag ) 
        return 0 ;
    
    /*先填写头部*/
    av->channel = ch ;
    av->width   = channel->para.width  ;
    av->height  = channel->para.height ;    
    av->fps     = channel->para.fps    ;      
    av->type    = VIDEO_JPEG_FRAME     ;
    av->length  = pkt.length ;
    av->stream  = pkt.stream ;
    av->pts     = pkt.pts    ;
    
    if( hi->type ==  HI_H26X_TYPE )  //H26X
    {
        if ( hi->channel[ch].para.venc_type == VENC_TYPE_H265 )
        {
            av->type = pkt.i_frame ? VIDEO_I_H265 : VIDEO_P_H265 ;        
        }else{
            av->type = pkt.i_frame ? VIDEO_I_FRAME : VIDEO_P_FRAME ;       
        }    
    }
    
    return 1 ;    
}



int hi_venc_init( HAL_DRIVER * hal , int port , void * lpara )
{
    HI_VIDEO_ENC * hi  = ( HI_VIDEO_ENC *) hal ;    
    
    UNUSED(port);
    UNUSED(lpara);
    
    hi->type = HI_H26X_TYPE    ;    
    if( strcmp( hal->name , "JPEGEnc" ) == 0 ){
        hi->type = HI_JPEG_TYPE ;      
    }
       
    memset( hi->channel , 0 , sizeof(VENC_CHANNEL)*MAX_HI_VENC ) ;          
    return 1 ;
}

int hi_venc_exit( HAL_DRIVER * hal , int port  )
{
    UNUSED(hal);
    UNUSED(port);
    return 1 ;
}

int hi_venc_create  ( HAL_DRIVER * hal , int port , int ch , int data , void * lpara ) 
{
    VIDEO_ENCODER_CHANNEL * venc = ( VIDEO_ENCODER_CHANNEL *)lpara ;
    HI_VIDEO_ENC * hi = ( HI_VIDEO_ENC *) hal ; 
    
    UNUSED(data);
    UNUSED(port);
    
    if( ch >= MAX_HI_VENC )
    {
        printf("hi_venc_create , ch=%d not support !" , ch  ) ;
        return 0 ;
    }
    
    std::string  flow_param ;
    std::string  stream_param ;      
         
    PARAM_STRING_APPEND(flow_param, KEY_NAME, "rkmpp");
    PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE , IMAGE_NV12 );
    if( hi->type == HI_H26X_TYPE )
        PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, venc->venc_type == VENC_TYPE_H265 ? VIDEO_H265 : VIDEO_H264 );
    else
        PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, IMAGE_JPEG  ); //IMAGE_JPEG
              
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_WIDTH , venc->width);
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_HEIGHT, venc->height);
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_VIR_WIDTH, venc->width);
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_VIR_HEIGHT, venc->height);
    PARAM_STRING_APPEND_TO(stream_param, KEY_COMPRESS_BITRATE , venc->bitrate);
    PARAM_STRING_APPEND_TO(stream_param, KEY_COMPRESS_BITRATE_MAX, venc->bitrate * 20 / 16);
    PARAM_STRING_APPEND_TO(stream_param, KEY_COMPRESS_BITRATE_MIN, venc->bitrate * 3 / 4 );     
    PARAM_STRING_APPEND_TO(stream_param, KEY_PROFILE , H264_PROFILE ) ;
    if( hi->type == HI_H26X_TYPE )
    {
        //VBR
        const char *quality[5] = 
        {
            KEY_LOW, 
            KEY_MEDIUM,
            KEY_HIGH,
            KEY_HIGHER,
            KEY_HIGHEST               
        };
        PARAM_STRING_APPEND(stream_param, KEY_COMPRESS_QP_INIT  , "40") ;
        PARAM_STRING_APPEND(stream_param, KEY_COMPRESS_QP_STEP  , "10" ) ;
        PARAM_STRING_APPEND(stream_param, KEY_COMPRESS_QP_MIN   , "15") ;
        PARAM_STRING_APPEND(stream_param, KEY_COMPRESS_QP_MAX   , "51") ;
        PARAM_STRING_APPEND(stream_param, KEY_COMPRESS_QP_MAX_I , "48") ;
        PARAM_STRING_APPEND(stream_param, KEY_COMPRESS_QP_MIN_I , "30") ;

        PARAM_STRING_APPEND(stream_param, KEY_COMPRESS_RC_MODE , venc->cbr ? KEY_CBR : KEY_VBR ) ;
        PARAM_STRING_APPEND(stream_param, KEY_COMPRESS_RC_QUALITY , quality[venc->quality] ) ;
    }
    else
    {
        PARAM_STRING_APPEND_TO(stream_param, KEY_JPEG_QFACTOR , venc->quality * 20 + 10 ); //For JPEG 1 ~99
    }
    
    PARAM_STRING_APPEND(stream_param, KEY_FPS   , std::to_string(venc->fps).append("/1"));
    PARAM_STRING_APPEND(stream_param, KEY_FPS_IN, "30/1");
    PARAM_STRING_APPEND_TO(stream_param, KEY_FULL_RANGE, 1);
    PARAM_STRING_APPEND_TO(stream_param, KEY_ROTATION  , 0);  
    PARAM_STRING_APPEND_TO(stream_param, KEY_VIDEO_GOP , venc->gop );  
    
           
    flow_param    = easymedia::JoinFlowParam(flow_param, 1, stream_param);
    auto video_encoder = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>("video_enc" , flow_param.c_str());
    if (!video_encoder) 
    {
        printf("Create video_enc flow failed\n" );
        return 1 ;
    }
       
    hi->channel[ch].enabled  = 1 ;
    hi->channel[ch].encoder  = video_encoder ;
    memcpy( &hi->channel[ch].para , venc , sizeof(VIDEO_ENCODER_CHANNEL) ) ;
    
    video_encoder->SetOutputCallBack( &hi->channel[ch] , encoder_output_callback );      
    return 1 ;   
}


int hi_venc_destroy ( HAL_DRIVER * hal , int port , int ch , void * lpara )
{
    HI_VIDEO_ENC * hi = ( HI_VIDEO_ENC *) hal ;        
    
    UNUSED(lpara);
    UNUSED(port);
    
    if( ch >= MAX_HI_VENC )
        return 0 ;
    
    hi->channel[ch].enabled  = 0 ;
    
    ////RKMEDIA资源不释放////
    return 1 ;
}


int hi_venc_ioctl   ( HAL_DRIVER * hal , int port , int ch , int op   , int idx , void * lpara )     
{
    HI_VIDEO_ENC * hi = ( HI_VIDEO_ENC *) hal ;      
    int ret ;
    
    VIDEO_ENCODER_CHANNEL * attr ;
    
    UNUSED(port);
    UNUSED(idx);
    if( ch >= MAX_HI_VENC )
        return 0 ;
                       
    ret = 0 ;    
    switch( op )
    {
    case OP_GET_PRIVATE : //返回指定channel的flow
        ((RV1126_HAL_PRIV*)lpara)->flow = hi->channel[ch].encoder ; //void 
        break ;        
        
    case OP_GET_FRAME :
        ret = hi_venc_get_frame( hi , ch  , (AV_STREAM_FRAME *)lpara ) ;    
        break ;
        
    case OP_RELEASE_FRAME : 
        core_pool_free( hi_pool_id , ((AV_STREAM_FRAME *)lpara)->stream ) ;
        ret  = 1 ;
        break ;

    case OP_REQUEST_I_FRAME :
        printf("Insert i frame .");
        if( hi->channel[ch].enabled )
            easymedia::video_encoder_force_idr( hi->channel[ch].encoder ) ;
        break ; 
        
    case OP_CHANNEL_SET_ATTR :
        attr = (VIDEO_ENCODER_CHANNEL *)lpara ;
        if( hi->channel[ch].enabled && hi->channel[ch].para.cbr)
        {
            hi->channel[ch].para.bitrate = attr->bitrate ;
            easymedia::video_encoder_set_bps( hi->channel[ch].encoder , attr->bitrate ,  attr->bitrate / 4 , attr->bitrate * 20 / 16 ) ;
        }
        break ;     
    }
     
    return ret ;
}


int hi_venc_bind    ( HAL_DRIVER * hal , int port , int ch , int flag , void * obj ) 
{
    HI_VIDEO_ENC * hi = ( HI_VIDEO_ENC *) hal ;  
    HAL_OBJECT * src  = ( HAL_OBJECT *) obj ;
    RV1126_HAL_PRIV    priv ;
    
    UNUSED(flag);
    UNUSED(port);
    
    if( ch >= MAX_HI_VENC )
        return 0 ;
        
    if( !hi->channel[ch].enabled  )
        return 0 ;
    
    priv.flow = nullptr ;    
    src->drv->ioctl( src->drv , src->port , src->channel , OP_GET_PRIVATE , 0 , &priv ) ;     
    if( priv.flow != nullptr )   
        priv.flow->AddDownFlow( hi->channel[ch].encoder , 0 , 0 ) ;
    
    return 1 ;       
}

int hi_venc_start   ( HAL_DRIVER * hal , int port , int ch , int flag ) 
{
    UNUSED(hal);
    
    printf( "%s %d port = %d ch = %d,  flag = %d ", __FUNCTION__, __LINE__, port, ch ,  flag );
    
    if( ch >= MAX_HI_VENC )
        return 0 ;
        
    return 1 ;           
}



////////////////////////////////////////////////////////////////////////////////////////

HI_VIDEO_ENC  HI_venc_h264 = {
    {
    "H264Enc" , "RV1126 VENC" , 1 , 
    hi_venc_init ,
    hi_venc_exit ,
    hi_venc_create  ,
    hi_venc_destroy ,
    hi_venc_ioctl   ,
    hi_venc_bind    ,
    hi_venc_start   ,
    NULL            ,
    NULL            ,
    }
     
};

HI_VIDEO_ENC  HI_venc_jpeg = {
    {
    "JPEGEnc" , "RV1126 VENC" , 1 , 
    hi_venc_init ,
    hi_venc_exit ,
    hi_venc_create  ,
    hi_venc_destroy ,
    hi_venc_ioctl   ,
    hi_venc_bind    ,
    hi_venc_start   ,
    NULL            ,
    NULL            ,
    }
};





void RV1126_register_vcodec( void )
{
    register_driver( (HAL_DRIVER *)&HI_venc_h264  , 0 , NULL ) ;
    register_driver( (HAL_DRIVER *)&HI_venc_jpeg  , 0 , NULL ) ;
}


void RV1126_unregister_vcodec( void )
{
     
    unregister_driver("JPEGEnc");
    unregister_driver("H264Enc");    
}


