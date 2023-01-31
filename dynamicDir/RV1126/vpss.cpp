/**************************************************
 *
 *Description:
 *      RV1126的HAL驱动 -- Live部分
 *      NOTE : VPSS 输入是 NV12 , 内部转成ARGB处理、输出
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
#include <arm_neon.h>

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
#include "sys-gui.h"

#define  MAX_VPSS_PORT     8
#define  MAX_VPSS_CHANNEL  8
#define  MAX_VPSS_OVERLAY  32

#define  PIXEL_DIFF_LEVEL  16



typedef  void ( * RGB32_VIDEO_CALLBACK  ) ( VIDEO_MEM_INFO * video , int data  ) ;

typedef struct
{
    int    width  ;
    int    height ;
    
    std::shared_ptr<easymedia::Flow>  channel_flow ; 
}HI_VPSS_CHANNEL  ;


typedef struct 
{
    int    enable  ;
    int    width   ;
    int    height  ;

    int    bright   ; //亮度    0 ~  256  ( -128  ~ 128 )
    int    contrast ; //对比度  0 ~  256  ( 1/7.3 ~ 7.3 )
    int    deinterlace ; //去交织
    
    int    rgb32_callback_data ;
    RGB32_VIDEO_CALLBACK   rgb32_callback ;
        
    std::vector<GUI_OBJECT *> gui_pages ;
    std::mutex     gui_mutex ; 
    GUI_FONT       gui_font ;     
        
    std::shared_ptr<easymedia::Flow>  vpss_flow   ; 
     
    HI_VPSS_CHANNEL     channel[MAX_VPSS_CHANNEL] ;        
}HI_VPSS_PORT ;


typedef struct 
{
    HAL_DRIVER hal ;
    HI_VPSS_PORT port [MAX_VPSS_PORT] ;
}HI_VIDEO_VPSS ;

////做逐行处理
static void NEON_image_deinterlace( void * addr ,  int width , int height , int stride )
{
    int x , y , h ;
    U8 * line0 , * line1 , * line2 ;
    
    line0 = (U8*)addr ;
    line1 = line0 + stride ;
    line2 = line1 + stride ;
    h     = ( height >> 1 ) - 1 ;
    
    uint8x16_t   DXX ;
    DXX  = vdupq_n_u8( PIXEL_DIFF_LEVEL ) ; 
    
    for( y = 0 ; y < h ; y++ )
    {
        /*
        for( x = 0 ; x < width ; x++ )
        {
            int d0 , d1 ;
            d0 = line0[x] ;
            d1 = line2[x] ;
            d0 = ( d0 + d1 ) >> 1 ;
            d1 = abs( d0 - line1[x] ) ;
            if( d1 >= PIXEL_DIFF_LEVEL )
                line1[x] = (U8)d0 ;
        }
        */
        
        //Use ARM NEON 
        for( x = 0 ; x < width ; x += 16 ) //假设图像是按16像素对齐
        {
            uint8x16_t P0 , P1 , P2 , PABS ;  
            uint8x16_t MM ;  
            
            //fetch the pixel from each line
            P0  = vld1q_u8   ( line0 + x ) ;            
            P1  = vld1q_u8   ( line1 + x ) ;
            P2  = vld1q_u8   ( line2 + x ) ;
            
            //calc the avg data for line1           
            P0  = vshrq_n_u8 ( P0 , 1  ) ;   //line0 >> 1
            P2  = vshrq_n_u8 ( P2 , 1  ) ;   //line1 >> 1
            P0  = vaddq_u8   ( P0 , P2 ) ;   //( line0 + line2 ) >> 1 ;
            
            //calc the diff data between avg and line2
            PABS  = vabdq_u8 ( P0 , P1 ) ;                  
      
            //
            MM  = vcgeq_u8 ( PABS , DXX ) ; //great pixel will be set 0xFF 
            P0  = vandq_u8 ( MM , P0 ) ;    //keep the pixel masked with 0xFF
            
            MM  = vmvnq_u8 ( MM ) ;         //less pixel will be set 0xFF 
            P1  = vandq_u8 ( MM , P1 ) ;    //keep the pixel masked with 0xFF 
            
            //combine the data
            P1  = vorrq_u8( P1 , P0 ) ;
            
            //save the data
            vst1q_u8( line1 + x  , P1 ) ;   
        }
        
        line0 = line2 ;
        line1 = line0 + stride ;
        line2 = line1 + stride ;
    }
    
}

