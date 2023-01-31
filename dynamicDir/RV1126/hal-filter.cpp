#include "flow.h"

#include "hal-private.h"
#include "hal-filter.h"

#include <arm_neon.h>

namespace easymedia {


VpssFilter::VpssFilter(const char *param) 
{    
    enableFlag     = 1 ;
    
    imageCallback.argb = NULL ;
    imageCallback.yuyv = NULL ;
    imageCallback.video= NULL ;
    imageCallback.para = NULL ;
    
    sourceRect.x   = 0 ;
    sourceRect.y   = 0 ;
    sourceRect.w   = 0 ;
    sourceRect.h   = 0 ;
}
  
  
int  VpssFilter::FindOsd( int id )
{
    int i , count ;
    int ret = -1  ;
    
    count = osdPlanes.size() ;
    for( i = 0 ; i < count ; i++ )
    {
        if( osdPlanes[i].info.id == id )
        {
            ret = i ;
            break ;
        }
    }
    
    return ret  ; 
}    

int  VpssFilter::CreateOsd ( OSD_PLANE_INFO * osd )
{
    OSD_PLANE  plane  ;
    ImageInfo info ;
    int index ;

    index = FindOsd( osd->id ) ;
    if( index >= 0 )
    {
        OSD_PLANE * old = &osdPlanes[index] ;

        old->info.x      =  osd->x      ;
        old->info.y      =  osd->y      ;
        old->info.enable =  osd->enable ;
        old->info.alpha  =  osd->alpha  ;
        return 1 ;
    }

    memcpy( &plane.info , osd , sizeof(OSD_PLANE_INFO) ) ;
    
    info.pix_fmt = PIX_FMT_ARGB8888 ; //总是ARGB格式
    info.width   = (osd->width +3)&(~3);  //4字节对齐
    info.height  = (osd->height+3)&(~3);  //4字节对齐
    info.vir_width = info.width ;  //4字节对齐
    info.vir_height= info.height;  //4字节对齐
    
    //update align the size
    plane.info.width = info.width ;
    plane.info.height= info.height;

    size_t size = CalPixFmtSize(info);
    auto &&mb   = MediaBuffer::Alloc2(size, MediaBuffer::MemType::MEM_HARD_WARE);
    if ( mb.GetSize() == 0)
    {
        printf("%s : Can't alloc osd buffer [%d] = %dx%d .\n", __FUNCTION__ , osd->id , osd->width , osd->height );
        return 0 ;
    }
    
    plane.info.data = (int*)mb.GetPtr() ;
    plane.buffer  = std::make_shared<ImageBuffer>(mb, info);
    VpssRgbFill( plane.buffer , NULL  ) ;  //clear to zero 

    osdPlanes.push_back( plane ) ;

    osd->data = plane.info.data ;
    return 1 ;
}

int  VpssFilter::GetOsdPtr ( OSD_PLANE_INFO * osd )
{
    OSD_PLANE * plane  ;
    int index ;
    
    index = FindOsd( osd->id );
    if( index < 0  )
    {
        printf("%s : Osd = %d  is not created .\n", __FUNCTION__ , osd->id );
        return 0 ;        
    }       
    plane = &osdPlanes[index]    ;
    memcpy( osd , &plane->info , sizeof(OSD_PLANE_INFO) ); 
    
    return 1 ;
}    

int  VpssFilter::UpdateOsd ( OSD_PLANE_INFO * osd )
{
    OSD_PLANE * plane  ;
    int index ;
    
    index = FindOsd( osd->id );
    if( index < 0  )
    {
        printf("%s : Osd = %d  is not created .\n", __FUNCTION__ , osd->id );
        return 0 ;        
    }       
        
    plane = &osdPlanes[index] ;    
    if( osd->enable >= 0 ) //更新enable
        plane->info.enable =  osd->enable ;
    
    if( osd->data ) //更新内容
    {
        int y , * src , * dst ;
    
        src = (int*)osd->data   ;
        dst = (int*)plane->info.data ;
        for( y = 0 ; y < osd->height ; y++)
        {
            memcpy( dst , src , osd->width * OSD_PLANE_BPP ) ;
            src  +=  osd->width ;
            dst  +=  plane->info.width ;
        }   
    }    
    
    return 1 ;
}

int  VpssFilter::DestroyOsd( int id )
{
    int index ;    
    index = FindOsd( id );
    if( index < 0 )
    {
        printf("%s : Osd = %d  is not created .\n", __FUNCTION__ , id );
        return 0 ;        
    }         
    osdPlanes.erase( osdPlanes.begin() + index ) ;
    return 1 ;
}    


int  VpssFilter::Process(  std::shared_ptr<MediaBuffer> input, std::shared_ptr<MediaBuffer> &output) 
{
    ImageRect  src_rect , dst_rect ;
    int i , count , ret , max_w , max_h ;
    int bypass ;
    
    if (!input || input->GetType() != Type::Image)
        return -EINVAL;
    if (!output || output->GetType() != Type::Image)
        return -EINVAL;
    
    if( !enableFlag ) //drop frame
        return 1 ;
        
  

    //转换成ImageBuffer的指shared针
    auto src = std::static_pointer_cast<easymedia::ImageBuffer>(input);
    auto dst = std::static_pointer_cast<easymedia::ImageBuffer>(output);
    
    max_w = dst->GetWidth() ;
    max_h = dst->GetHeight();
   
    bypass = 0 ;
    if( src->GetPixelFormat() == dst->GetPixelFormat() 
        && src->GetWidth()  == dst->GetWidth()
        && src->GetHeight() == dst->GetHeight() )
    {
        output = input ;  //zeor copy 
        dst    = src   ;
        bypass = 1 ;
    }
    
    dataMutex.lock() ;
    
    //STEP 0 :  图像处理
    PixelFormat fmt = src->GetPixelFormat() ;
    if( imageCallback.yuyv && (fmt == PIX_FMT_NV12 || fmt == PIX_FMT_NV16) )
    {
        ret = imageCallback.yuyv( src->GetPtr() , src->GetWidth() , src->GetHeight() , 1 , src->GetVirWidth() , imageCallback.para ) ; //bpp = 1
        //Media Buffer cache同步，CPU写Buffer后会有部分数据留在缓存中没没能及时同步到内存(DDR)中，
        //此时使用该接口就是保障CPU操作Buffer之后，缓存被立即刷新到内存。
        if( ret )
        {
            src->BeginCPUAccess(true);
            src->EndCPUAccess(true);
        }
    }
    
    //STEP 1 :  COPY the whole image to dst , convert to ARGB    
    if( !bypass )
    {    
        if( sourceRect.w == 0 || sourceRect.h == 0 )
            ret = VpssRgaBlit( src, dst ) ;
        else
            ret = VpssRgaBlit( src, dst , &sourceRect ) ; //zoom
    }
    
    //STEP 2 : ARGB video callback
    if( imageCallback.video )
    {
        RV1126_HAL_BUFFER  buf ;
        buf.valid  = 1   ;
        buf.buffer = dst ;
        imageCallback.video( &buf , imageCallback.para ) ; //
    }
        
    //STEP 3 :  Blend OSD plane to dst , all are ARGB     
    count = osdPlanes.size() ;
    for( i = 0 ; i < count ; i++ )
    {
        if( osdPlanes[i].info.enable == 0 )
            continue ;
            
        dst_rect.x  = osdPlanes[i].info.x ;
        dst_rect.y  = osdPlanes[i].info.y ;
        dst_rect.w  = osdPlanes[i].info.width  ;
        dst_rect.h  = osdPlanes[i].info.height ;
        
        if( dst_rect.x + dst_rect.w > max_w )
            dst_rect.w = max_w - dst_rect.x ;
        if( dst_rect.y + dst_rect.h > max_h )
            dst_rect.h = max_h - dst_rect.y ;
        
        src_rect.x  = 0 ;
        src_rect.y  = 0 ;
        src_rect.w  = dst_rect.w  ;
        src_rect.h  = dst_rect.h  ;        
        
        //OSD 总是采用 alpha叠加操作 
        ret = VpssRgaBlit( osdPlanes[i].buffer , dst , &src_rect , &dst_rect , 0 , 0xFF0105 ) ; //src over
    }
            
    //STEP 4 : DO GUI OBJECT HANDLER
    if( imageCallback.argb )
    {
        int bpp = 4 ;
        if( dst->GetPixelFormat() == PIX_FMT_RGB888
            || dst->GetPixelFormat() == PIX_FMT_BGR888 )
            bpp = 3 ;
        ret = imageCallback.argb( dst->GetPtr() , dst->GetWidth() , dst->GetHeight() , bpp , dst->GetVirWidth() , imageCallback.para ) ; //bpp = 4 ARGB

        //Media Buffer cache同步，CPU写Buffer后会有部分数据留在缓存中没没能及时同步到内存(DDR)中，
        //此时使用该接口就是保障CPU操作Buffer之后，缓存被立即刷新到内存。
        if( ret )
        {
            dst->BeginCPUAccess(true);
            dst->EndCPUAccess(true);
        }
    }
    dataMutex.unlock() ;
    
    return 0 ;
}

int  VpssFilter::IoCtrl(unsigned long int request, ...) 
{
    void  * para ;
    va_list  vl ;
    int ret = 0 ;
    
    va_start(vl, request);
    para = va_arg(vl, void *);
    va_end(vl);
    
    dataMutex.lock() ;  
    
    switch (request) 
    {
        case OSD_OP_CREATE :
            ret = CreateOsd( (OSD_PLANE_INFO *)para ) ;
            break;
        
        case OSD_OP_GETPTR :
            ret = GetOsdPtr( (OSD_PLANE_INFO *)para ) ;
            break ;
        
        case OSD_OP_UPDATE :
            ret = UpdateOsd( (OSD_PLANE_INFO *)para ) ;            
            break ;
        
        case OSD_OP_REMOVE :
            ret = DestroyOsd( ((OSD_PLANE_INFO *)para)->id ) ;            
            break ;
        
        case OSD_OP_START :
            enableFlag = 1 ;
            break ;
            
        case OSD_OP_HALT :
            enableFlag = 0 ;
            break ;
                    
        case OSD_OP_CALLBACK :                      
            if( para == NULL )
            {
                imageCallback.argb  = NULL ;
                imageCallback.yuyv  = NULL ;
                imageCallback.video = NULL ;
                imageCallback.para  = NULL ;
                break ;            
            }             
            imageCallback.argb  = ((IMAGE_CALLBACK_INFO*)para)->argb ;
            imageCallback.yuyv  = ((IMAGE_CALLBACK_INFO*)para)->yuyv ;
            imageCallback.video = ((IMAGE_CALLBACK_INFO*)para)->video;
            imageCallback.para  = ((IMAGE_CALLBACK_INFO*)para)->para ;            
            break ;
        
        case IMG_OP_ZOOM :
            sourceRect.x = ((IMAGE_ZOOM_INFO*)para)->x   ;
            sourceRect.y = ((IMAGE_ZOOM_INFO*)para)->y   ;
            sourceRect.w = ((IMAGE_ZOOM_INFO*)para)->width  ;
            sourceRect.h = ((IMAGE_ZOOM_INFO*)para)->height ;
            break ;        
               
        default:
            break;
    }
    
    dataMutex.unlock() ;
    return ret;
}

DEFINE_COMMON_FILTER_FACTORY(VpssFilter)
const char *FACTORY(VpssFilter)::ExpectedInputDataType() { return nullptr ;}
const char *FACTORY(VpssFilter)::OutPutDataType() { return ""; }




///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ImageFilter::ImageFilter(const char *param) 
{
    std::map<std::string, std::string> params;
    if (!parse_media_param_map(param, params)) {
        SetError(-EINVAL);
        return;
    }    
    
    rotation        = 0 ;
    maskFrameData   = 1 ;
    dropPattern     = 0xFFFFFFFF ; //all need process     
    
    const std::string &v = params[KEY_BUFFER_ROTATE];
    if (!v.empty())
        rotation = std::stoi(v);   
            
    sourceRect.x   = 0 ;
    sourceRect.y   = 0 ;
    sourceRect.w   = 0 ;
    sourceRect.h   = 0 ;            
}


int  ImageFilter::Process(  std::shared_ptr<MediaBuffer> input, std::shared_ptr<MediaBuffer> &output) 
{    
    if (!input || input->GetType() != Type::Image)
        return -EINVAL;
    if (!output || output->GetType() != Type::Image)
        return -EINVAL;
    
    maskFrameData = maskFrameData << 1 ; //inc count
    if( maskFrameData == 0 )
        maskFrameData = 1  ;
        
    if( ( maskFrameData & dropPattern ) == 0 ) //drop this frames
        return 1 ;

    //转换成 ImageBuffer的shared指针
    auto src = std::static_pointer_cast<easymedia::ImageBuffer>(input);
    auto dst = std::static_pointer_cast<easymedia::ImageBuffer>(output);
    
    if( src->GetPixelFormat() == dst->GetPixelFormat() 
        && src->GetWidth()  == dst->GetWidth()
        && src->GetHeight() == dst->GetHeight() 
        && rotation == 0 )
    {
            output = input ;  //直接Zero复制
            dst    = src   ;
            return 0 ;
    }
    
    //STEP 1 :  COPY the whole image to dst , convert to ARGB
    if( src->GetPixelFormat() == PIX_FMT_NV16 )
        return Y16CovertNV12( src, dst );
    else if( sourceRect.w == 0 || sourceRect.h == 0 )
        return VpssRgaBlit( src, dst , NULL , NULL , rotation ) ; //图像转换：缩放、CSC 
    
    ///////////////需要裁剪、缩放、CSC ////////////////////    
    return VpssRgaBlit( src, dst , &sourceRect , NULL , rotation ) ; //zoom          
}

int  ImageFilter::IoCtrl(unsigned long int request, ...) 
{
    void * param ;
    va_list  vl ;
    
    va_start(vl, request);
    param = va_arg(vl, void *);
    va_end(vl);

    if ( !param )
        return 1 ;
    
    switch (request) 
    {
        case VO_OP_SET_PATTERN :
            dropPattern = *((int*)param);
            break;
        
        case VO_OP_GET_PATTERN :
            *((int*)param)  = dropPattern;    
            break ;
            
        case IMG_OP_ZOOM :            
            sourceRect.x = ((IMAGE_ZOOM_INFO*)param)->x   ;
            sourceRect.y = ((IMAGE_ZOOM_INFO*)param)->y   ;
            sourceRect.w = ((IMAGE_ZOOM_INFO*)param)->width  ;
            sourceRect.h = ((IMAGE_ZOOM_INFO*)param)->height ;            
            break ;            
    }
    return 1;
}

DEFINE_COMMON_FILTER_FACTORY(ImageFilter)
const char *FACTORY(ImageFilter)::ExpectedInputDataType() { return nullptr ;}
const char *FACTORY(ImageFilter)::OutPutDataType() { return ""; }


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static bool dummy_process_buffer(Flow *f, MediaBufferVector &input_vector)
{
    DummyFlow *flow = static_cast<DummyFlow *>(f);
    auto &buffer = input_vector[0];
    if (!buffer || !flow)
        return false;
        
    ////what we should do here ? nothing !
        
    return true ;
}

DummyFlow::DummyFlow(const char *param) 
{
    SlotMap sm;
    
    sm.input_slots.push_back(0);
    sm.thread_model   = Model::SYNC ;    
    sm.mode_when_full = InputMode::DROPCURRENT;
    sm.input_maxcachenum.push_back(0);
    sm.process = dummy_process_buffer;

    if (!InstallSlotMap(sm, "DummyFLow", 0)) 
    {
        printf("Fail to InstallSlotMap for DummyFLow\n");
        return;
    }
    SetFlowTag("DummyFLow");  
}

DummyFlow::~DummyFlow() 
{ 
    StopAllThread(); 
}

DEFINE_FLOW_FACTORY(DummyFlow, Flow)
const char *FACTORY(DummyFlow)::ExpectedInputDataType() { return nullptr; }
const char *FACTORY(DummyFlow)::OutPutDataType() { return ""; }
}




