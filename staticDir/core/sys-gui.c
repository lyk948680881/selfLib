
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>




#include "sys-core.h"
#include "sys-service.h"
#include "sys-hal.h"
#include "sys-gui.h"

#include "app-vinput.h"




/////////////////////////////////////////////////////////
//     Function : gui_create_display
//  Description : create one display screen
//        Input : 
//       Output : 
GUI_SCREEN   * gui_create_display( char * devname , int w , int h , int bpp ) 
{
    GUI_SCREEN   * screen ; 
//  GUI_SURFACE  * surf ;
  
    
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;

    //16Bit RGB 
    struct fb_bitfield stR = {10, 5, 0};
    struct fb_bitfield stG = {5 , 5, 0};
    struct fb_bitfield stB = {0 , 5, 0};
    struct fb_bitfield stA = {15, 1, 0};
    
    
    //32Bit RGB
    struct fb_bitfield stR32 = {16, 8, 0};
    struct fb_bitfield stG32 = {8, 8, 0};
    struct fb_bitfield stB32 = {0, 8, 0};
    struct fb_bitfield stA32 = {24, 8, 0};       

    U32 needed ;
    int i  ;
    int ret ;
    
    int compress    = 0;
    int enableChAlpha = 0;  

    w  = ( w + 3 )/4*4 ; //4字节对齐
    h  = ( h + 1 )/2*2 ; //2字节对齐
    
    if( bpp < 8 )
    {
        LOG_PRINTF("gui_create_display , unsupport bpp [%d] " , bpp );
        return NULL ;
    }
                
    screen =  ( GUI_SCREEN * ) malloc( sizeof(GUI_SCREEN)) ;
    if( screen == NULL )
    {
        LOG_PRINTF("gui_create_display , malloc fail .");
        return NULL ;
    }
        
    memset( screen , 0 , sizeof(GUI_SCREEN) ); 
    screen->vir_addr = (void*)-1 ;
    pthread_mutex_init( &screen->screen_mutex  , NULL );  
    
    screen->accel = find_driver( "Accel2D" ) ;
    if( !screen->accel ) 
    {
        LOG_PRINTF("gui_create_display , open accel fail . ");
        gui_destroy_display( screen ) ;
        return NULL ;
    }

            
    screen->fd = open( devname , O_RDWR ) ;
    if( screen->fd < 0 )
    {
        LOG_PRINTF("gui_create_display , open %s fail . " , devname );
        gui_destroy_display( screen ) ;
        return NULL ;        
    }
    
    compress = 0 ;
    HAL_drv_ioctl( screen->accel , 0 , 0 , ACCEL_FB_COMPRESS , screen->fd , &compress ) ;            
    
    enableChAlpha = 0 ;
    HAL_drv_ioctl( screen->accel , 0 , 0 , ACCEL_FB_CHALPHA, screen->fd , &enableChAlpha) ;            

 
    
    screen->width  = w ;
    screen->height = h ;
    screen->bpp    = bpp/8 ;           //RGB16
    
    //获取fb信息
    if( (ret = ioctl( screen->fd , FBIOGET_VSCREENINFO, &vinfo )) < 0 )
    {
        LOG_PRINTF("gui_create_display , ioctl FBIOGET_VSCREENINFO fail ret = %x" ,ret  );
        gui_destroy_display( screen ) ;
        return NULL ;        
    }
    vinfo.xres_virtual      = w  ;
    vinfo.yres_virtual      = h*2;
    vinfo.xres              = w  ;
    vinfo.yres              = h  ;
    vinfo.activate          = FB_ACTIVATE_NOW;
    vinfo.bits_per_pixel    = screen->bpp * 8;
    vinfo.xoffset           = 0  ;
    vinfo.yoffset           = 0  ;
    if ( screen->bpp == 2 )
    {
        vinfo.red               = stR;
        vinfo.green             = stG;
        vinfo.blue              = stB;
        vinfo.transp            = stA;
    }else if ( screen->bpp == 4 )
    {
        vinfo.red               = stR32 ;
        vinfo.green             = stG32 ;
        vinfo.blue              = stB32 ;
        vinfo.transp            = stA32 ;
    }
      
    if( (ret = ioctl( screen->fd  , FBIOPUT_VSCREENINFO, &vinfo ) ) < 0 )
    {
        LOG_PRINTF("gui_create_display , ioctl FBIOPUT_VSCREENINFO fail %x" , ret  );
        gui_destroy_display( screen ) ;
        return NULL ;        
    } 
 
    
    
    if( ioctl( screen->fd , FBIOGET_FSCREENINFO , &finfo) < 0 )
    {
        LOG_PRINTF("gui_create_display , ioctl FBIOGET_FSCREENINFO fail "  );
        gui_destroy_display( screen ) ;
        return NULL ;         
    }
    printf("screen w = %d, h = %d \r\n",  vinfo.xres , vinfo.yres );  
    screen->length     = finfo.smem_len;
    needed             = w*h*screen->bpp * MAX_ONSCREEN_SURFACE ;
    if( needed  > screen->length )
    {
        LOG_PRINTF("gui_create_display , not enough phy memory "  );
        gui_destroy_display( screen ) ;
        return NULL ;        
    }
        
    printf("screen length = %d \r\n", screen->length );
    screen->phy_addr = finfo.smem_start;
    screen->vir_addr = mmap( NULL , screen->length , PROT_READ|PROT_WRITE, MAP_SHARED , screen->fd , 0 );
    if( screen->vir_addr == (void *)-1 )   
    {
        LOG_PRINTF("gui_create_display , mmap fail "  );
        gui_destroy_display( screen ) ;
        return NULL ;
    }
            
    for( i = 0 ; i < MAX_ONSCREEN_SURFACE ; i++ )
    {
        screen->surface[i].phy_addr  =  screen->phy_addr + i * ( w*h*(screen->bpp)) ;
        screen->surface[i].vir_addr  =  screen->vir_addr + i * ( w*h*(screen->bpp)) ;
        screen->surface[i].width     =  screen->width  ;
        screen->surface[i].height    =  screen->height ;
        screen->surface[i].bpp       =  screen->bpp    ;
        screen->surface[i].stride    =  w * (screen->bpp) ;
        screen->surface[i].accel     =  screen->accel     ;
    }        
    
    screen->back_index       = MAX_ONSCREEN_SURFACE - 1 ;
    screen->mem_info[0].start= needed                   ;
    screen->mem_info[0].space= screen->length   - needed;
                
    gui_display_colorkey( screen , 0 ) ;
    //gui_display_alpha   ( screen , 0x0 , 0x80 ) ;  
    
    //clear to zero
    gui_surface_fill   ( &screen->surface[0] , NULL , 0 ) ;    
    gui_surface_fill   ( &screen->surface[1] , NULL , 0 ) ; 
    //gui_display_show( screen , 1 );
    
    return screen ;    
}


