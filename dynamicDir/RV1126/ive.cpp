
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <arm_neon.h>


//rkmedia
#include "flow.h"

#include "hal-private.h"

//#include "sys-service.h"
//#include "sys-core.h"
#include "sys-hal.h"
//#include "sys-list.h"


#define MAX_IVE_PORT_NUM      1 


#define MAX_IVE_BUF_WIDTH      1920
#define MAX_IVE_BUF_HEIGHT     1080



typedef struct 
{
    RV1126_HAL_BUFFER  imgU32   [MAX_IVE_U32_NUM   ] ; //12
    RV1126_HAL_BUFFER  imgRandom[MAX_IVE_RANDOM_NUM] ; //8 -- 实际使用时再创建
    int                randomUsed ;

    pthread_mutex_t    mutex    ;   //互锁信号量    
}HI_IVE_DATA;

typedef struct 
{
    HAL_DRIVER    hal     ;   
    
    HI_IVE_DATA   ive     ;
}HI_DRIVER_IVE;


#define RV1126_IVE_ALIGN(a)  (((a)+15)/16*16)


static int RV1126_ive_image_to_meminfo ( RV1126_HAL_BUFFER *img , VIDEO_MEM_INFO *memInfo )
{
    int i , len ;
    U8 * ptr ;
    
    memInfo->w          = img->buffer->GetWidth()  ;
    memInfo->h          = img->buffer->GetHeight() ;    
    memInfo->bpp        = img->buffer->GetPixelFormat() == PIX_FMT_RGB888 ? 3 : 4 ;
    
    ptr = (U8*)img->buffer->GetPtr() ;
    len = img->buffer->GetVirWidth() * memInfo->h ;
    for( i = 0 ; i < 3 ; i++ )
    {
        memInfo->phyAddr[i] = img->buffer->GetFD() ;
        memInfo->virAddr[i] = ptr + len * i  ;
        memInfo->stride[i]  = img->buffer->GetVirWidth() ; 
    }
    memInfo->ref = img ;
    return 1 ;
}



static int RV1126_ive_destroy_image( RV1126_HAL_BUFFER* img )
{
    if( img->valid ==  0 )
        return 0 ;
               
    img->buffer.reset() ;
    img->valid = 0 ;
    return 1 ;
}

//support ARGB32 and RGB24 
static int RV1126_ive_create_image( RV1126_HAL_BUFFER * pstImg , int bpp , int width , int height )
{    
    ImageInfo info ;
    info.width      = width  ;
    info.height     = height ;
    info.vir_width  = RV1126_IVE_ALIGN(width) ; 
    info.vir_height = RV1126_IVE_ALIGN(height); 
    info.pix_fmt    = bpp == 4 ? PIX_FMT_ARGB8888:PIX_FMT_RGB888;    
    auto &&mb = easymedia::MediaBuffer::Alloc2( info.vir_width * info.vir_height * bpp , easymedia::MediaBuffer::MemType::MEM_HARD_WARE ) ;
    if (mb.GetSize() == 0)
        return -1 ;
    
    pstImg->buffer = std::make_shared<easymedia::ImageBuffer>(mb , info);
    pstImg->valid  = 1 ;
    
    return  0 ;
}


static int RV1126_ive_destroy_all( HI_IVE_DATA *iveData )
{
    int i ;
    
    for ( i=0 ; i<MAX_IVE_U32_NUM ; i++ )
        RV1126_ive_destroy_image( &iveData->imgU32[i] );
    
    for ( i=0 ; i<MAX_IVE_RANDOM_NUM ; i++ )
        RV1126_ive_destroy_image( &iveData->imgRandom[i] );
    
    return 1 ;
}

static PixelFormat RV1126_ive_get_rect_and_fmt( VIDEO_MEM_INFO * mem , ImageRect * rect )
{
    RV1126_HAL_BUFFER * pImg = (RV1126_HAL_BUFFER * )mem->ref ;
    int  stride , offset ;
    
    stride    = pImg->buffer->GetVirWidth() ;
    offset    = mem->virAddr[0] - (U8*)pImg->buffer->GetPtr() ; //获取偏移量

    //按像素进行计算
    rect->x   = offset % stride ;
    rect->y   = offset / stride ;
    rect->w   = mem->w ;
    rect->h   = mem->h ; 
    
    if( mem->bpp == 4 )
        return PIX_FMT_ARGB8888 ;
    else if( mem->bpp == 3 )
        return PIX_FMT_RGB888 ;
    else if( mem->bpp == 2 )
        return PIX_FMT_YUYV422 ;
    else if( mem->bpp == -3 ) //负数只可用于格式转换时
        return PIX_FMT_BGR888 ;
        
    return PIX_FMT_NV12 ;  //YUV420SP
}



