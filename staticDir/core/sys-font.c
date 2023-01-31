
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <iconv.h>

#include <ft2build.h>  
#include FT_FREETYPE_H 

#include "sys-core.h"
#include "sys-service.h"
#include "sys-hal.h"
#include "sys-gui.h"



#define FT_FLOOR(X)	((X & -64) / 64)
#define FT_CEIL(X)	(((X + 63) & -64) / 64)

#define FT_MAX_CACHE      128

typedef struct 
{    
    int cached   ;
    U32 access   ;

    int size  ;            //单个字符的大小
    int style ;

    int width ;
    int rows  ;   
    int stride;
    
    int y_offset ;         //字符的Y偏移 
    int x_offset ;         //字符的X偏移
    int x_advance;         //字符X方向的大小
    
    U8 *data  ;    
}GUI_FONT_BITMAP ;


typedef struct
{
    int  size  ;           //当前字体的大小
    int  style ;
        
    //TTF attribute
    FT_Face  face  ;            
    
    int   ascent   ;
    int   height   ;
    
    pthread_mutex_t   font_mutex    ;       
    
    GUI_FONT_BITMAP    cache[ FT_MAX_CACHE ] ;    
        
}GUI_FONT_PRIVATE ;



typedef struct
{
    int         opened     ;
    
    FT_Library  ttf_lib    ;
    iconv_t     iconv_tool ;
}GUI_FONT_CTRL ;

GUI_FONT_CTRL   font_ctrl = {0} ;


GUI_FONT_BITMAP * gui_load_glyph( GUI_FONT_PRIVATE * font , U16 index ) ;
void gui_draw_char( GUI_SURFACE * surf , int x , int y , GUI_FONT_BITMAP * bitmap , int fcolor , int bcolor )  ;

//文字编码转换iconv，使用内部缓存from/to可以为同一个地址
int  gui_char_convert( char * from , char * to , int len , char * srcfmt , char *dstfmt )
{
    char  buffer[256] , *output ;
    iconv_t   ic ;
    
    int count ;
        
    ic = iconv_open( dstfmt , srcfmt ) ;
    if( ic == (iconv_t)-1 ) 
    {
        printf("\ngui_char_convert , cant open iconv lib\n" ) ;
        return 0 ;
    }
    
    count  = 250    ;
    output = buffer ;
    iconv( ic , &from , (size_t *)&len , &output , (size_t *)&count ) ;            
    iconv_close( ic ) ;
    
    count = 250 - count ;  //获取已经转换好的数据大小
    buffer[count] =   0 ;  //字符串结束
    memcpy( to , buffer , count + 1 ) ;   
    return count ;
}

/////////////////////////////////////////////////////////
//     Function : gui_font_init
//  Description : TTF font type , init iconv system
//        Input : 
//       Output : 
int        gui_font_init  ( char * local ) 
{
    if( font_ctrl.opened <= 0 )
    {
        if( FT_Init_FreeType( &font_ctrl.ttf_lib ) )
        {
            LOG_PRINTF("gui_init_font , cant init TTF lib" ) ;
            return 0 ;
        }
                
        font_ctrl.iconv_tool = iconv_open( "UNICODELITTLE" , local ) ;
        if( font_ctrl.iconv_tool == (iconv_t)-1 ) 
        {
            LOG_PRINTF("gui_init_font , cant open iconv lib" ) ;
            FT_Done_FreeType( font_ctrl.ttf_lib ) ;
            return 0 ;            
        } 
        
        font_ctrl.opened = 0 ;               
    }        
    
    font_ctrl.opened++ ;
        
    return font_ctrl.opened ;
}

/////////////////////////////////////////////////////////
//     Function : gui_font_exit
//  Description : TTF font type 
//        Input : 
//       Output : 
int        gui_font_exit  ( void )
{
    font_ctrl.opened-- ;    
    if( font_ctrl.opened <= 0 )
    {
        FT_Done_FreeType( font_ctrl.ttf_lib ) ;
        iconv_close( font_ctrl.iconv_tool ) ;
        font_ctrl.opened = 0 ;
    }        
    return font_ctrl.opened ;    
}