/////////////////////////////////////////////////////////
//     Function : gui_destroy_display
//  Description : destroy one display screen
//        Input : 
//       Output : 
void   gui_destroy_display( GUI_SCREEN * screen ) 
{
    if( screen->accel )
        screen->accel = NULL  ;
    if( screen->vir_addr != (void*)-1 )
        munmap( screen->vir_addr , screen->length ) ;

    if( screen->fd )
        close( screen->fd ) ;
    
    
    pthread_mutex_destroy(&screen->screen_mutex ) ;    
    
    free( screen ) ;
}


/////////////////////////////////////////////////////////
//     Function : gui_create_blank_surface
//  Description : 
//        Input :
//       Output :
GUI_SURFACE * gui_create_surface_by_meminfo ( VIDEO_MEM_INFO *mem , int bpp )
{
    GUI_SURFACE *surf = (GUI_SURFACE *)malloc( sizeof(GUI_SURFACE) );
    
    if ( surf == NULL )
    {
        printf("no mem\r\n") ;
        exit( -1 );
    }    

    
                    
    surf->phy_addr = mem->phyAddr[0]  ;
    surf->vir_addr = mem->virAddr[0]  ;
    surf->x        = 0 ;
    surf->y        = 0 ;        
    surf->width    = mem->w ;
    surf->height   = mem->h ;
    surf->bpp      = bpp/8 ;    
    surf->stride   = mem->w * surf->bpp  ;

    surf->visible  = 1 ;
    surf->clip.x   = 0 ;
    surf->clip.y   = 0 ;
    surf->clip.width   = mem->w ;
    surf->clip.height  = mem->h ;
    surf->accel        = find_driver( "Accel2D" )  ;

    gui_surface_fill   ( surf , NULL , 0 ) ;    
    
    
    return surf ;
}


void gui_destroy_blank_surface ( GUI_SURFACE *surf )
{
    free( surf );
}



void gui_modify_surface_param ( GUI_SURFACE *surf , int w, int h, int bpp )
{
    surf->x        = 0 ;
    surf->y        = 0 ;
    surf->width    = w ;
    surf->height   = h ;
    surf->bpp      = bpp/8 ;
    surf->stride   = surf->width*surf->bpp ;
}


void gui_modify_surface_window ( GUI_SURFACE *surf , int x, int y, int w, int h , GUI_RECT *old )
{
    if ( old != NULL )
    {
        old->x      = surf->x ;
        old->y      = surf->y ;
        old->width  = surf->width ;
        old->height = surf->height ;
    }

    surf->x        = x ;
    surf->y        = y ;
    surf->width    = w ;
    surf->height   = h ;
}


/////////////////////////////////////////////////////////
//     Function : gui_create_surface
//  Description : create one off screen surface , use screen's phy address
//        Input : 
//       Output : 
GUI_SURFACE  * gui_create_surface ( GUI_SCREEN * screen , int x , int y , int w , int h ) 
{
    GUI_SURFACE  * surf ;    
    GUI_MEM_INFO * mem , *ptr ;        
    int i , find_surf , len ;
        
        
    w  = ( w + 3 )/4*4 ; //4字节对齐
    h  = ( h + 1 )/2*2 ; //2字节对齐
    
    pthread_mutex_lock ( &screen->screen_mutex ) ;
                
    surf      = screen->user_surf  ;    
    find_surf = 0 ;
    for( i = 0 ; i < MAX_USER_SURFACE ; i++ )
    {
        if( surf->phy_addr == 0 && surf->vir_addr == 0 )
        {
            //Find a slot 
            find_surf = 1 ;
            break ;
        }
        surf++ ;
    }
    
    if( !find_surf )
    {
        pthread_mutex_unlock ( &screen->screen_mutex ) ;
        LOG_PRINTF("gui_create_surface , out of surface"  );
        return NULL ;
    }
    
    
    //寻找一个最小的合适的内存块
    len       = w * h * screen->bpp ;
    ptr       = screen->mem_info    ;
    mem       = NULL  ;
    for( i = 0 ; i < MAX_MEM_INFO ; i++ )
    {
        if( ptr->space >= len  )
        {            
            if( !mem )
                mem = ptr ;
            
            if( mem->space > ptr->space ) //有更小的内存块，记录下来
                mem = ptr ;                                                 
        }        
        ptr++ ;
    }
    
    if( !mem )
    {
        pthread_mutex_unlock ( &screen->screen_mutex ) ;
        LOG_PRINTF("gui_create_surface , out of memory"  );
        return NULL ;
    }
                    
    surf->phy_addr = screen->phy_addr + mem->start  ;
    surf->vir_addr = screen->vir_addr + mem->start  ;
    surf->x        = x ;
    surf->y        = y ;        
    surf->width    = w ;
    surf->height   = h ;
    surf->stride   = w * screen->bpp  ;
    surf->bpp      = screen->bpp ;
    surf->visible  = 1 ;
    surf->clip.x   = 0 ;
    surf->clip.y   = 0 ;
    surf->clip.width   = w ;
    surf->clip.height  = h ;
    surf->accel        = screen->accel ;
    
    //调整内存大小
    mem->start    += len ;
    mem->space    -= len ;
    if( mem->space == 0 )
        mem->start =  0  ;    
    
    pthread_mutex_unlock ( &screen->screen_mutex ) ;
    
    //clear to zero  
    gui_surface_fill   ( surf , NULL , 0 ) ;
    
    return surf ;    
}