int RV1126_ive_init( HAL_DRIVER * hal , int port , void * lpara )     
{
    VIDEO_IVE_PARA *ive          = (VIDEO_IVE_PARA *)lpara ;
    HI_DRIVER_IVE  *drv          = (HI_DRIVER_IVE *)hal ;
    
    int i , ret ;

    memset( &drv->ive , 0 , sizeof(HI_IVE_DATA ) );
    
    ret = 0 ;
    for ( i=0 ; i< MAX_IVE_U32_NUM ; i++ )
        ret += RV1126_ive_create_image( &drv->ive.imgU32[i], 4 , ive->width, ive->height );             

    if ( ret )
    {
        printf("IVE create image errorr \r\n");
        RV1126_ive_destroy_all( &drv->ive );
        return 0 ;
    }
    
    pthread_mutex_init( &drv->ive.mutex  , NULL ); 

    return 1 ;
}


int RV1126_ive_mem_fill ( VIDEO_MEM_INFO *dst , U32 color )
{
    U32 * ptr ;
    int x , y ;
    
    uint32x4_t  data ;
    
    data = vmovq_n_u32( color ) ; 
    ptr  = (U32*)dst->virAddr[0] ; 
    for( y = 0 ; y < dst->h ; y++ )
    {
        for( x = 0 ; x < dst->w ;  x += 4 ) 
        {
            vst1q_u32( ptr + x , data ) ;
        }
        
        ptr += dst->stride[0] ;
    }
    
    return 1 ;
}

//blit具有1. 格式转换  2. 缩放   3. blend  4. COPY
int RV1126_ive_blit    ( VIDEO_MEM_INFO *src ,  VIDEO_MEM_INFO *dst , int alpha )
{
    RV1126_HAL_BUFFER * pSrcImg = (RV1126_HAL_BUFFER * )src->ref ;
    RV1126_HAL_BUFFER * pDstImg = (RV1126_HAL_BUFFER * )dst->ref ;
    ImageRect src_rect , dst_rect ;
    int blend = 0 ;
    
    ImageInfo & src_info = pSrcImg->buffer->GetImageInfo() ;    
    src_info.pix_fmt = RV1126_ive_get_rect_and_fmt( src , &src_rect ) ;
    
    ImageInfo & dst_info = pDstImg->buffer->GetImageInfo() ;
    dst_info.pix_fmt = RV1126_ive_get_rect_and_fmt( dst , &dst_rect ) ;
    
    if( alpha > 0 )
        blend = ( alpha << 16 ) | 0x0105 ; //全局alpha和像素alpha叠加 src over
       
    return VpssRgaBlit( pSrcImg->buffer , pDstImg->buffer , &src_rect , &dst_rect , 0 , blend ) ;
}

int RV1126_ive_lut( VIDEO_MEM_INFO * gray ,  VIDEO_MEM_INFO * rgb32 , VIDEO_MEM_INFO * colorbar )
{
    //TODO : 

    return 1 ;
}

//把Y分量变化到ARGB的alpha通道去
int RV1126_ive_add_alpha( VIDEO_MEM_INFO * rgb32 ,  VIDEO_MEM_INFO * alpha )
{
    U8  * pA , * pARGB ;        
    int x ,y ;
    
    uint8x8x4_t  data ;
    
    pARGB = (U8*)rgb32->virAddr[0] ;
    pA    = (U8*)alpha->virAddr[0] ;
    for( y = 0 ; y < alpha->h ; y++ )
    {
        for( x = 0 ; x < alpha->w ; x += 8 ) 
        {
            //load it
            data        = vld4_u8( pARGB + x * 4 ) ;            
            data.val[3] = vld1_u8( pA    + x ) ; 
            
            //save it           
            vst4_u8( pARGB + x * 4 , data );
            //vst4_lane_u8( pARGB + x * 4 , data , 3 );
        }
        
        pARGB += rgb32->stride[0] * 4 ;
        pA    += alpha->stride[0] ;
    }

    return 1 ;
}

