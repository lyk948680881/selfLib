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

//rkmedia 
#include "buffer.h"
#include "filter.h"
#include "image.h"
#include "key_string.h"
#include "media_config.h"
#include "flow.h"
#include "control.h"
#include "stream.h"

#include "hal-private.h"

#include "sys-core.h"
//#include "sys-service.h"
#include "sys-hal.h"


#define  MAX_AI_PORT         1

typedef struct
{
    void  * stream  ;
    U64     pts     ;   //时间戳
    int     length  ;    
}PCM_PACKET ;

//视频输入端口定义
typedef struct
{
    int    enabled      ;
    
    int    sample_rate    ;    
    int    sample_width   ;  
    int    sample_format  ;    //
    int    sample_channels;    //
    int    sample_count   ;    //每帧采样数
    
    std::shared_ptr<easymedia::Flow>  input_flow   ;
    std::shared_ptr<easymedia::Flow>  output_flow  ; //数据输出，可以是input_flow或者aenc
    
    
    std::mutex                 pcm_mutex ;
    std::vector<PCM_PACKET>    pcm_pkt   ;           
}HI_AI_PORT ;


typedef struct
{
    HAL_DRIVER   hal ;
        
    HI_AI_PORT  port[MAX_AI_PORT] ;
}HI_AUDIO_INPUT ;


extern MEM_POOL_ID  hi_pool_id  ;       //全局的内存池定义

static void RV1126_ai_callback( void * handler , std::shared_ptr<easymedia::MediaBuffer> mb )
{
    HI_AI_PORT * ai = ( HI_AI_PORT *) handler ;

    PCM_PACKET  pkt ;

    pkt.length = mb->GetValidSize() ;
    pkt.pts    = mb->GetUSTimeStamp() ;
    pkt.stream = core_pool_alloc( hi_pool_id , pkt.length , 100 ) ; //1000ms
    
    if( pkt.stream == NULL ) 
        return ;
    memcpy( pkt.stream , mb->GetPtr() , pkt.length ) ;  //save to our pool buffer
        
    ai->pcm_mutex.lock() ;
    ai->pcm_pkt.push_back( pkt ) ;
    ai->pcm_mutex.unlock() ;
    
}