/////////////////////////////////////////////////////////
//     Function : gui_destroy_surface
//  Description : destroy one created surface
//        Input : 
//       Output : 
void  gui_destroy_surface( GUI_SCREEN * screen , GUI_SURFACE  * surf) 
{
    GUI_MEM_INFO * mem , *ptr ;        
    int  offset , len , i ;
    int  good ;
    
    
    offset = surf->phy_addr - screen->phy_addr ;
    len    = surf->stride * surf->height ;
    
    pthread_mutex_lock ( &screen->screen_mutex ) ;
    
    //step 1 . try to take the mem back     
    mem = screen->mem_info ;
    ptr = NULL ;
    good= 0    ;
    for( i = 0 ; i < MAX_MEM_INFO ; i++ )
    {
        if( mem->space )
        {
            if( mem->start + mem->space == offset )
            {                
                mem->space += len    ;
                good  = 1 ;
                break ;                
            }
        }else if( ptr == NULL ) {
            //暂时记录下来
            ptr = mem ;
        }
        
        mem++ ;
    }
    
    if( !good && ptr )
    {
        ptr->start = offset ;
        ptr->space = len    ;
        mem = ptr ;
    }
    
    //step 2 . check whether we could combine the mem blocks
    ptr    = screen->mem_info ;
    offset = mem->start + mem->space ;
    for( i = 0 ; i < MAX_MEM_INFO ; i++ )
    {
        if( ptr->start == offset )
        {
            mem->space += ptr->space ;
            ptr->space  = 0 ;
            ptr->start  = 0 ;  
            break ;          
        }        
        ptr++ ;
    }
    
                        
    memset( surf , 0 , sizeof(GUI_SURFACE) ) ;           
    
    pthread_mutex_unlock ( &screen->screen_mutex ) ;
}


/////////////////////////////////////////////////////////
//     Function : gui_display_flip
//  Description : switch back to front 
//        Input : 
//       Output : 
int   gui_display_flip   ( GUI_SCREEN * screen )
{
    struct fb_var_screeninfo vinfo;
    
    if(ioctl( screen->fd , FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        LOG_PRINTF("gui_display_flip , FBIOGET_VSCREENINFO " );
        return 0 ;
    }

    vinfo.xoffset           = 0;
    vinfo.yoffset           = screen->back_index * screen->height ;    
    if( ioctl( screen->fd , FBIOPAN_DISPLAY, &vinfo ) >= 0 )
    {
        screen->back_index = ( screen->back_index + 1 )%MAX_ONSCREEN_SURFACE ;        
    }
        
    return 1 ;    
}



/////////////////////////////////////////////////////////
//     Function : gui_display_surface
//  Description : get current backbuffer
//        Input : 
//       Output : 
GUI_SURFACE  * gui_display_surface( GUI_SCREEN * screen  ) 
{
    return screen->surface + screen->back_index ;    
}



GUI_SURFACE * gui_current_surface( GUI_SCREEN * screen )
{
    return screen->surface ;
}


GUI_SURFACE * gui_get_surface( GUI_SCREEN *screen , int index )
{
    return screen->surface + index ;
}


/////////////////////////////////////////////////////////
//     Function : gui_set_clip_rect
//  Description : set surface's clip region
//        Input : 
//       Output : 
void  gui_set_clip_rect  ( GUI_SURFACE * src  , GUI_RECT *clip ) 
{
    if( clip )    
    {
        src->clip = *clip ;
        return ;
    }
    
    //change to surface size
    src->clip.x = 0 ;
    src->clip.y = 0 ;
    src->clip.width  = src->width ;
    src->clip.height = src->height;
}


//////////////////////////////////////////////////////////
//DISPLAY相关函数
int  gui_display_alpha  ( GUI_SCREEN * screen , U8 alpha0 , U8 alpha1 )
{
    U32 data ;
    
    data = (alpha1 << 16) | alpha0 ;
    return HAL_drv_ioctl( screen->accel , 0 , 0 , ACCEL_FB_ALPHA , screen->fd , &data ) ;    
}

int gui_display_colorkey( GUI_SCREEN * screen , int colorkey )
{
    return HAL_drv_ioctl( screen->accel , 0 , 0 , ACCEL_FB_COLOKEY , screen->fd , &colorkey ) ;            
}

int gui_display_show    ( GUI_SCREEN * screen , int show )
{
    return HAL_drv_ioctl( screen->accel , 0 , 0 , ACCEL_FB_SHOW , screen->fd , &show ) ;            
}

int  gui_display_update ( GUI_SCREEN * screen , GUI_RECT * rect , GUI_SURFACE ** surface , int num ) 
{
    ACCEL_SURFACE_BLT blt ;
    
    blt.src   = (void**)surface ;
    blt.count = num     ;
    blt.dst   = gui_display_surface(screen) ;
    blt.rect  = rect    ;    
    return HAL_drv_ioctl( screen->accel , 0 , 0 , ACCEL_SURF_BLT , 0 , &blt ) ;            
}


//////////////////////////////////////////////////////////
//表面函数
GUI_RECT  gui_get_clip_rect  ( GUI_SURFACE * src )
{ 
    return src->clip ;
}

void      gui_surface_hide ( GUI_SURFACE * surf , int visible )
{ 
    surf->visible = visible ;
}

//////////////////////////////////////////////////////////
//表面函数---平台相关，没有CLIP区域
int  gui_surface_copy ( GUI_SURFACE * src , GUI_SURFACE * dst , GUI_RECT * src_rect , GUI_RECT * dst_rect , int scale )
{
    ACCEL_SURFACE_COPY cp ;
    
    cp.src = src ;
    cp.dst = dst ;
    cp.s_rect = src_rect ;
    cp.d_rect = dst_rect ;
    cp.scale  = scale    ;
    return HAL_drv_ioctl( src->accel , 0 , 0 , ACCEL_SURF_COPY , 0 , &cp ) ;            
}

int  gui_surface_fill ( GUI_SURFACE * surface , GUI_RECT * rect  , U32 data ) 
{
    ACCEL_SURFACE_FILL fill ;
    
    fill.src  = surface ;
    fill.rect = rect    ;
    fill.color= data    ;
    return HAL_drv_ioctl( surface->accel , 0 , 0 , ACCEL_SURF_FILL , 0 , &fill ) ;            
}

int gui_surface_clut ( GUI_SURFACE * src, GUI_SURFACE *dst , int type )
{
    ACCEL_SURFACE_CLUT clut ;
    
    clut.src   = src ;
    clut.dst   = dst ;
    clut.rect  = NULL ;
    clut.type  = type ;


    return HAL_drv_ioctl( dst->accel , 0, 0, ACCEL_SURF_CLUT , 0, &clut );
}

int  gui_surface_blend( GUI_SURFACE * src , GUI_SURFACE * dst , GUI_RECT * rect  , int alpha ) 
{
    ACCEL_SURFACE_BLEND blend ;
    
    blend.src  = src ;
    blend.dst  = dst ;
    blend.rect = rect ;
    blend.alpha= alpha ;
    return HAL_drv_ioctl( src->accel , 0 , 0 , ACCEL_SURF_BLEND , 0 , &blend ) ;            
}




int gui_surface_resize ( GUI_SURFACE *src, GUI_SURFACE *dst , GUI_RECT * src_rect, GUI_RECT *dst_rect )
{
    ACCEL_SURFACE_RESIZE resize ;
    
    resize.src  = src;
    resize.dst  = dst ;
    resize.s_rect =  src_rect ;
    resize.d_rect =  dst_rect ;
    
    return HAL_drv_ioctl ( dst->accel, 0 , 0 , ACCEL_SURF_RESIZE, 0, &resize );
}


int gui_surface_merge ( GUI_SURFACE *src1 , GUI_SURFACE *src2, GUI_SURFACE *dst )
{
    ACCEL_SURFACE_ROP rop ;
    
    rop.src     = src1 ;
    rop.src2    = src2 ;
    rop.dst     = dst ;
    rop.s_rect  = NULL ;
    rop.d_rect  = NULL ;
    //printf(" src1 [%d,%d,%d,%d] src2 [%d,%d,%d,%d]\r\n", 
    //           src1->x, src1->y, src1->width , src1->height ,
    //           src2->x, src2->y, src2->width, src2->height );
    //printf(" dst [%d,%d,%d,%d]\r\n",
    //           dst->x, dst->y, dst->width, dst->height ) ;
    return HAL_drv_ioctl ( dst->accel , 0, 0 , ACCEL_SURF_OPT, 0, &rop ) ;
}

int  gui_surface_blt ( GUI_SURFACE *dst, GUI_RECT * rect , GUI_SURFACE ** surface , int num ) 
{
    ACCEL_SURFACE_BLT blt ;
    
    blt.src   = (void**)surface ;
    blt.count = num     ;
    blt.dst   = dst ;
    blt.rect  = rect    ;    
    return HAL_drv_ioctl( dst->accel , 0 , 0 , ACCEL_SURF_BLT , 0 , &blt ) ;            
}





////////////////////////////////////////////////////////////////////////////////
//基本图形函数，与CLIP区域进行运算
////////////////////////////////////////////////////////////////////////////////
#define REGION_CODE(x,y,cx,cy,cw,ch)     ( ( (y) > (cy+ch) ? 8 : 0) | \
                                           ( (y) < (cy) ? 4 : 0) | \
                                           ( (x) > (cx+cw) ? 2 : 0) | \
                                           ( (x) < (cx) ? 1 : 0) )

                                           