///Bright   = 0 ~ 256 => -128 ~ 128
///Contrast = 0 ~ 256 => gain = e^-2 ~ e^2
static void NEON_image_bright_contrast( void * addr ,  int width , int height , int stride , int bright , int contrast )
{
    float   gain ;
    int x , y  ;
    U8 * ptr , * dst ;
    

    //GAIN : e^-2 ~ e^2
    gain  = expf( ( contrast - 128.0f ) / 64.0f ) * 32 ; //Q5
    
    //y   = [x - 127.5 * (1 - B)] * C + 127.5 * (1 + B);
    //B/C = [-1,1]               
    
    int16x8_t  B1 , B2 , C , D000 , D255 ;  //128bit    
    B1   = vdupq_n_s16 ( (int16_t )((bright-384)>>1) ) ; //(B-384)/2
    B2   = vdupq_n_s16 ( (int16_t )(((bright+128)>>1)<<5) ) ; //(B+128)/2*32 => Q5
    C    = vdupq_n_s16 ( (int16_t )gain ) ; //Q5
    D000 = vdupq_n_s16 ( 0   ) ; 
    D255 = vdupq_n_s16 ( 255 ) ; 
    
    
    ptr = ( U8 * ) addr ;
    for( y = 0 ;y < height ; y++ )
    {
        /*
        int dat ;
        for( x = 0 ; x < width ; x++ )
        {
            dat = ptr[x] ;
            dat = dat + ((bright - 384) >> 1) ;
            dat = dat * contrast  ;
            dat = dat >> 7 ;
            dat = dat + ((128 + bright) >> 1) ;
            
            if( dat < 0 )
                dat = 0 ;
            else if( dat > 255 )
                dat = 255 ;
            ptr[x] = dat ;
        } 
        */
        
        //Use ARM NEON 
        {
            int16x8_t  P  ;  
            uint8x8_t  uu ;              
            
            dst = ptr ;            
            for( x = 0 ; x < width ; x += 8 ) //假设图像是按8像素对齐
            {
                uu  = vld1_u8  ( dst ) ;
                P   = (int16x8_t)vmovl_u8 ( uu  ) ;    //u8->int16                
                P   = vaddq_s16( P , B1) ;
                P   = vmulq_s16( P , C ) ;
                P   = vaddq_s16( P , B2) ;
                P   = vshrq_n_s16( P, 5) ;  // Q5 -> Q0
                P   = vmaxq_s16( P , D000 ) ;  //小于0的为0
                P   = vminq_s16( P , D255 ) ;  //大于255的为255
                uu  = vmovn_u16((uint16x8_t)P); 
                vst1_u8( dst , uu ) ;                
                dst = dst + 8 ;
            }
        }
                      
        ptr  += stride ;         
    }    
}

/////处理YUV图像: 亮度、对比度调节、DEINTERLACE
int RV1126_yuv_image_handler( void * addr , int width , int height , int bpp , int stride , void * para)
{
    HI_VPSS_PORT  * vpss = ( HI_VPSS_PORT *)para ;
    int ret = 0 ;
    
    //是否要做逐行处理
    if( vpss->deinterlace )
    {
        NEON_image_deinterlace( addr , width , height , stride ) ;
        ret++ ;
    }
    
    //是否要做图像调整
    if( vpss->bright != 128 || vpss->contrast != 128 ) 
    {
        NEON_image_bright_contrast( addr , width , height , stride , vpss->bright , vpss->contrast ) ;
        ret++ ;
    }
     
    return ret ;   
}

/////处理RGB32 VIDEO函数
int RV1126_rgb32_video_handler( void * video , void * para )
{
    RV1126_HAL_BUFFER * img = ( RV1126_HAL_BUFFER * ) video ;
    HI_VPSS_PORT  * vpss = ( HI_VPSS_PORT *)para ;
    VIDEO_MEM_INFO  info ;
    int i , len ;
    U8 * ptr ;
    
    if( vpss->rgb32_callback == NULL )
        return 0 ;
        
    info.w          = img->buffer->GetWidth()  ;
    info.h          = img->buffer->GetHeight() ;    
    info.bpp        = 4 ;
    ptr = (U8*)img->buffer->GetPtr() ;
    len = img->buffer->GetVirWidth() * info.h ;
    for( i = 0 ; i < 3 ; i++ )
    {
        info.phyAddr[i] = img->buffer->GetFD() ;
        info.virAddr[i] = ptr + len * i  ;
        info.stride[i]  = img->buffer->GetVirWidth() ; 
    }
    info.ref = img ;
    
    vpss->rgb32_callback ( &info , vpss->rgb32_callback_data ) ;   
    
    return 1 ;
}