//////////////////////////////////////////////////////////////////////////////
//Live audio input 定义
//初始化视频端口
int RV1126_ai_init( HAL_DRIVER * hal , int port , void * lpara )
{
    HI_AUDIO_INPUT   * drv   = ( HI_AUDIO_INPUT *)hal ;   
    LIVE_AUDIO_PORT  * ai    = ( LIVE_AUDIO_PORT * ) lpara ;    
    

    if( port >= MAX_AI_PORT )
    {
        printf("RV1126_ai_init , port(%d) not supported !" , port ) ;
        return 0 ;
    }
    
    drv->port[port].sample_rate     =  ai->sample_rate   ;
    drv->port[port].sample_width    =  ai->sample_width  ;
    drv->port[port].sample_format   =  ai->sample_format ; 
    drv->port[port].sample_channels =  ai->sample_channels ; 
    drv->port[port].sample_count    =  ai->sample_count    ;
    
    std::string  flow_param = "";
    std::string  stream_param = "" ;
    
    SampleFormat fmt = SAMPLE_FMT_S16 ;
    if( ai->sample_format == LIVE_AUDIO_AAC )
        fmt = SAMPLE_FMT_FLTP ;
         
    //创建输入flow
    PARAM_STRING_APPEND(flow_param, KEY_NAME    , "alsa_capture_stream");
    PARAM_STRING_APPEND(stream_param, KEY_DEVICE, "default");
    PARAM_STRING_APPEND(stream_param, KEY_SAMPLE_FMT , SampleFmtToString(fmt));
    PARAM_STRING_APPEND_TO(stream_param, KEY_CHANNELS, ai->sample_channels );
    PARAM_STRING_APPEND_TO(stream_param, KEY_FRAMES  , ai->sample_count    );
    PARAM_STRING_APPEND_TO(stream_param, KEY_SAMPLE_RATE, ai->sample_rate  );
    flow_param    = easymedia::JoinFlowParam(flow_param, 1, stream_param);
    auto cap = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>( "source_stream", flow_param.c_str());
    if( !cap ) 
    {
        printf( "Port[%d] Create alsa_capture_stream failed\n" , port  );
        return 0 ;
    }
    drv->port[port].input_flow = cap ;   
        
    if( ai->sample_format == LIVE_AUDIO_LPCM )
    {
        PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE, AUDIO_PCM_S16 );
        auto dummy = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>( "dummy_flow" , flow_param.c_str() );
        if( !dummy  ) 
        {
            printf( "Port[%d] Create dummy flow failed\n" , port );
            return 0 ;
        }
        //增加下行FLOW，启动Capture Stream
        drv->port[port].input_flow->AddDownFlow( dummy , 0 , 0 ) ; 
        drv->port[port].output_flow = cap ; //assign input to it   
         
    }else {
        MediaConfig cfg ;
        SampleInfo  info = {fmt , ai->sample_channels , ai->sample_rate , 0 };
        
        auto &ac = cfg.aud_cfg;
        ac.sample_info = info ;
        ac.bit_rate = 64000   ; //always 64Kbs
        cfg.type    = Type::Audio ;
        
        //create audio encoder
        flow_param   = "";
        stream_param = "" ;
        
        PARAM_STRING_APPEND(flow_param, KEY_NAME, "ffmpeg_aud");
        if( ai->sample_format == LIVE_AUDIO_AAC )        
        {
            PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE , AUDIO_PCM_FLTP);        
            PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, AUDIO_AAC   );
            stream_param.append(easymedia::to_param_string(cfg, AUDIO_AAC));
        }else if( ai->sample_format == LIVE_AUDIO_G711A ){ 
            PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE , AUDIO_PCM_S16);    
            PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, AUDIO_G711A );
            stream_param.append(easymedia::to_param_string(cfg, AUDIO_G711A));
        }else { 
            PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE , AUDIO_PCM_S16);    
            PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, AUDIO_G711U ); 
            stream_param.append(easymedia::to_param_string(cfg, AUDIO_G711U));    
        }       
        flow_param    = easymedia::JoinFlowParam(flow_param, 1, stream_param );
        auto enc_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>("audio_enc" , flow_param.c_str() ) ;
        if( !enc_flow ) 
        {
            printf( "Port[%d] Create audio encoder flow failed\n" , port  );
            return 0 ;
        }
        
        //增加下行FLOW，启动Capture Stream
        drv->port[port].input_flow->AddDownFlow( enc_flow , 0 , 0 ) ; 
        drv->port[port].output_flow = enc_flow ; //assign input to it                 
    }
    
    
    //设置回调函数
    drv->port[port].output_flow->SetOutputCallBack( &drv->port[port] , RV1126_ai_callback ) ;
    
    drv->port[port].enabled     =  1     ;
    
    printf("Audio Input device init OK !" );
        
    return 1 ;
}

//关闭视频端口
int RV1126_ai_exit(  HAL_DRIVER * hal , int port )
{
    HI_AUDIO_INPUT * drv = ( HI_AUDIO_INPUT *)hal ;
    
    if( port >= MAX_AI_PORT )
        return 0 ;
    
    if( !drv->port[port].enabled)
        return 0 ;

    drv->port[port].enabled =  0  ;
        
    ////rkmedia资源不释放/////
    return 1 ;   
}

//创建视频通道
int RV1126_ai_create( HAL_DRIVER * hal , int port , int ch , int data , void * lpara )
{   
    return 1 ;
}