/////////////////////////////////////////////////////////
//     Function : gui_point_in_rect
//  Description : 
//        Input : 
//       Output : 
int   gui_point_in_rect( GUI_RECT * rect , GUI_POINT * p) 
{
    if(   p->x >= rect->x 
       && p->y >= rect->y 
       && p->x <= ( rect->x + rect->width  )
       && p->y <= ( rect->y + rect->height ) )
       return 1 ;
       
    return 0 ;
}

/////////////////////////////////////////////////////////
//     Function : gui_rect_and_op
//  Description : 
//        Input : 
//       Output : 
int  gui_rect_and_op( GUI_RECT * in1 , GUI_RECT * in2 , GUI_RECT *out )
{
    int x[2] , y[2] ;
    int x1 , y1 , x2 , y2 ;

    if(   in1->width == 0 || in1->height == 0 
       || in2->width == 0 || in2->height == 0 )
       return 0 ;

    x1   = in1->x + in1->width  ;
    y1   = in1->y + in1->height ;
    x2   = in2->x + in2->width  ;
    y2   = in2->y + in2->height ;

    x[0] = in1->x > in2->x ? in1->x : in2->x ;
    y[0] = in1->y > in2->y ? in1->y : in2->y ;
    x[1] = x1 < x2 ? x1 : x2 ;    
    y[1] = y1 < y2 ? y1 : y2 ;
    
    if( x[1] <= x[0] || y[1] <= y[0] )
        return 0 ;
    
    out->x  = x[0] ;
    out->y  = y[0] ;
    out->width  =  x[1] - x[0] ;
    out->height =  y[1] - y[0] ;
    
    return 1 ;
    
}

/////////////////////////////////////////////////////////
//     Function : gui_rect_and_op
//  Description : 
//        Input : 
//       Output : 
int  gui_rect_or_op ( GUI_RECT * in1 , GUI_RECT * in2 , GUI_RECT *out )
{
    int x[2] , y[2] ;
    int x1 , y1 , x2 , y2 ;

    if( in1->width == 0 || in1->height == 0 )
    {
        *out = *in2 ;
        return 1 ;
    }else if( in2->width == 0 || in2->height == 0 ) {
        *out = *in1 ;
        return 1 ;
    }

    x1   = in1->x + in1->width  ;
    y1   = in1->y + in1->height ;
    x2   = in2->x + in2->width  ;
    y2   = in2->y + in2->height ;

    
    x[0] = in1->x < in2->x ? in1->x : in2->x ;
    y[0] = in1->y < in2->y ? in1->y : in2->y ;
    x[1] = x1 > x2 ? x1 : x2 ;    
    y[1] = y1 > y2 ? y1 : y2 ;
    
    if( x[1] <= x[0] || y[1] <= y[0] )
        return 0 ;
    
    out->x  = x[0] ;
    out->y  = y[0] ;
    out->width  =  x[1] - x[0] ;
    out->height =  y[1] - y[0] ;
    return 1 ;    
}