////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


static RockchipRga  rkRgaTool ;

static int get_rga_format(PixelFormat f) 
{
    std::map<PixelFormat, int> rga_format_map = 
    {
        {PIX_FMT_YUV420P, RK_FORMAT_YCbCr_420_P},
        {PIX_FMT_NV12, RK_FORMAT_YCbCr_420_SP},
        {PIX_FMT_NV21, RK_FORMAT_YCrCb_420_SP},
        {PIX_FMT_YUV422P, RK_FORMAT_YCbCr_422_P},
        {PIX_FMT_NV16, RK_FORMAT_YCbCr_422_SP},
        {PIX_FMT_NV61, RK_FORMAT_YCrCb_422_SP},
        {PIX_FMT_YUYV422, RK_FORMAT_YUYV_422},
        {PIX_FMT_UYVY422, -1},
        {PIX_FMT_RGB565, RK_FORMAT_RGB_565},
        {PIX_FMT_BGR565, -1},
        {PIX_FMT_RGB888, RK_FORMAT_BGR_888},
        {PIX_FMT_BGR888, RK_FORMAT_RGB_888},
        {PIX_FMT_ARGB8888, RK_FORMAT_BGRA_8888},
        {PIX_FMT_ABGR8888, RK_FORMAT_RGBA_8888}
    };
    auto it = rga_format_map.find(f);
    if (it != rga_format_map.end())
        return it->second;
    return -1;
}