int RV1126_ai_get_chn_frame( HAL_DRIVER * hal , int port , int ch, AV_STREAM_FRAME *av )
{
    HI_AUDIO_INPUT  * drv  = (HI_AUDIO_INPUT *)hal ;
    HI_AI_PORT    * ai ;
    
    PCM_PACKET      pkt ;
    int  flag = 0 ;
    
    ai = &drv->port[port] ;
    
    ai->pcm_mutex.lock()   ;
    if( ai->pcm_pkt.size() )
    {
        pkt = ai->pcm_pkt[0] ;
        ai->pcm_pkt.erase( ai->pcm_pkt.begin() ) ;
        flag = 1 ;
    }        
    ai->pcm_mutex.unlock() ;
    
    if( !flag ) 
        return 0 ;

    /*先填写头部*/
    av->channel = 0 ;
    av->width   = 0 ;
    av->height  = 0 ;    
    av->sample  = ai->sample_rate ;  
    
    if( ai->sample_format == LIVE_AUDIO_LPCM )    
        av->type    = AUDIO_PCM_FRAME ;
    else if( ai->sample_format == LIVE_AUDIO_AAC )  
        av->type    = AUDIO_AAC_FRAME ; 
    else 
        av->type    = AUDIO_G711_FRAME ; 
                       
    av->length  = pkt.length ;
    av->stream  = pkt.stream ;
    av->pts     = pkt.pts    ;
           
    return 1 ;    
}


int RV1126_ai_get_private ( HAL_DRIVER * hal , int port , int ch , void * lpara )
{
    RV1126_HAL_PRIV * priv = ( RV1126_HAL_PRIV * )lpara ;
    HI_AUDIO_INPUT  * drv  = (HI_AUDIO_INPUT *)hal ;
    
    priv->flow = drv->port[port].input_flow;
    
    return 1 ;    
}


//ioctl timeout单位毫秒
int RV1126_ai_ioctl( HAL_DRIVER * hal , int port , int ch , int op   , int idx , void * lpara )
{
    HI_AUDIO_INPUT * drv = ( HI_AUDIO_INPUT *)hal ;      
    int ret ;
          
    if( port >= MAX_AI_PORT  )
    {
        printf("RV1126_ai_ioctl , port(%d) ch(%d) not supported !" , port , ch ) ;
        return 0 ;
    } 

    if( !drv->port[port].enabled )
    {
        printf("RV1126_ai_ioctl , port(%d) disabeled !" , port ) ;
        return 0 ;
    }    
        
    ret = 0 ;
    
    switch( op )
    {  
    case OP_GET_PRIVATE : //返回指定channel的flow
        ret = RV1126_ai_get_private( hal , port , ch , lpara ) ;
        break ;
    
    case OP_GET_FRAME :        
        ret = RV1126_ai_get_chn_frame( hal , port, ch, ( AV_STREAM_FRAME * ) lpara );
        break ;    
    
    case OP_RELEASE_FRAME : 
        core_pool_free( hi_pool_id , ((AV_STREAM_FRAME *)lpara)->stream ) ;
        ret  = 1 ;
        break ;
         
   
    default :                  
        printf("OP[%X] not support !" , op ) ;
        break ;    
    }

    return ret ;
}





//视频通道使能
int RV1126_ai_start( HAL_DRIVER * hal , int port , int ch , int flag )
{
    UNUSED(hal);
    UNUSED(flag);
    
    if( port >= MAX_AI_PORT  )
    {
        printf("RV1126_ai_start , port(%d) ch(%d) not supported !" , port , ch ) ;
        return 0 ;
    }     
    return 1 ;    
}


static HI_AUDIO_INPUT   hi_audio_input = {
    {
    "AudioInput"  , "RV1126 AI" , 1 ,
    RV1126_ai_init ,
    RV1126_ai_exit ,
    RV1126_ai_create ,
    NULL             ,
    RV1126_ai_ioctl  ,
    NULL             ,
    RV1126_ai_start  ,
    NULL             ,
    }
};



void RV1126_unregister_audio_input( void )
{
    unregister_driver("AudioInput");    
}

void RV1126_register_audio_input( void )
{
    register_driver( (HAL_DRIVER *)&hi_audio_input  , -1 , NULL ) ;    
}