/////////////////////////////////////////////////////////
//     Function : gui_clip_line
//  Description : Copy From directFB lib
//        Input : 
//       Output :                                            
int   gui_clip_line  ( GUI_RECT * rect, GUI_LINE * line )
{
    
    U8 rc1 = REGION_CODE( line->p[0].x, line->p[0].y,rect->x,rect->y,rect->width,rect->height);
    U8 rc2 = REGION_CODE( line->p[1].x, line->p[1].y,rect->x,rect->y,rect->width,rect->height);
   
    while ( rc1 | rc2) 
    {
        /* line completely outside the clipping rectangle */
        if ( rc1 & rc2)
             return 0 ;


        if (rc1) 
        {
             if (rc1 & 8) { /* divide line at bottom*/
                  line->p[0].x = line->p[0].x +( line->p[1].x-line->p[0].x) * ( rect->y+rect->height - line->p[0].y) /( line->p[1].y - line->p[0].y );
                  line->p[0].y = rect->y + rect->height ;
             }else if( rc1 & 4) { /* divide line at top*/
                  line->p[0].x = line->p[0].x +( line->p[1].x-line->p[0].x) * ( rect->y  - line->p[0].y) /( line->p[1].y - line->p[0].y );                                        
                  line->p[0].y = rect->y ;
             }else if (rc1 & 2) { /* divide line at right*/
                  line->p[0].y = line->p[0].y +( line->p[1].y-line->p[0].y) * ( rect->x + rect->width - line->p[0].x) /( line->p[1].x - line->p[0].x );                                                            
                  line->p[0].x = rect->x+rect->width ;
             }
             else if (rc1 & 1) { /* divide line at left*/
                  line->p[0].y = line->p[0].y +( line->p[1].y-line->p[0].y) * ( rect->x - line->p[0].x) /( line->p[1].x - line->p[0].x );                                                            
                  line->p[0].x = rect->x ;                    
             }
             
             rc1 = REGION_CODE( line->p[0].x, line->p[0].y,rect->x,rect->y,rect->width,rect->height);
        } else {
             if ( rc2 & 8) {  /* divide line at bottom*/
                  line->p[1].x = line->p[0].x +( line->p[1].x-line->p[0].x) * ( rect->y+rect->height - line->p[0].y) /( line->p[1].y - line->p[0].y );
                  line->p[1].y = rect->y + rect->height ;
             } else if ( rc2 & 4) { /* divide line at top*/                    
                  line->p[1].x = line->p[0].x +( line->p[1].x-line->p[0].x) * ( rect->y  - line->p[0].y) /( line->p[1].y - line->p[0].y );                                        
                  line->p[1].y = rect->y ;
             }else if  ( rc2 & 2) { /* divide line at right*/
                  line->p[1].y = line->p[0].y +( line->p[1].y-line->p[0].y) * ( rect->x + rect->width - line->p[0].x) /( line->p[1].x - line->p[0].x );                                                            
                  line->p[1].x = rect->x+rect->width ;               
             }else if (rc2 & 1) { /* divide line at right*/
                  line->p[1].y = line->p[0].y +( line->p[1].y-line->p[0].y) * ( rect->x - line->p[0].x) /( line->p[1].x - line->p[0].x );                                                            
                  line->p[1].x = rect->x ;                    
             }
             
             rc2 = REGION_CODE( line->p[1].x, line->p[1].y,rect->x,rect->y,rect->width,rect->height);
       }        
    }
          
    return 1 ;
}





/////////////////////////////////////////////////////////
//     Function : gui_get_pixel
//  Description : 以color画一个像素
//        Input : 
//       Output : 
int gui_get_pixel ( GUI_SURFACE * surface , int x , int y  )
{
    GUI_POINT p ;
    U8 * ptr ;
    int color ;
    
    color = 0 ;
    p.x = x ;
    p.y = y ;
    if( !gui_point_in_rect( &surface->clip , &p ) )
        return color ;
        
    ptr = (U8*)SURFACE_OFFSET(surface,x,y);    
    if( surface->bpp == 1 )
        color = *((U8*)ptr) ;
    else if( surface->bpp == 2 )
        color = *((U16*)ptr) ;
    else if( surface->bpp == 3 ){
        color |= (*ptr++ ) << 16 ;
        color |= (*ptr++ ) << 8  ;
        color |= (*ptr++ );
    }else if( surface->bpp == 4 )
        color = *((U32*)ptr) ;
    
    return color ;
}

/////////////////////////////////////////////////////////
//     Function : gui_draw_pixel
//  Description : 以color画一个像素
//        Input : 
//       Output : 
void gui_draw_pixel ( GUI_SURFACE * surface , int x , int y , int color) 
{
    GUI_POINT p ;
    U8 * ptr ;
    
    p.x = x ;
    p.y = y ;
    if( !gui_point_in_rect( &surface->clip , &p ) )
        return ;
        
    ptr = (U8*)SURFACE_OFFSET(surface,x,y);    
    if( surface->bpp == 1 )
        *((U8*)ptr)   = (U8)color ;
    else if( surface->bpp == 2 )
        *((U16*)ptr)  = (U16)color ;
    else if( surface->bpp == 3 ){
        *ptr++  = (color >> 16)&0xFF  ; //R
        *ptr++  = (color >> 8 )&0xFF  ; //G  
        *ptr++  = (color)&0xFF        ; //B  
    }else if( surface->bpp == 4 )
        *((U32*)ptr)  = (U32)color ;     
}

/////////////////////////////////////////////////////////
//     Function : gui_draw_pixel
//  Description : 以color按16bit画一个像素
//        Input : 
//       Output : 
inline void gui_draw_pixel_2bpp ( GUI_SURFACE * surface , int x , int y , int color) 
{
    void * ptr ;

    ptr = SURFACE_OFFSET(surface,x,y);    
    *((U16*)ptr)  = (U16)color ;
    
}


/////////////////////////////////////////////////////////
//     Function : gui_surf_offset
//  Description : 取SURFACE中的画布偏移量
//        Input : 
//       Output : 
inline void *gui_surf_offset ( GUI_SURFACE *surface, int x, int y )
{
    void * ptr ;

    ptr = SURFACE_OFFSET(surface,x,y);   

    return ptr ;
}