/////////////////////////////////////////////////////////
//     Function : gui_wide_char
//  Description : out buffer must be 2 times than in buffer
//        Input : 
//       Output : WIDE Char的个数
int  gui_wide_char  ( U8 * str , U16 * out , int count ) 
{
    int ilen  , olen  , total ;    
    char *input , *output ;
    
    if( font_ctrl.opened <= 0 )
        return 0 ;
        
    total     = 0 ;            
    while( *str && count-- > 0 )
    {
        if( *str <= 128 )  //ASCII
        {
            *out++ = (U16)(*str) ;
             str++ ;             
             total++  ;
             continue ;
        }
        
        //双字节,ICONV会修改输入参数
        ilen  =  2 ;
        olen  =  2 ; 
        input = (char*)str;  
        output= (char*)out;                              
        iconv( font_ctrl.iconv_tool , &input , (size_t *)&ilen , &output , (size_t *)&olen ) ;    

        str   += 2 ;                                                               
        out   += 1 ;

        total++ ;                  
    }    

    return total ;
}


/////////////////////////////////////////////////////////
//     Function : gui_open_font_private
//  Description : TTF font type 
//        Input : 
//       Output : 
GUI_FONT_PRIVATE * gui_open_font_private  ( char * name , int size , int style ) 
{   
    GUI_FONT_PRIVATE * font ;    
    FT_Fixed scale;

    if( font_ctrl.opened <= 0 )
        return NULL ;
                 
    font = ( GUI_FONT_PRIVATE * ) malloc( sizeof(GUI_FONT_PRIVATE) ) ;
    if( font == NULL )
    {
        LOG_PRINTF("gui_open_font_private , out of memory" ) ;
        return NULL ;
    }
    
    //zero it
    memset( font , 0 , sizeof(GUI_FONT_PRIVATE) ) ;
    
    font->size   = size ;
    font->style  = style;
  
    if( FT_New_Face( font_ctrl.ttf_lib , name , 0, &font->face ) )
    {
        LOG_PRINTF("gui_open_font_private , open face %s fail " , name ) ;
        free( font ) ;
        return NULL ;
    }
  
    if( !FT_IS_SCALABLE(font->face) )
    {
        LOG_PRINTF("gui_open_font_private , %s is not ttf font " , name ) ;
        FT_Done_Face(font->face);
        free(font);
        return NULL ;
    }
  
    /* Set the character size and use default DPI (72) */
    if ( FT_Set_Char_Size(font->face, 0, size * 64 , 0, 0) )
    {
        LOG_PRINTF("gui_open_font_private , cant set font size "  ) ;
        FT_Done_Face(font->face);
        free(font);
        return NULL ;        
    }
    
    //保存字体信息
    scale = font->face->size->metrics.y_scale ;
    font->ascent = FT_CEIL( FT_MulFix( font->face->bbox.yMax , scale ) );
    font->height = font->ascent - FT_CEIL( FT_MulFix( font->face->bbox.yMin , scale ) ) + 1 ;
        
    pthread_mutex_init( &font->font_mutex  , NULL );  
    
    return font ;
}

/////////////////////////////////////////////////////////
//     Function : gui_close_font_private
//  Description : TTF font type close
//        Input : 
//       Output : 
int  gui_close_font_private( GUI_FONT_PRIVATE * f ) 
{    
    int i ;
                    
    pthread_mutex_lock ( &f->font_mutex ) ;
    for( i = 0 ; i < FT_MAX_CACHE ; i++ )
    {
        if( f->cache[i].cached  )
        {
            if( f->cache[i].data )
                free( f->cache[i].data )  ;
            
            f->cache[i].cached = 0 ;
            free( f->cache[i].data )  ;
            f->cache[i].data   = NULL ;
        }
    }         
    pthread_mutex_unlock( &f->font_mutex ) ;   
     
    pthread_mutex_destroy(&f->font_mutex ) ;
           
    FT_Done_Face(f->face);
    free(f); 
           
    return 1 ;    
}