/////处理GUI叠加函数
int RV1126_gui_object_handler( void * addr , int width , int height , int bpp , int stride , void * para)
{
    HI_VPSS_PORT  * vpss = ( HI_VPSS_PORT *)para ;
    GUI_SURFACE  surf ;
    GUI_CONTEXT  gui ;
    int i , count ;
    
    if( vpss->gui_pages.size() == 0 || vpss->gui_font == NULL )
        return 0 ;
    
    surf.bpp         = bpp    ;
    surf.width       = width  ;
    surf.height      = height ;
    surf.stride      = stride * bpp ;
    surf.accel       = NULL  ;
    surf.visible     = 0     ;
    surf.vir_addr    = addr  ;    
    surf.phy_addr    = 0     ;
    surf.clip.x      = 0     ; 
    surf.clip.y      = 0     ;
    surf.clip.width  = width ;
    surf.clip.height = width ;
    
    gui.surface = &surf ;
    gui.font    = vpss->gui_font ;

    ////显示GUI OBJECT到当前的Surface上//////
    vpss->gui_mutex.lock() ;
    count = vpss->gui_pages.size() ;
    for( i = 0 ; i < count ; i++ )
        draw_gui_object( &gui , vpss->gui_pages[i] , OP_DRAW_OBJ , NULL ) ;
    vpss->gui_mutex.unlock() ;
    
    return 1 ;
}

int RV1126_vpss_init ( HAL_DRIVER * hal , int port , void * lpara )
{
    VIDEO_VPSS_PARAM * vpss = (VIDEO_VPSS_PARAM *)lpara ;
    HI_VIDEO_VPSS    * drv  = (HI_VIDEO_VPSS *)hal ;
    
    if ( port > MAX_VPSS_PORT   )
        return 0;
    
    std::string  flow_param ;
    std::string  stream_param ;  
        
    PARAM_STRING_APPEND(flow_param, KEY_NAME, "vpss-filter"); 
    PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE , vpss->format == IMAGE_NV12_FMT ? IMAGE_NV12 : INTERNAL_PIX_FMT );//Set intput buffer type.    
    PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, INTERNAL_PIX_FMT);//Set output buffer type .
    
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_WIDTH , vpss->width );
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_HEIGHT, vpss->height);
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_WIDTH , vpss->width);
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_HEIGHT, vpss->height);
    
    
    //buffer pool
    PARAM_STRING_APPEND   (flow_param, KEY_MEM_TYPE, KEY_MEM_HARDWARE);
    PARAM_STRING_APPEND_TO(flow_param, KEY_MEM_CNT, 5);        
 
    //must have 
    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_ROTATE, 0);
    flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);
 
    auto vpss_flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>("filter" , flow_param.c_str());
    if (!vpss_flow) 
    {
        printf("Create vpss  main filter flow failed\n" );
        return 0 ;
    }
    
    //添加回调函数
    IMAGE_CALLBACK_INFO cb ;
    cb.argb = RV1126_gui_object_handler ;
    cb.yuyv = RV1126_yuv_image_handler  ;
    cb.video= RV1126_rgb32_video_handler;
    cb.para = &drv->port[port] ;
    vpss_flow->Control( OSD_OP_CALLBACK , (void*)&cb ) ; 
    
    drv->port[port].enable    = 1 ;    
    drv->port[port].width     = vpss->width  ;
    drv->port[port].height    = vpss->height ;  
    drv->port[port].bright    = 128 ;
    drv->port[port].contrast  = 128 ;
    drv->port[port].deinterlace = 0 ;
    drv->port[port].vpss_flow      = vpss_flow ;
    drv->port[port].gui_font       = NULL   ;
    
    drv->port[port].rgb32_callback_data   = -1 ;
    drv->port[port].rgb32_callback        = NULL ;    
    
    drv->port[port].gui_pages.clear()  ; 
    return 1 ;
}