/////////////////////////////////////////////////////////
//     Function : gui_draw_line
//  Description : 以color画一个任意角度的线
//        Input : 
//       Output : 
void gui_draw_line  ( GUI_SURFACE * surface , int x0, int y0, int x1, int y1, int color )
{    
    int  xdelta , ydelta , xinc , yinc , rem ;
    int  count , i , stride ;
    U8 * dst ;
      
    GUI_LINE  line ;
    
    line.p[0].x = x0 ;
    line.p[0].y = y0 ;
    line.p[1].x = x1 ;
    line.p[1].y = y1 ;    
    
    if( !gui_clip_line( &surface->clip , &line ) ) //在CLIP之外，不需要画
        return ;
        
    dst    = (U8*)SURFACE_OFFSET(surface ,line.p[0].x , line.p[0].y);
    if( line.p[0].x == line.p[1].x )
    {
        //画垂直的直线
        count  = line.p[1].y - line.p[0].y ;   
        stride = surface->stride ;     
        if( count < 0 )
        {
            count  = -count ;
            stride = -stride;
        }                
        
        switch( surface->bpp )
        {
        case 1 :
            for( i = 0 ; i <= count ; i++ )
            {
                *((U8*)dst)   = (U8)color  ;
                dst += stride ;
            }
            break ;
        case 2 :
            for( i = 0 ; i <= count ; i++ )
            {
                *((U16*)dst)   = (U16)color  ;
                dst += stride ;
            }
            break ; 
        case 3 :
            for( i = 0 ; i <= count ; i++ )
            {
                dst[0]   = (color >> 16)&0xFF ; //R
                dst[1]   = (color >> 8 )&0xFF ; //G
                dst[2]   = (color)&0xFF       ; //B
                dst += stride ;
            }
            break ;       
        case 4 :
            for( i = 0 ; i <= count ; i++ )
            {
                *((U32*)dst)   = (U32)color  ;
                dst += stride ;
            }
            break ;              
        }          
        return ;
    }    
    
    if( line.p[0].y == line.p[1].y )
    {
        
        //画水平的直线
        count  = line.p[1].x - line.p[0].x ;         
        stride = 1 ;               
        if( count < 0 )
        {
            count  = -count ;
            stride = -1     ;
        }                
        
        switch( surface->bpp )
        {
        case 1 :            
            for( i = 0 ; i <= count ; i++ )
            {
                *((U8*)dst)   = (U8)color  ;
                dst += stride ;
            }
            break ;
        case 2 :
            stride = stride * 2 ;
            for( i = 0 ; i <= count ; i++ )
            {
                *((U16*)dst)   = (U16)color  ;
                dst += stride ;
            }
            break ;
        case 3 :
            stride = stride * 3 ;
            for( i = 0 ; i <= count ; i++ )
            {
                dst[0]   = (color >> 16)&0xFF ; //R
                dst[1]   = (color >> 8 )&0xFF ; //G
                dst[2]   = (color)&0xFF       ; //B
                dst     += stride ;
            }
            break ;        
        case 4 :
            stride = stride * 4 ;
            for( i = 0 ; i <= count ; i++ )
            {
                *((U32*)dst)   = (U32)color  ;
                dst += stride ;
            }
            break ;              
        }          
        return ;
    }     
    
    
    //画任意角度的线 
    xdelta = line.p[1].x - line.p[0].x ;
    ydelta = line.p[1].y - line.p[0].y ;
    xinc   = 1 ;
    yinc   = 1 ;
    if( xdelta < 0 )
    {
        xinc   = -1 ;
        xdelta = -xdelta ;
    }
    if( ydelta < 0 )
    {
        yinc   = -1 ;
        ydelta = -ydelta ;
    }    
    
    //start to drawline
    if (xdelta >= ydelta) 
    {
        rem = xdelta / 2;
        while(1)
        {
            if( line.p[0].x == line.p[1].x )
                break ;
                
            line.p[0].x += xinc;
            rem         += ydelta;
            if (rem >= xdelta) 
            {
                rem          -= xdelta;
                line.p[0].y  += yinc;
            }
            gui_draw_pixel( surface , line.p[0].x , line.p[0].y , color ) ;   
        }
    } else {
        rem = ydelta / 2;
        while(1)
        {
            if( line.p[0].y == line.p[1].y )
                break;
                
            line.p[0].y += yinc;
            rem         += xdelta;
            if (rem >= ydelta) 
            {
                rem          -= ydelta;
                line.p[0].x  += xinc  ;
            }
            gui_draw_pixel( surface , line.p[0].x , line.p[0].y , color ) ;
        }
    }          
}



/////////////////////////////////////////////////////////
//     Function : gui_draw_rect
//  Description : 以color画一个四边形
//        Input : 
//       Output : 
void gui_draw_rect  ( GUI_SURFACE * surface , int x , int y , int w , int h , int fill , int color ) 
{
    GUI_RECT  rect ;
    int i  ;
    
    //printf("[%d,%d,%d,%d]\r\n", x, y, w, h);
    if( !fill )
    {
        gui_draw_line( surface , x  , y  , x+w , y    , color ) ;
        gui_draw_line( surface , x+w, y  , x+w , y+h  , color ) ;
        gui_draw_line( surface , x  , y  , x   , y+h  , color ) ;
        gui_draw_line( surface , x  , y+h, x+w , y+h  , color ) ;
        return ;
    }    

    rect.x =  x ;
    rect.y =  y ;
    rect.width  =  w ;
    rect.height =  h ;
    //printf("rect [%d,%d,%d,%d] clip [%d,%d,%d,%d]\r\n", 
    //             rect.x, rect.y, rect.width, rect.height ,
    //             surface->clip.x, surface->clip.y, surface->clip.width, surface->clip.height );
    if( !gui_rect_and_op( &surface->clip , &rect , &rect ) ) //在CLIP区域之外，不需要画
        return  ;
    
    if( surface->accel )    
    {    
        gui_surface_fill( surface , &rect , color ) ;        
        return ;
    }
    
    //软件填充
    for( i = 0 ; i < h ; i++ )
        gui_draw_line( surface , x , y+i , x+w , y+i , color ) ;        
}