/////////////////////////////////////////////////////////
//     Function : gui_font_attribute_private
//  Description : TTF font type attribute
//        Input : 
//       Output : 
int gui_font_attribute_private ( GUI_FONT_PRIVATE * f , int size , int style )
{    
    FT_Fixed scale;       
    //int i ;
        
    if( f->size == size && f->style == style )
    {
        //相同，不需要处理
        return 1 ;
    }

    f->size   = size ;
    f->style  = style;    
    //free all the cache , 不再需要
    //for( i = 0 ; i < FT_MAX_CACHE ; i++ )
    //{
    //    if( f->cache[i].cached  )
    //    {
    //        if( f->cache[i].data )
    //            free( f->cache[i].data )  ;
            
    //        f->cache[i].cached = 0    ;            
    //        f->cache[i].data   = NULL ;
    //        f->cache[i].access = 0    ;
    //    }
    //}  
               
    /* Set the character size and use default DPI (72) */
    if ( FT_Set_Char_Size( f->face, 0, size * 64 , 0, 0) )
    {
        LOG_PRINTF("gui_font_attribute , cant set font size " ) ;
        return 0 ;        
    }    
    
    //保存字体信息
    scale = f->face->size->metrics.y_scale ;
    f->ascent = FT_CEIL( FT_MulFix( f->face->bbox.yMax , scale ) );
    f->height = f->ascent - FT_CEIL( FT_MulFix( f->face->bbox.yMin , scale ) ) + 1 ;    

    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : gui_load_glyph
//  Description : load one char into buffer
//        Input : 
//       Output : 
GUI_FONT_BITMAP * gui_load_glyph( GUI_FONT_PRIVATE * font , U16 index ) 
{
    GUI_FONT_BITMAP * cache , *tep , *old ;  
    FT_GlyphSlot slot ;  
    FT_Bitmap *fbitmap ;
    int i  ;
    U8  *dst , *src ;
    
    
    if( font == NULL )
        return NULL ;
    
    /*
        同时做了3件事 1. 检查当前字符是否已经在内存里（字模/大小/属性）
                    2. 获取一个空闲的区域
                    3. 获取最老的缓冲
    */
    cache  = font->cache ;            
    tep    = NULL ; 
    old    = NULL ;     
    for( i = 0 ; i < FT_MAX_CACHE ; i++ )
    {
        if( cache->cached )
        {
            if( cache->cached == index  && cache->size == font->size && cache->style == font->style )
            {
                cache->access = time( NULL ) ;          //update the access time                
                return cache ;                
            }
            
            if( old == NULL )
                old = cache ;
            
            if( old->access > cache->access )
                old = cache ;
        }else if( tep == NULL ) {
            tep = cache ;            
        }
        
        cache++ ;
    }
        
    if( tep == NULL )
    {
        //old 肯定指向了一个区域
        free( old->data ) ;        
        memset( old , 0 , sizeof(GUI_FONT_BITMAP) ) ;        
        tep = old  ;
    }
   
    tep->cached = index ;     
                
    if( FT_Load_Char( font->face , index , FT_LOAD_RENDER|FT_LOAD_MONOCHROME) )
    {   
        tep->cached = 0 ;              
        return NULL ;
    }
    
    
    //把字形保存到cache中
    slot   = font->face->glyph ;
    fbitmap= &slot->bitmap ;
    
    tep->y_offset  = font->ascent - FT_FLOOR( slot->metrics.horiBearingY ) ;
    tep->x_offset  = FT_FLOOR( slot->metrics.horiBearingX ) ;
    if( tep->y_offset < 0 )  tep->y_offset = 0 ;
    if( tep->x_offset < 0 )  tep->x_offset = 0 ;
    tep->x_advance = FT_CEIL ( slot->metrics.width ) + tep->x_offset * 2 ;    
    if( tep->x_advance == 0 ) tep->x_advance = FT_FLOOR ( slot->metrics.horiAdvance ) ;
            
    tep->rows   = fbitmap->rows ;    
    tep->width  = fbitmap->width;  
    tep->stride = fbitmap->pitch;  
    tep->data   = ( U8 * ) malloc( fbitmap->pitch * fbitmap->rows ) ;
    if( tep->data == NULL )
    {
        tep->cached = 0 ;
        return NULL ;
    }
    
    dst = tep->data ;
    src = fbitmap->buffer ;
    for( i = 0 ; i < fbitmap->rows ; i++ )
    {
        memcpy( dst , src , fbitmap->pitch ) ;
        dst += fbitmap->pitch ;
        src += fbitmap->pitch ;
    }
    
    tep->access = time( NULL ) ;    
    tep->size   = font->size   ;
    tep->style  = font->style  ;
        
    return tep ;
}




/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
//  APP应用封装
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
typedef struct
{
    GUI_FONT_PRIVATE  *ascii         ;    
    GUI_FONT_PRIVATE  *extra         ;        
}GUI_APP_FONT ;

/////////////////////////////////////////////////////////
//     Function : gui_open_font
//  Description : open one font
//        Input : 
//       Output : 
GUI_FONT   gui_open_font  ( char * ascii , char *extra ,  int size , int style ) 
{
    GUI_APP_FONT * af ;
    
    if( ascii == NULL )
        return NULL ;
    
    af = ( GUI_APP_FONT *) malloc ( sizeof( GUI_APP_FONT ) ) ; 
    if( af == NULL )
    {
        LOG_PRINTF("gui_open_font , out of memory" ) ;
        return NULL ;
    }
    
    //zero it
    memset( af , 0 , sizeof(GUI_APP_FONT) ) ;
    
    af->ascii = gui_open_font_private( ascii , size , style ) ;
    printf( "ascii [%s]= %p\n\r", ascii , af->ascii );
    if( af->ascii == NULL )
    {
        free( af ) ;
        return NULL ;
    }
    
    if( extra )
        af->extra = gui_open_font_private( extra , size , style ) ;
    printf( "extra [%s]= %p\n\r", extra , af->extra );
    
    return ( GUI_FONT ) af ;
    
}

/////////////////////////////////////////////////////////
//     Function : gui_close_font
//  Description : 
//        Input : 
//       Output : 
int        gui_close_font ( GUI_FONT  font )
{
    GUI_APP_FONT * af ;
    
    af = ( GUI_APP_FONT * ) font ;
    
    if( af->ascii )
        gui_close_font_private( af->ascii ) ;
    if( af->extra )
        gui_close_font_private( af->extra ) ;
    
    free( af ) ;
    
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : gui_take_font
//  Description : 
//        Input : 
//       Output : 
int        gui_take_font      ( GUI_FONT font ) 
{
    GUI_APP_FONT * af ;
    
    af = ( GUI_APP_FONT * ) font ;

    if( af->ascii )
        pthread_mutex_lock ( &af->ascii->font_mutex ) ;   
    if( af->extra )
        pthread_mutex_lock ( &af->extra->font_mutex ) ;   
        
    return 1 ;    
    
}
/////////////////////////////////////////////////////////
//     Function : gui_release_font
//  Description : 
//        Input : 
//       Output : 
int        gui_release_font   ( GUI_FONT font ) 
{
    GUI_APP_FONT * af ;
    
    af = ( GUI_APP_FONT * ) font ;
    
    if( af->ascii )
        pthread_mutex_unlock ( &af->ascii->font_mutex ) ;   
    if( af->extra )
        pthread_mutex_unlock ( &af->extra->font_mutex ) ;       
        
    return 1 ;
}


///////
///////以下函数需要锁住字库，否则cache可能不正确 
///////


/////////////////////////////////////////////////////////
//     Function : gui_font_attribute
//  Description : 
//        Input : 
//       Output : 
int        gui_font_attribute ( GUI_FONT  font , int size , int style )
{
    GUI_APP_FONT * af ;
    
    af = ( GUI_APP_FONT * ) font ;
    
    if( af->ascii )
        gui_font_attribute_private( af->ascii , size ,style ) ;
    if( af->extra )
        gui_font_attribute_private( af->extra , size ,style ) ;    
        
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : gui_font_height
//  Description : 
//        Input : 
//       Output : 
int        gui_font_height( GUI_FONT gf )
{
    GUI_APP_FONT *font = ( GUI_APP_FONT * ) gf ;    
    
    return font->ascii->height ;
}

/////////////////////////////////////////////////////////
//     Function : gui_text_position
//  Description : get string size
//        Input : 
//       Output : 
int  gui_text_position (  GUI_FONT gf , U16 * str , int count , int *y )
{
    GUI_APP_FONT *font = ( GUI_APP_FONT * ) gf ;    
    int i , x , yy  ;
    GUI_FONT_BITMAP * bitmap ; 
    GUI_FONT_PRIVATE* f ;
    
            
    f    = font->ascii ;    
    x    = 0 ; 
    yy   = 0 ;    
    for( i = 0 ; i < count ; i++ )
    {
        if( str[i] == 0x0D )
            continue ;
        
        f = font->ascii ;
        if( str[i] > 0x80 )
            f = font->extra ;        
        
        bitmap = gui_load_glyph( f , str[i] ) ;    
        if( !bitmap  )
            continue ;
        
        if( str[i] == 0x0A )
        {
            yy    += f->height ;
            x      = 0 ;    
            continue ;
        }
            
        x  +=  bitmap->x_advance ;                           
    }
                    
    *y  = yy ;
        
    return x ;    
}

/////////////////////////////////////////////////////////
//     Function : gui_text_position_adv
//  Description : get string size
//        Input : 
//       Output : 
int  gui_text_position_adv(  GUI_FONT gf , U16 * str , int count , int width , int *y , int *start  )
{
    GUI_APP_FONT *font = ( GUI_APP_FONT * ) gf ;    
    int i , x ,  seg , yy ;
    GUI_FONT_BITMAP * bitmap ; 
    GUI_FONT_PRIVATE* f ;
    
    
    f    = font->ascii ;        
    x    = 0 ;       
    yy   = 0 ;
    seg  = 0 ;     
    
    //获取FOCUS位置的坐标
    for( i = 0 ; i < count ; i++ )
    {
        if( str[i] == 0x0D )
            continue ;
        
        f = font->ascii ;
        if( str[i] > 0x80 )
            f = font->extra ;        
        
        bitmap = gui_load_glyph( f , str[i] ) ;    
        if( !bitmap  )
            continue ;
        
        if( str[i] == 0x0A )
        {
            yy    += f->height ;
            x      = 0 ;                         
            continue ;
        }
        
        x  +=  bitmap->x_advance ;                                                                                   
        if( x > width )
        {
            seg = i  ;                   
            x   = bitmap->x_advance ;             
        }        
    }                             
    *y     = yy ;
    *start = seg;
        
    return x ;      
}

/////////////////////////////////////////////////////////
//     Function : gui_text_size
//  Description : get string size
//        Input : 
//       Output : 
int gui_text_size( GUI_FONT  ft , U16 * str , int count , int *height ) 
{
    int i , x , maxY , maxX ;
    GUI_FONT_BITMAP * bitmap ; 
    GUI_FONT_PRIVATE* f ;
    GUI_APP_FONT* font = ( GUI_APP_FONT * ) ft ;
            
    f    = font->ascii ;    
    x    = 0 ;     
    maxX = 0 ;
    maxY = 0 ;
    for( i = 0 ; i < count ; i++ )
    {
        if( str[i] == 0x0D )
            continue ;
        
        f = font->ascii ;
        if( str[i] > 0x80 )
            f = font->extra ;        
        
        bitmap = gui_load_glyph( f , str[i] ) ;    
        if( !bitmap  )
            continue ;
        
        if( str[i] == 0x0A )
        {
            maxY     += f->height ;
            if( maxX < x  )
                maxX  = x ;             
            x         = 0 ;    
            continue ;
        }
            
        x  +=  bitmap->x_advance ;                           
    }
    
    if( f ) 
        maxY += f->height ;
        
    if( maxX < x ) 
        maxX = x ;
        
    *height  = maxY ;
        
    return maxX ;
}

    
/////////////////////////////////////////////////////////
//     Function : gui_draw_char
//  Description : draw one char to buffer
//        Input : 
//       Output : 
void gui_draw_char( GUI_SURFACE * surf , int x , int y , GUI_FONT_BITMAP * bitmap , int fcolor , int bcolor ) 
{
    int i , j ;
    U8 *src  , *ptr , mask ;
    
    src = bitmap->data ;
    for( i = 0 ; i < bitmap->rows ; i++ )
    {
        mask   = 0x80 ;
        ptr    = src  ;
        for( j = 0 ; j < bitmap->width ; j++ )
        {   
            if( *ptr & mask )
            {
                gui_draw_pixel( surf , x + j + bitmap->x_offset, y + i + bitmap->y_offset , fcolor ) ;
            }else if( bcolor ){
                gui_draw_pixel( surf , x + j + bitmap->x_offset, y + i + bitmap->y_offset , bcolor ) ;
            }            
            mask = mask >> 1 ;
            if( mask == 0 )
            {
                mask = 0x80 ;
                ptr++ ;                
            }
        }
        
        src += bitmap->stride ;
    }
}


/////////////////////////////////////////////////////////
//     Function : gui_draw_text
//  Description : draw ttf text to buffer 
//        Input : str为UNICODE
//       Output : 
int  gui_draw_text(  GUI_SURFACE * surf , GUI_FONT font , GUI_RECT * rect , U16 * str , int count , int mode , int fcolor , int bcolor )
{
    GUI_FONT_PRIVATE* f ;
    GUI_APP_FONT *  app ;
    GUI_FONT_BITMAP * bitmap ;
    int x , y , wrap , vcenter ;
    int max_x , max_y , sx , sy ;
    GUI_RECT temp ;
    U16 wide ;

    if( font_ctrl.opened <= 0 || rect == NULL )
        return 0 ;
            
    app = ( GUI_APP_FONT *) font ;    
        
    wrap     = mode & GUI_TEXT_WRAP    ;
    vcenter  = mode & GUI_TEXT_VCENTER ;
    mode     = mode & 0x0F ;
    max_x    = 0 ;
    
    if( vcenter || mode != GUI_TEXT_LEFT )
        max_x = gui_text_size( font , str , count , &max_y ) ;                
    
    if( mode == GUI_TEXT_RIGHT )    
    {        
        sx     = rect->x + rect->width - max_x ;
        sy     = rect->y ;        
        if( vcenter )
            sy = rect->y + rect->height/2 - max_y/2 ;
                    
    }else if( mode == GUI_TEXT_HCENTER ){                       
        sx     = rect->x + rect->width /2 - max_x/2 ;
        sy     = rect->y ;        
        if( vcenter )
            sy = rect->y + rect->height/2 - max_y/2 ;
    }else{         
        sx = rect->x ;
        sy = rect->y ;        
        if( vcenter )
            sy = rect->y + rect->height/2 - max_y/2 ;
    }
            
    
    //修改CLIP区域
    temp = surf->clip ;           
    gui_rect_and_op( rect , &surf->clip , &surf->clip ) ;  
            
    //开始绘制 
    x  = sx ;
    y  = sy ;
    max_x = rect->x + rect->width ;
    while( count-- > 0 && *str ) 
    {
        wide = *str++ ;
        if( wide == 0x0D )
            continue ;

        f = app->ascii ;
        if( wide > 0x80 )
            f = app->extra ;   
                    
        bitmap = gui_load_glyph( f , wide ) ;
        if( !bitmap  )
            continue ;
        
        if( wide == 0x0A )
        {
            //change line
            x  = sx   ;
            y += f->height ;
           
        }else{           
            if( wrap && ( x + bitmap->x_advance ) > max_x )
            {
                x   = sx ;
                y  += f->height ;
            }
         
            //draw char            
            gui_draw_char( surf , x , y , bitmap , fcolor , bcolor ) ;                  
            x  += bitmap->x_advance ;                                    
        }                
    }
    
    //恢复CLIP区域
    surf->clip = temp ;    
    
    return 1 ;
}