int RV1126_vpss_create( HAL_DRIVER * hal , int port , int ch , int data , void * lpara)
{
    VIDEO_VPSS_PARAM * vpss = (VIDEO_VPSS_PARAM *)lpara ;
    HI_VIDEO_VPSS    * drv  = (HI_VIDEO_VPSS *)hal ;
        
    if ( port > MAX_VPSS_PORT  || ch > MAX_VPSS_CHANNEL )
    {
        printf("vpss group error or ch error!");
        return 0;
    }

    std::string  flow_param ;
    std::string  stream_param ;  
        
    PARAM_STRING_APPEND(flow_param, KEY_NAME, "image-filter"); 
    PARAM_STRING_APPEND(flow_param, KEY_INPUTDATATYPE , INTERNAL_PIX_FMT); //内部是ARGB
    //Set output buffer type.
    PARAM_STRING_APPEND(flow_param, KEY_OUTPUTDATATYPE, vpss->format == IMAGE_NV12_FMT ? IMAGE_NV12 : INTERNAL_PIX_FMT );        

    //Set output buffer size.
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_WIDTH , vpss->width );
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_HEIGHT, vpss->height);
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_WIDTH , vpss->width );
    PARAM_STRING_APPEND_TO(flow_param, KEY_BUFFER_VIR_HEIGHT, vpss->height);
    
    //buffer pool
    PARAM_STRING_APPEND   (flow_param, KEY_MEM_TYPE, KEY_MEM_HARDWARE);
    PARAM_STRING_APPEND_TO(flow_param, KEY_MEM_CNT, data ); //JPEG下非1就错误？？


    PARAM_STRING_APPEND_TO(stream_param, KEY_BUFFER_ROTATE, 0);
    flow_param = easymedia::JoinFlowParam(flow_param, 1, stream_param);
        
    auto flow = easymedia::REFLECTOR(Flow)::Create<easymedia::Flow>("filter" , flow_param.c_str());
    if (!flow ) 
    {
        printf("Create vpss filter flow failed\n" );
        return 0 ;
    }

    
    drv->port[port].channel[ch].width        = vpss->width  ;
    drv->port[port].channel[ch].height       = vpss->height ;
    drv->port[port].channel[ch].channel_flow = flow ;
    
    //add to main link 
    drv->port[port].vpss_flow->AddDownFlow( flow , 0 , 0 ) ;
    return 1 ;
}

int RV1126_vpss_start( HAL_DRIVER * hal , int port , int ch , int flag )
{
    HI_VIDEO_VPSS    * drv  = (HI_VIDEO_VPSS *)hal ;
       
    if ( port >= MAX_VPSS_PORT || ch >= MAX_VPSS_CHANNEL )
    {
        printf( "port(%d) error or ch(%d) error", port, ch );
        return 0;
    }  
    
    drv->port[port].enable = flag ;
    drv->port[port].vpss_flow->Control( flag ? OSD_OP_START : OSD_OP_HALT , NULL ) ;            
    return 1 ;    
}


int RV1126_vpss_bind ( HAL_DRIVER * hal , int port , int ch , int flag , void * obj  )
{
    HAL_OBJECT       * src  = (HAL_OBJECT *)obj ;
    HI_VIDEO_VPSS    * drv  = (HI_VIDEO_VPSS *)hal ;
    RV1126_HAL_PRIV    priv ;
    
    if ( port > MAX_VPSS_PORT )
    {
        printf("vpss group error !");
        return 0;
    }
    
    priv.flow = nullptr ;     
    src->drv->ioctl( src->drv , src->port , src->channel , OP_GET_PRIVATE , 0 , &priv ) ;     
    if( priv.flow != nullptr )
    {   
        if( flag )
            priv.flow->AddDownFlow( drv->port[port].vpss_flow , 0 , 0 ) ;
        else
            priv.flow->RemoveDownFlow( drv->port[port].vpss_flow ) ;
    }
    return 1 ;    
}


int RV1126_vpss_send_frame( HAL_DRIVER * hal , int port , AV_STREAM_FRAME * av , U32 idx )
{
    HI_VIDEO_VPSS * drv  = (HI_VIDEO_VPSS *)hal ;
    
    if ( port > MAX_VPSS_PORT )
    {
        printf("vpss group error error!");
        return 0;
    }  
    
    RV1126_HAL_BUFFER * buf = ( RV1126_HAL_BUFFER * )av->ref ;
    if( buf->valid )
    {
        //PixelFormat  fmt = argb ? PIX_FMT_ARGB8888 : PIX_FMT_NV12 ;
        
        std::shared_ptr<easymedia::MediaBuffer> mb = buf->buffer ;        
        drv->port[port].vpss_flow->SendInput( mb , 0 ) ;
    }  
    return 1 ;
}