//YUV进行叠加，使用src的Y通道当作alpha
int RV1126_ive_yuv_blend( VIDEO_MEM_INFO *src ,  VIDEO_MEM_INFO *dst , int alpha )
{    
    U8 *srcY , *srcUV , *dstY , *dstUV  ;
    int x , y , width ;
    

    srcY   = src->virAddr[0] ;    
    srcUV  = src->virAddr[1] ;
    if( *srcUV == 0 ) 
        srcUV  += src->stride[0] * 8 ; //RV1126 RGA SCALE后需要特殊处理
    
    dstY   = dst->virAddr[0] ;        
    dstUV  = dst->virAddr[1] ; 
    if( *dstUV == 0 ) 
        dstUV  += dst->stride[0] * 8 ; //RV1126 RGA SCALE后需要特殊处理    
    
    ////
    width = src->w / 8 * 8 ;  //对齐到8像素    
    for( y = 0 ;  y < src->h ; y++ )
    {
        
        for( x = 0 ; x < width ; x += 8 ) //一次8个像素
        {
            uint8x8_t   ss , dd , aa ;
            uint16x8_t  ssaa , ddaa ;
            uint16x8_t  ssss , dddd ;
            uint16x8_t  t0 , t1 ;
            
            //dstY[x]  = ( dstY[x]  * ( 255 - alpha ) + srcY[x]  * alpha ) >> 8 ;
            ss = vld1_u8( srcY + x) ;
            dd = vld1_u8( dstY + x) ;            
            aa = alpha == 255 ? ss : vdup_n_u8( alpha )  ;
            
            ssaa = vmovl_u8( aa ) ;              //sa           
            ddaa = vmovl_u8( vmvn_u8  ( aa ) ) ; //da =  1 - sa
            
            ssss = vmovl_u8( ss ) ;
            dddd = vmovl_u8( dd ) ;
            
            t0 = vmulq_u16( ssaa , ssss ) ;
            t1 = vmulq_u16( ddaa , dddd ) ;
            t0 = vaddq_u16( t0 , t1 ) ;
            t1 = vshrq_n_u16( t0 , 8) ;
            
            dd = vmovn_u16(t1);  //
            vst1_u8 ( dstY + x, dd ) ;
            
            
            //dstUV[x] = ( dstUV[x] * ( 255 - alpha ) + srcUV[x] * alpha ) >> 8 ;  
            ss = vld1_u8( srcUV + x) ;
            dd = vld1_u8( dstUV + x) ;
            ssss = vmovl_u8( ss ) ;
            dddd = vmovl_u8( dd ) ;                   
                        
            t0 = vmulq_u16( ssaa , ssss ) ;
            t1 = vmulq_u16( ddaa , dddd ) ;
            t0 = vaddq_u16( t0 , t1 ) ;
            t1 = vshrq_n_u16( t0 , 8) ;
            
            dd = vmovn_u16(t1);  //
            vst1_u8 ( dstUV + x, dd ) ;
        }
        
        
        if( width != src->w ) //最后一起计算一次，分开存储
        {
            uint8x8_t   ss , dd , aa ;
            uint16x8_t  ssaa , ddaa ;
            uint16x8_t  ssss , dddd ;
            uint16x8_t  t0 , t1 ;
            
            U8  yy[8] , uv[8] ;
            
            //dstY[x]  = ( dstY[x]  * ( 255 - alpha ) + srcY[x]  * alpha ) >> 8 ;
            ss = vld1_u8( srcY + width) ;
            dd = vld1_u8( dstY + width) ;            
            aa = alpha == 255 ? ss : vdup_n_u8( alpha )  ;
            
            ssaa = vmovl_u8( aa ) ;              //sa           
            ddaa = vmovl_u8( vmvn_u8  ( aa ) ) ; //da =  1 - sa
            
            ssss = vmovl_u8( ss ) ;
            dddd = vmovl_u8( dd ) ;
            
            t0 = vmulq_u16( ssaa , ssss ) ;
            t1 = vmulq_u16( ddaa , dddd ) ;
            t0 = vaddq_u16( t0 , t1 ) ;
            t1 = vshrq_n_u16( t0 , 8) ;
            
            dd = vmovn_u16(t1);  //            
            vst1_u8 ( yy , dd ) ;
            
            //dstUV[x] = ( dstUV[x] * ( 255 - alpha ) + srcUV[x] * alpha ) >> 8 ;  
            ss = vld1_u8( srcUV + x) ;
            dd = vld1_u8( dstUV + x) ;
            ssss = vmovl_u8( ss ) ;
            dddd = vmovl_u8( dd ) ;                   
                        
            t0 = vmulq_u16( ssaa , ssss ) ;
            t1 = vmulq_u16( ddaa , dddd ) ;
            t0 = vaddq_u16( t0 , t1 ) ;
            t1 = vshrq_n_u16( t0 , 8) ;
            
            dd = vmovn_u16(t1);  //  
            vst1_u8 ( uv , dd ) ;          
            
            memcpy( dstY  + width , yy , src->w - width ) ;
            memcpy( dstUV + width , uv , src->w - width ) ;
        }
    
        srcY  += src->stride[0] ;
        dstY  += dst->stride[0] ;
        
        if( ( y % 2 ) ==  1 )
        {
            srcUV += src->stride[0] ;
            dstUV += dst->stride[0] ;
        }
    }
    
   
    return 1 ;
}