int Y16CovertNV12(std::shared_ptr<easymedia::ImageBuffer> src, std::shared_ptr<easymedia::ImageBuffer> dst)
{
    if (!src || !src->IsValid())
        return -EINVAL;
    if (!dst || !dst->IsValid())
        return -EINVAL;

    uint8_t * ptr_srcY , * ptr_srcUV , * ptr_dstY , *ptr_dstUV ;
    int  i , len_src , len_dst ;

    uint8x16x2_t  dstY ;
    uint8x16_t    srcUV , srcY ;

    len_src = src->GetVirWidth() * src->GetHeight() ;
    ptr_srcY  = (uint8_t*)src->GetPtr() ;
    ptr_srcUV = ptr_srcY + len_src  ;

    len_dst = dst->GetVirWidth() * dst->GetHeight() ;
    ptr_dstY  = (uint8_t*)dst->GetPtr() ;
    ptr_dstUV = ptr_dstY + len_dst ;

   // printf("%d-%d-%d\n",src->GetWidth(),src->GetVirWidth(),dst->GetVirWidth());
   // printf("%d-%d-%d-%d\n",src->GetHeight(),dst->GetHeight(),src->GetValidSize(),dst->GetValidSize());
#if 0  //C
    int  x , y ;
    for( y = 0 ; y < src->GetHeight() ; y++ )
    {
         for( x = 0 ; x < src->GetWidth() ; x++ )
         {
             ptr_dstY[2*x + 0] = ptr_srcUV[x] ;
             ptr_dstY[2*x + 1] = ptr_srcY [x] ;
         }

         ptr_srcY  +=  src->GetVirWidth();
         ptr_srcUV +=  src->GetVirWidth();
         ptr_dstY  +=  dst->GetVirWidth();
    }

#else //NEON加速处理，此处要求16 Byte对齐, src->GetVirWidth()==src->GetWidth()

    for( i = 0 ; i < len_src ; i += 16 )
    {
        //uint8x16_t   vld1q_u8 (const uint8_t *a); //将内存起始地址为a的后面16字节的内容赋值给类型为uint8x16_t的向量
        //uint8x16x2_t vzipq_u8 (uint8x16_t __a, uint8x16_t __b); //先取a一个数据，再取b一个数据，a0 b0 a1 b1 ...

        srcUV  = vld1q_u8( ptr_srcUV + i );
        srcY   = vld1q_u8( ptr_srcY  + i );
        dstY   = vzipq_u8( srcUV, srcY );

        vst1q_u8( ptr_dstY + 0  + i*2 , dstY.val[0] );
        vst1q_u8( ptr_dstY + 16 + i*2 , dstY.val[1] );
    }

#endif

    memset( ptr_dstUV , 0x80 , len_dst>>1 ) ;

    return 0;

}