/////////////////////////////////////////////////////////
//     Function : gui_draw_circle
//  Description : 以color画一个圆
//        Input : 
//       Output : 
void gui_draw_circle( GUI_SURFACE * surface , int x , int y , int r , int fill , int color )
{
	int tx , ty , p ;
    
    tx = 0 ;
    ty = r ;
    p  = 3 - 2 * r ;
    
	if (  ( y + r) < surface->clip.y 
	    ||( x + r) < surface->clip.x 
	    ||( y - r) >= ( surface->clip.y + surface->clip.height ) 
	    ||( x - r) >= ( surface->clip.x + surface->clip.width  ) 
	    || r < 1 )
		return ;            //Out of clip region

	while( tx < ty )
	{		
	    if( fill )
	    {
	        gui_draw_line( surface , x - tx , y + ty , x + tx + 1 , y + ty , color ) ;
	        gui_draw_line( surface , x - tx , y - ty , x + tx + 1 , y - ty , color ) ;
	        
	        gui_draw_line( surface , x - ty , y + tx , x + ty + 1 , y + tx , color ) ;
	        gui_draw_line( surface , x - ty , y - tx , x + ty + 1 , y - tx , color ) ;
	    }else{
    		gui_draw_pixel( surface , x + tx , y + ty , color ) ;
        	gui_draw_pixel( surface , x - tx , y + ty , color ) ;
        	gui_draw_pixel( surface , x + tx , y - ty , color ) ;
        	gui_draw_pixel( surface , x - tx , y - ty , color ) ;
        	
        	gui_draw_pixel( surface , x + ty , y + tx , color ) ;
        	gui_draw_pixel( surface , x - ty , y + tx , color ) ;
        	gui_draw_pixel( surface , x + ty , y - tx , color ) ;
        	gui_draw_pixel( surface , x - ty , y - tx , color ) ;
        }
            							
		if (p < 0)
			p += 4 * (tx++) + 6;
		else
			p += 4 * ((tx++) - (ty--)) + 10;
	}
	
	if (tx == ty)
	{
	    if( fill )
	    {
	        gui_draw_line( surface , x - tx , y + ty , x + tx + 1 , y + ty , color ) ;
	        gui_draw_line( surface , x - tx , y - ty , x + tx + 1 , y - ty , color ) ;
	        
	        gui_draw_line( surface , x - ty , y + tx , x + ty + 1 , y + tx , color ) ;
	        gui_draw_line( surface , x - ty , y - tx , x + ty + 1 , y - tx , color ) ;
	    }else{
    		gui_draw_pixel( surface , x + tx , y + ty , color ) ;
        	gui_draw_pixel( surface , x - tx , y + ty , color ) ;
        	gui_draw_pixel( surface , x + tx , y - ty , color ) ;
        	gui_draw_pixel( surface , x - tx , y - ty , color ) ;
        	
        	gui_draw_pixel( surface , x + ty , y + tx , color ) ;
        	gui_draw_pixel( surface , x - ty , y + tx , color ) ;
        	gui_draw_pixel( surface , x + ty , y - tx , color ) ;
        	gui_draw_pixel( surface , x - ty , y - tx , color ) ;
        }	
	}
}



/////////////////////////////////////////////////////////
//     Function : gui_draw_polygon
//  Description : 以color画一个多边形
//        Input : 
//       Output : 
void gui_draw_polygon( GUI_SURFACE * surface , GUI_POINT * point , int num , int color ) 
{        
    int i ;
    
    if( num < 3 )     
        return ;
        
    for( i = 1 ; i < num ; i++ )
    {
        gui_draw_line( surface , point[i-1].x , point[i-1].y , 
                       point[i].x , point[i].y , color ) ;
    }
    
    gui_draw_line( surface , point[num-1].x , point[num-1].y , 
                   point[0].x , point[0].y , color ) ;   
}

void gui_draw_icon ( GUI_SURFACE * surface , void *icon, int x, int y, int w, int h )
{
    GUI_RECT rect ;
    int i ;
    
    rect.x = x ;
    rect.y = y ;
    rect.width = w ;
    rect.height = h ;
    
    if ( !gui_rect_and_op ( &surface->clip, &rect, &rect ) )
        return ;
        
    //LOG_PRINTF("x= %d, y = %d, rect [%d, %d, %d, %d]", x, y ,
    //                  rect.x, rect.y, rect.width , rect.height );
    int src_stride = w*surface->bpp ;
    U8 *dst = (U8 *)surface->vir_addr + rect.y * surface->stride + rect.x*surface->bpp ;
    U8 *src = (U8 *)icon + (rect.y-y)*src_stride + (rect.x-x)*surface->bpp;
    for ( i = 0 ; i < rect.height; i++ )
    {
        memcpy( dst, src, rect.width * surface->bpp );
        dst += surface->stride ;
        src += src_stride ;
    }
        
    
}



/////////////////////////////////////////////////////////
//     Function : gui_flood_fill
//  Description : 以color填充一个区域，与CLIP运行
//        Input : 
//       Output : 
#define  FLOOD_STACK_DEPTH   (1024*2)  //2K

typedef struct
{
    short x    ;
    short y    ;       
}FLOOD_FILL_STACK;

#define  FLOOD_PUSH(X,Y)   \
    if( sp < ( stack + FLOOD_STACK_DEPTH ) && (Y) > clip->y && (Y) < ( clip->y + clip->height ) ) \
    {  sp->x = (X) ;  sp->y = (Y) ;  ++sp ; }

#define  FLOOD_POP(X,Y)    \
    {  --sp ; X = sp->x ; Y = sp->y ;}