int RV1126_ive_ioctl( HAL_DRIVER * hal , int port , int ch , int op   , int data , void * lpara )
{
    HI_DRIVER_IVE  * drv      = (HI_DRIVER_IVE *)hal ;
    HI_IVE_DATA    * iveData  = &drv->ive ;  
    

    VIDEO_MEM_INFO    * memInfo = ( VIDEO_MEM_INFO *)lpara ;
    RV1126_HAL_BUFFER * img ;
    int i , ret ;
    
    ret = 0 ;

    switch(op)
    {
        case OP_IVE_MEM_COPY :
            ret = RV1126_ive_blit( memInfo , memInfo + 1 , 0 );
            break ;

        case OP_IVE_CTRL_OUTLINE :
            break ;
        
        case OP_IVE_ADD_ALPHA :
            ret = RV1126_ive_add_alpha( memInfo , memInfo + 1  );
            break ;
            
        case OP_IVE_YUV_BLEND :
            ret = RV1126_ive_yuv_blend( memInfo , memInfo + 1 ,  data );
            break ;
        
        case OP_IVE_LUT :
            ret = RV1126_ive_lut( memInfo , memInfo + 1 ,  memInfo + 2 );
            break ;
                    
        case OP_IVE_CTRL_BLIT :
            ret = RV1126_ive_blit( memInfo , memInfo + 1 ,  data );
            break ;
        
        case OP_IVE_MEM_FILL :
            ret = RV1126_ive_mem_fill( memInfo , (U32)data );
            break ;    
                    
        case OP_IVE_U32_BUF :
            img  = &iveData->imgU32[0] ;
            for( i=0; i<MAX_IVE_U32_NUM; i++ )
                RV1126_ive_image_to_meminfo( img++, memInfo++ ) ;
            ret =  1;
            break ;            
            
            
        case OP_IVE_GET_ONE_BUF :            
            if( iveData->randomUsed < MAX_IVE_RANDOM_NUM ) 
            {
                img  = &iveData->imgRandom[iveData->randomUsed] ;
                if ( RV1126_ive_create_image( img , memInfo->bpp , memInfo->w , memInfo->h ) == 0 )
                {
                    RV1126_ive_image_to_meminfo( img, memInfo ) ; 
                    iveData->randomUsed++ ;
                    ret = 1 ;
                }
            }   
            break ;


        default :
            break ;
    }
        
    return ret ;    
        
}

//模块退出函数
int  RV1126_ive_exit( HAL_DRIVER * hal , int port )
{
    return 1 ;
}

HI_DRIVER_IVE  HI_video_ive = {    
    {
    "VideoIVE"  , "RV1126 IVE" , 1 , 
    RV1126_ive_init ,
    RV1126_ive_exit ,
    NULL ,
    NULL             ,
    RV1126_ive_ioctl ,
    NULL             ,
    NULL             ,
    NULL             ,        
    } ,
};




void RV1126_unregister_ive( void )
{    
    unregister_driver("VideoIVE");    
}

void RV1126_register_ive( void )
{
    register_driver( (HAL_DRIVER *)&HI_video_ive  , -1 , NULL ) ;    
}