int  VpssRgaBlit( std::shared_ptr<easymedia::ImageBuffer> src, std::shared_ptr<easymedia::ImageBuffer> dst, ImageRect *src_rect , ImageRect *dst_rect , int rotation ,   int blend )
{
    if (!src || !src->IsValid())
        return -EINVAL;
    if (!dst || !dst->IsValid())
        return -EINVAL;
        
        
    rga_info_t src_info, dst_info;
    
    ////SRC
    memset(&src_info, 0, sizeof(src_info));
    src_info.fd      = src->GetFD();  
    src_info.virAddr = src->GetPtr();
    src_info.mmuFlag = 1;
  
    if( rotation == 0 )
        src_info.rotation= 0 ;
    else if( rotation == 90 )
        src_info.rotation= HAL_TRANSFORM_ROT_90 ;
    else if( rotation == 180 )
        src_info.rotation= HAL_TRANSFORM_ROT_180 ;
    else if( rotation == 270 )
        src_info.rotation= HAL_TRANSFORM_ROT_270 ;
    else
        src_info.rotation = 0 ;
    
    if (src_rect)
        rga_set_rect(&src_info.rect, src_rect->x, src_rect->y, src_rect->w,
                     src_rect->h, src->GetVirWidth(), src->GetVirHeight(),
                     get_rga_format(src->GetPixelFormat()));
    else
        rga_set_rect(&src_info.rect, 0, 0, src->GetWidth(), src->GetHeight(),
                     src->GetVirWidth(), src->GetVirHeight(),
                     get_rga_format(src->GetPixelFormat()));  
    
    src_info.blend = blend ;
      
    /////DST                 
    memset(&dst_info, 0, sizeof(dst_info));                 
    dst_info.fd      = dst->GetFD();  
    dst_info.virAddr = dst->GetPtr();
    dst_info.mmuFlag = 1;
    if (dst_rect)
        rga_set_rect(&dst_info.rect, dst_rect->x, dst_rect->y, dst_rect->w,
                     dst_rect->h, dst->GetVirWidth(), dst->GetVirHeight(),
                     get_rga_format(dst->GetPixelFormat()));
    else
        rga_set_rect(&dst_info.rect, 0, 0, dst->GetWidth(), dst->GetHeight(),
                     dst->GetVirWidth(), dst->GetVirHeight(),
                     get_rga_format(dst->GetPixelFormat()));    

    // flush cache,  2688x1520 NV12 cost 1399us, 1080P cost 905us
    //src->BeginCPUAccess(false);
    //src->EndCPUAccess(false);

    int ret = rkRgaTool.RkRgaBlit(&src_info, &dst_info, NULL);
    if (ret) {
        dst->SetValidSize(0);
        printf("%s : Fail to RkRgaBlit, ret=%d\n", __FUNCTION__ , ret);
    } else {
        size_t valid_size = CalPixFmtSize(dst->GetPixelFormat(),
                                          dst->GetVirWidth(), dst->GetVirHeight(), 16);
        dst->SetValidSize(valid_size);
        if (src->GetUSTimeStamp() > dst->GetUSTimeStamp())
            dst->SetUSTimeStamp(src->GetUSTimeStamp());
        dst->SetAtomicClock(src->GetAtomicClock());
    }

    return ret;          
}    
                     
int  VpssRgbFill( std::shared_ptr<easymedia::ImageBuffer> dst , ImageRect *rect , int color )
{
    if (!dst || !dst->IsValid())
        return -EINVAL;
        
    rga_info_t  dst_info;
    
    memset(&dst_info, 0, sizeof(dst_info));                 
    dst_info.fd      = dst->GetFD();  
    dst_info.virAddr = dst->GetPtr();
    dst_info.mmuFlag = 1;
    dst_info.color   = color ;
    if (rect)
        rga_set_rect(&dst_info.rect, rect->x, rect->y, rect->w,
                     rect->h, dst->GetVirWidth(), dst->GetVirHeight(),
                     get_rga_format(dst->GetPixelFormat()));
    else
        rga_set_rect(&dst_info.rect, 0, 0, dst->GetWidth(), dst->GetHeight(),
                     dst->GetVirWidth(), dst->GetVirHeight(),
                     get_rga_format(dst->GetPixelFormat())); 
                        
    return rkRgaTool.RkRgaCollorFill( &dst_info );     
}    