void gui_flood_fill ( GUI_SURFACE * surface , int x , int y , int color )
{
    FLOOD_FILL_STACK  *stack , *sp ;
    int  old_color , from , check ;
    GUI_POINT p    ;  
    GUI_RECT *clip ;  
    
    int  prev_line , next_line ;
       
    p.x  = x ;
    p.y  = y ;
    clip = &surface->clip ;
    if( !gui_point_in_rect( clip , &p ) )  //out of region
        return  ;        
        
    old_color = gui_get_pixel( surface , x , y ) ;
    if( old_color == color )
        return ;
    
    stack = ( FLOOD_FILL_STACK *) malloc( FLOOD_STACK_DEPTH * sizeof(FLOOD_FILL_STACK) ) ;
    if( stack == NULL )
    {
        LOG_PRINTF("gui_flood_fill , no memory " );
        return ;
    }
        
    sp    =  stack ;
    
    FLOOD_PUSH( x , y  ) ;  //UP    
                     
    while( sp > stack )
    {                        

        FLOOD_POP( from , y  ) ;
              
        //向左边填充
        prev_line = 0 ; next_line = 0 ;
        for( x = from ; x >= clip->x ; x-- )
        {
            if( gui_get_pixel( surface , x , y ) != old_color )  //到边界了
                break ;
            
            check = gui_get_pixel( surface , x , y + 1 ) ;
            if( !next_line && check == old_color )
            {
                FLOOD_PUSH( x  , y + 1  ) ;  
                next_line = 1 ;                
            }else if( next_line && check != old_color ){
                next_line = 0 ;
            }
            
            check = gui_get_pixel( surface , x , y - 1 ) ;
            if( !prev_line && check == old_color )
            {
                FLOOD_PUSH( x  , y - 1  ) ;  
                prev_line = 1 ;                
            }else if( next_line && check != old_color ){
                prev_line = 0 ;
            }       
                             
            //填充颜色    
            gui_draw_pixel( surface , x , y , color ) ;
        }
                                
      
        //向右边填充
        prev_line = 0 ; next_line = 0 ;
        for( x = from + 1 ; x <= clip->x + clip->width ; x++ )
        {
            if( gui_get_pixel( surface , x , y ) != old_color )  //到边界了
                break ;
            
            check = gui_get_pixel( surface , x , y + 1 ) ;
            if( !next_line && check == old_color )
            {
                FLOOD_PUSH( x  , y + 1 ) ;  
                next_line = 1 ;                
            }else if( next_line && check != old_color ){
                next_line = 0 ;
            }
            
            check = gui_get_pixel( surface , x , y - 1 ) ;
            if( !prev_line && check == old_color )
            {
                FLOOD_PUSH( x  , y - 1  ) ;  
                prev_line = 1 ;                
            }else if( next_line && check != old_color ){
                prev_line = 0 ;
            }          
              
            //填充颜色    
            gui_draw_pixel( surface , x , y , color ) ;
        }                                                               
    }
    
    free( stack ) ;
                                
}


static GUI_SURFACE overlay_surf = {0} ;
void gui_create_overlay_gui ( int x, int y, int w, int h )
{
    VINPUT_USER_GUI_MSG *user_gui ;
    KERNEL_MESSAGE msg ;
    int bpp = 32 ;
    
    //限制一下OSD大小不要超过D1大小
    if( w >= 720 ) w = 720 ;
    if( h >= 576 ) h = 576 ;
    //限制一下OSD大小不要超过D1大小
        
    user_gui = ( VINPUT_USER_GUI_MSG *) &msg ;
    core_fill_message(  (MESSAG_HEADER *)&msg, SYS_VINPUT, SYS_VINPUT, SYSVIN_SET_USER_GUI , sizeof(VINPUT_USER_GUI_MSG)  );
    user_gui->channel =   VIN_OSD_PRIVATEGUI ;
    user_gui->alpha   =   0 ;
    user_gui->x       =   x ;
    user_gui->y       =   x ;
    user_gui->width   =   w;
    user_gui->height  =   h;
    user_gui->bcolor  =   0 ;
    user_gui->fcolor  =   0 ;
            
    overlay_surf.bpp   = bpp/8 ;
    overlay_surf.width = w ;
    overlay_surf.height = h ;
    overlay_surf.stride = w * (bpp/8) ;
    
    overlay_surf.clip.x      = overlay_surf.clip.y = 0 ;
    overlay_surf.clip.width  = w ;
    overlay_surf.clip.height = h ;
    overlay_surf.accel       = NULL ;
    overlay_surf.visible     = 0 ;
    overlay_surf.accel       = find_driver( "Accel2D" ) ;

    overlay_surf.vir_addr = (char *)malloc( overlay_surf.width * overlay_surf.height * overlay_surf.bpp ); 

    if( overlay_surf.vir_addr )
        memset(overlay_surf.vir_addr, 0x00, overlay_surf.width * overlay_surf.height * overlay_surf.bpp);
    
    core_send_message( &msg ); 
        
    user_gui->head.command = SYSVIN_ENABLE_USER_GUI ;
    user_gui->enable       = 1 ;
    core_send_message( &msg ); 
    
}


GUI_SURFACE * gui_get_overlay_surface ()
{
    return &overlay_surf;
}


void gui_update_overlay_surface(  )
{
    VINPUT_USER_GUI_MSG *user_gui ;
    KERNEL_MESSAGE msg ;

    user_gui = ( VINPUT_USER_GUI_MSG *) &msg ;
    core_fill_message(  (MESSAG_HEADER *)&msg, SYS_VINPUT, SYS_VINPUT, SYSVIN_UPDATE_USER_GUI, sizeof(VINPUT_USER_GUI_MSG)  );

    user_gui->channel = VIN_OSD_PRIVATEGUI ;

    user_gui->width     =  overlay_surf.width  ;
    user_gui->height    =  overlay_surf.height ;
    user_gui->data      =  overlay_surf.vir_addr ;

    core_send_message( &msg ); 
}


void gui_close_overlay_surface( )
{
    KERNEL_MESSAGE              msg ;
    VINPUT_USER_GUI_MSG * user_gui ;

    user_gui = ( VINPUT_USER_GUI_MSG *) &msg ;
    core_fill_message(  (MESSAG_HEADER *)&msg, SYS_VINPUT, SYS_VINPUT, SYSVIN_ENABLE_USER_GUI , sizeof(VINPUT_USER_GUI_MSG )  );
    user_gui->channel   =  VIN_OSD_PRIVATEGUI ;
    user_gui->enable    =  0 ;

    core_send_message( &msg ); 
    
    user_gui->head.command = SYSVIN_DESTROY_USER_GUI ;
    core_send_message( &msg );  

    if( overlay_surf.vir_addr )
    {
        free(overlay_surf.vir_addr);
        overlay_surf.vir_addr = NULL;
    }
}