int RV1126_vpss_get_private ( HAL_DRIVER * hal , int port , int ch , void * lpara )
{
    RV1126_HAL_PRIV * priv = ( RV1126_HAL_PRIV * )lpara ;
    HI_VIDEO_VPSS * drv  = (HI_VIDEO_VPSS *)hal ;
    
    if ( port > MAX_VPSS_PORT  || ch > MAX_VPSS_CHANNEL )
    {
        printf("vpss group error or ch error!");
        return 0;
    }
    
    if( ch < 0 )    
        priv->flow = drv->port[port].vpss_flow;
    else
        priv->flow = drv->port[port].channel[ch].channel_flow ;
    
    return 1 ;    
}

int RV1126_vpss_config_channel( HAL_DRIVER * hal , int port , int ch , int pattern )
{    
    HI_VIDEO_VPSS * drv  = (HI_VIDEO_VPSS *)hal ;
    
    if ( port > MAX_VPSS_PORT  || ch > MAX_VPSS_CHANNEL )
    {
        printf("vpss group error or ch error!");
        return 0;
    }
    
    drv->port[port].channel[ch].channel_flow->Control( VO_OP_SET_PATTERN , &pattern );
    return 1 ;
}

int RV1126_vpss_add_del_page( HAL_DRIVER * hal , int port , int op , void * page )
{
    HI_VIDEO_VPSS *drv  = (HI_VIDEO_VPSS *)hal ;
    HI_VPSS_PORT * vpss ;
    int i , count ;
    
    vpss  = &drv->port[port] ;
    
    vpss->gui_mutex.lock() ;
  
    if( op == OP_ADD_GUI_PAGE )
    {
        //add new page
        vpss->gui_pages.push_back( (GUI_OBJECT *)page ) ;
        vpss->gui_mutex.unlock() ; 
        return 1 ;
    }
    
    //here is to delete one page from list
    count = vpss->gui_pages.size() ;
    for( i = 0 ; i < count ; i++ )
    {
        if( vpss->gui_pages[i] == page )
        {
            vpss->gui_pages.erase( vpss->gui_pages.begin() + i ) ;
            break ;
        }
    }   
    
    vpss->gui_mutex.unlock() ;       
    return 1 ;     
}

int RV1126_vpss_set_zoom( HAL_DRIVER * hal , int port , void * para )
{    
    HI_VIDEO_VPSS   *  drv  = (HI_VIDEO_VPSS *)hal ;
    LIVE_VIDEO_ZOOM * zoom ;
    IMAGE_ZOOM_INFO   info ;
    
    
    if ( port > MAX_VPSS_PORT  )
    {
        printf("vpss group error or ch error!");
        return 0;
    }
    zoom = (LIVE_VIDEO_ZOOM*)para ;
    
    info.x = (zoom->x + 3 )/4*4;
    info.y = (zoom->y + 3 )/4*4;
    info.width  = (zoom->width  + 3 )/4*4;
    info.height = (zoom->height + 3 )/4*4;
    drv->port[port].vpss_flow->Control( IMG_OP_ZOOM , &info );
    return 1 ;
}

int RV1126_vpss_ioctl( HAL_DRIVER * hal , int port , int ch , int op   , int idx , void * lpara )
{
    HI_VIDEO_VPSS *drv = (HI_VIDEO_VPSS *)hal ;
    VIDEO_OVERLAY_CANVAS *canvas ;    
    VIDEO_OVERLAY_CFG *overlay ; 
    
        
    OSD_PLANE_INFO plane ;
    int ret = 0 ;
    
    
    switch ( op )
    {
    case OP_SEND_FRAME :
        ret = RV1126_vpss_send_frame( hal , port , (AV_STREAM_FRAME *)lpara , (U32)idx ) ;        
        break ;

    case OP_ZOOM_PARA :
        RV1126_vpss_set_zoom( hal , port , lpara ) ;
        break ;
        
    case OP_GET_PRIVATE : //返回指定channel的flow
        ret = RV1126_vpss_get_private( hal , port , ch , lpara ) ;
        break ;
        
    case OP_CONFIG_OVERLAY :
    case OP_CREATE_OVERLAY :
        
        overlay    = ( VIDEO_OVERLAY_CFG * ) lpara ;
        overlay->x = ( overlay->x + 3 )/4*4 ;
        overlay->y = ( overlay->y + 3 )/4*4 ;
        overlay->width  = ( overlay->width  + 3 )/4*4 ;
        overlay->height = ( overlay->height + 3 )/4*4 ;        
        if ( overlay->width > drv->port[port].width )
            overlay->width = drv->port[port].width ;
        if ( overlay->height > drv->port[port].height )
            overlay->height = drv->port[port].height ;
        if( overlay->width && overlay->height && idx < MAX_VPSS_OVERLAY )
        {    
            plane.id     =  idx ;
            plane.x      =  overlay->x ;
            plane.y      =  overlay->y ;
            plane.width  =  overlay->width ;
            plane.height =  overlay->height;
            plane.alpha  =  overlay->alpha ;
            plane.enable =  1    ;
            plane.data   =  NULL ;
            
            ret = drv->port[port].vpss_flow->Control( OSD_OP_CREATE , &plane ) ;                                            
        }        
        break ;        
        

    case OP_ENABLE_OVERLAY :
        if ( idx < MAX_VPSS_OVERLAY )
        {
            plane.id     =  idx ;   
            plane.enable =  *((int*)lpara) ;  
            plane.data   =  NULL ;             
            ret = drv->port[port].vpss_flow->Control( OSD_OP_UPDATE , &plane ) ; 
        }
        break ;
        
    case OP_DESTROY_OVERLAY :
        if ( idx < MAX_VPSS_OVERLAY )
        {
            plane.id     =  idx ;                        
            ret = drv->port[port].vpss_flow->Control( OSD_OP_REMOVE , &plane ) ; 
        }
        break ;
        
    case OP_UPDATE_BITMAP  :  
        if ( idx < MAX_VPSS_OVERLAY )
        {
            VIDEO_OSD_BITMAP * bitmap = (VIDEO_OSD_BITMAP *)lpara ;
            
            plane.id     =  idx ;   
            plane.enable =  -1  ;   
            plane.width  =  bitmap->width ;
            plane.height =  bitmap->height;  
            plane.data   =  (int*)bitmap->data ;         
            ret = drv->port[port].vpss_flow->Control( OSD_OP_UPDATE , &plane ) ; 
        }              
        break ;
        
        
    case OP_GET_CANVAS_INFO :
        if( idx < MAX_VPSS_OVERLAY  )
        {
            plane.id     =  idx ;                 
            ret = drv->port[port].vpss_flow->Control( OSD_OP_GETPTR , &plane ) ;   
                    
            canvas = ( VIDEO_OVERLAY_CANVAS * ) lpara ;
            canvas->w              = plane.width  ;
            canvas->h              = plane.height ;
            canvas->phy_addr       = 0            ;
            canvas->vir_addr       = (int)plane.data   ;
            canvas->stride         = plane.width * OSD_PLANE_BPP ;                           
        }
        break ;    
        
    case OP_CONFIG_CHANNEL :  //配置通道的DropPattern
        ret = RV1126_vpss_config_channel( hal , port , ch , idx );     
        break ;

    case OP_DEL_GUI_PAGE :
    case OP_ADD_GUI_PAGE :
        ret = RV1126_vpss_add_del_page( hal , port , op , lpara ) ;
        break ;

    case OP_SET_GUI_FONT :        
        drv->port[port].gui_font  = lpara   ; 
        break ;
    
    case OP_LOCK_GUI_OBJ :
        drv->port[port].gui_mutex.lock()   ;
        break ;
    
    case OP_UNLOCK_GUI_OBJ :
        drv->port[port].gui_mutex.unlock() ;
        break ;
        
    case OP_CSC_BRIGHT :
        drv->port[port].bright = idx ;
        break ;
        
    case OP_CSC_CONTRAST :
        drv->port[port].contrast = idx ; 
        break ;

    case OP_DEINTERLACE :
        drv->port[port].deinterlace = idx ; 
        break ;

    case OP_REG_CALLBACK :
        drv->port[port].rgb32_callback_data = idx ;
        drv->port[port].rgb32_callback = (RGB32_VIDEO_CALLBACK)lpara ;          
        break ;

    default :
        break ;
    }
    return ret ;
}

int RV1126_vpss_exit( HAL_DRIVER * hal , int port )
{
    ////rkmedia资源不释放/////
    
    return 1;
}

static HI_VIDEO_VPSS   HI_video_vpss = {
    {
    "VPSS"  , "RV1126 VPSS" , 1 ,
    RV1126_vpss_init ,
    RV1126_vpss_exit ,
    RV1126_vpss_create ,
    NULL             ,
    RV1126_vpss_ioctl  ,
    RV1126_vpss_bind   ,
    RV1126_vpss_start  ,
    NULL             ,
    }
};

void RV1126_unregister_vpss( void )
{
    unregister_driver("VPSS");
}

void RV1126_register_vpss( void )
{
    register_driver( (HAL_DRIVER *)&HI_video_vpss , -1 , NULL ) ;
}

