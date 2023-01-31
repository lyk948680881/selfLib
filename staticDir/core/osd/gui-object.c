
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

#include "sys-core.h"
#include "sys-service.h"
#include "sys-hal.h"
#include "sys-gui.h"
#include "sys-message.h"


#define GUI_FOCUS_COLOR  OSD_ALPHA_COLOR(255,OSD_RGB_GREEN)   //GREEN


//Function   : get_image_pixel
//Description: 获取image data中指定位置的颜色           
static int  get_image_pixel ( IMAGE_DATA * img , int x , int y )
{    
    int  stride , bpp ;
    U8 * ptr ;
    
    bpp = img->bpp >> 3 ;
    stride = img->width * bpp ;
    ptr    = img->data + y * stride + x * bpp ;
    
    if( bpp == 1 )
        return (int)*ptr ;
    else if( bpp == 2 )
        return (int)*((U16*)ptr) ;
    else if( bpp == 4 )
        return *((int*)ptr) ;    
    return 0 ;
}

//Function   : draw_part_back
//Description: 复制部分的数据  
static void  draw_part_back( GUI_CONTEXT * gui , int dx , int dy , int sx , int sy , int w , int h , IMAGE_DATA *img )
{
    U8 * dst , * src ;
    int  stride , bpp , i ;
    
    bpp    = img->bpp >> 3 ;    
    stride = bpp * img->width  ;
    
    dst = SURFACE_OFFSET( gui->surface , dx , dy ) ;    
    src = img->data + sy * stride + sx * bpp ;
    for ( i = 0 ; i < h ; i++ )
    {
        memcpy( dst, src, w * gui->surface->bpp );
        dst += gui->surface->stride ;
        src += stride ;
    }
}



//Function   : draw_pixel
//Description: draw one pixel to position x y
//             if flag = 0 , then the bits will clear
void draw_pixel( GUI_CONTEXT * gui , int x , int y , int color)
{
    gui_draw_pixel( gui->surface , x , y , color ) ;
}


//Function   : draw_line
//Description: draw line with vertical or horizontal
void draw_line( GUI_CONTEXT * gui , int fx , int fy , int tx,  int ty , int color)
{    
    gui_draw_line( gui->surface , fx , fy , tx , ty , color ) ;
}


//Function    : draw_rect_box
//Description : display a static rect box on the screen
//              if bcolor != 0 , it is a solid rect
void draw_rect  ( GUI_CONTEXT * gui , int x , int y , int w , int h , int fcolor , int bcolor  )
{
    gui_draw_rect( gui->surface , x , y , w , h , bcolor != 0  , fcolor ) ;    
}


//Function    : draw_background
//Description : display background according to the image and type
void draw_background( GUI_CONTEXT * gui , int x , int y , int w , int h , IMAGE_DATA * img , int type ) 
{
    int ww  , hh , count  ; 
    int xx  , yy ;    
    int color , dst , alpha_mask ;
            
    //预处理        
    if( img->bpp == 1 ) 
        return ;   
    
    alpha_mask = 0x8000 ;    
    if( img->bpp == 4 )
        alpha_mask = 0xFF000000 ;       
              
    //根据类型进行绘制      
    switch( type )
    {        
        case OBJ_BACK_NONE :
            break ;
                      
        case OBJ_BACK_COPY : //COPY                                    
            gui_draw_icon( gui->surface , img->data , x , y , img->width , img->height ) ;
            break ;
            
        case OBJ_BACK_BLEND : //BLEND                          
            for( yy = 0 ; yy < img->height ; yy++ )
            {                       
                for( xx = 0 ; xx < img->width ; xx++ )
                {
                    color = get_image_pixel( img , xx , yy ) ;
                    dst   = gui_get_pixel  ( gui->surface , x + xx , y + yy ) ;
                    
                    if( color & alpha_mask )
                        gui_draw_pixel( gui->surface , x + xx , y + yy , color ) ;      
                    else
                        gui_draw_pixel( gui->surface , x + xx , y + yy , color | dst ) ;      
                }                              
            }            
            break ;    
        
        case OBJ_BACK_SCALE_H : //SCALE H 水平拉伸 
            {
                ww = img->width / 2 ;                
                for( yy = 0 ; yy < img->height ; yy ++ )
                {
                    //left 直接COPY
                    count  = ww;
                    for( xx = 0 ; xx < count ; xx++ )
                    {
                        color = get_image_pixel( img ,  xx , yy ) ;
                        if( color ) 
                            gui_draw_pixel( gui->surface , x + xx , y + yy , color ) ;     
                    } 
                    
                    //middle 重复填充
                    count = w - 2 * ww ;
                    color = get_image_pixel(  img , ww , yy ) ;
                    if( color ) 
                    {
                        for( xx = 0 ; xx < count  ; xx++ )                   
                            gui_draw_pixel( gui->surface , x + ww + xx , y + yy , color ) ; 
                    }
                    
                    //right  直接COPY       
                    count = ww ;
                    for( xx = 0 ; xx < count ; xx++ )
                    {
                        color = get_image_pixel( img , xx + ww , yy ) ;
                        if( color ) 
                            gui_draw_pixel( gui->surface , x + w - ww + xx , y + yy , color ) ;     
                    }         
                }
            }
            break ;            
        
        case OBJ_BACK_SCALE : //SCALE ALL 9宫格方法
            {
                //CANVAS_BACK_SCALE 9 grid
                ww = img->width / 3 ;
                hh = img->height/ 3 ;    
                
                //step 1  绘制9宫格第1 2 3块             
                for( yy = 0 ; yy < hh ; yy++ ) //高度为背景图像1/3
                {
                    //left 直接COPY
                    count  = ww;
                    for( xx = 0 ; xx < count ; xx++ )
                    {
                        color = get_image_pixel( img , xx , yy ) ;
                        gui_draw_pixel( gui->surface , x + xx , y + yy , color ) ;     
                    } 
                    
                    //middle 重复填充
                    count = w - 2 * ww ;
                    color = get_image_pixel(  img , ww + ww/2 , yy ) ;
                    for( xx = 0 ; xx < count  ; xx++ )
                        gui_draw_pixel( gui->surface , x + ww + xx , y + yy , color ) ;     
                    
                    //right  直接COPY       
                    count = ww ;
                    for( xx = 0 ; xx < count ; xx++ )
                    {
                        color = get_image_pixel(  img , xx + 2*ww , yy ) ;
                        gui_draw_pixel( gui->surface , x + w - ww + xx , y + yy , color ) ;     
                    }     
                }
                
                //step 2 绘制9宫格第4 6块   
                count  = h - 2 * hh ;
                for( xx = 0 ; xx < ww ; xx++ ) //需要填充
                {
                    //left                
                    color  = get_image_pixel( img , xx , hh + hh/2 ) ;
                    for( yy = 0 ; yy < count ; yy++ )
                        gui_draw_pixel( gui->surface , x + xx , y + hh + yy , color ) ;     
                            
                    //right
                    color  = get_image_pixel( img , xx + 2*ww , hh + hh/2 ) ;
                    for( yy = 0 ; yy < count ; yy++ )
                        gui_draw_pixel( gui->surface , x + w - ww + xx , y + hh + yy , color ) ;     
                }
            
                //step 3 绘制9宫格第7 8 9块   
                for( yy = 0 ; yy < hh ; yy++ ) //高度为背景图像1/3
                {
                    //left  直接COPY      
                    count  = ww;
                    for( xx = 0 ; xx < count ; xx++ )
                    {
                        color = get_image_pixel( img , xx , yy + 2 * hh ) ;
                        gui_draw_pixel( gui->surface , x + xx , y + h - hh + yy  , color ) ;     
                    } 
                    
                    //middle  重复填充      
                    count = w - 2 * ww ;
                    color = get_image_pixel( img , ww + ww/2 , yy + 2 * hh ) ;
                    for( xx = 0 ; xx < count ; xx++ )
                        gui_draw_pixel( gui->surface , x + ww + xx , y + h - hh + yy , color ) ;     
                    
                    //right 直接COPY
                    count = ww ;
                    for( xx = 0 ; xx < count ; xx++ )
                    {
                        color = get_image_pixel( img , xx + 2*ww , yy + 2 * hh ) ;
                        gui_draw_pixel( gui->surface , x + w - ww + xx , y + h - hh + yy , color ) ;     
                    }     
                }
                
                //Step 4 : 绘制9宫格第5块  重复填充  
                color = get_image_pixel( img , ww + ww/2 , hh + hh/2 ) ;
                gui_draw_rect( gui->surface , x + ww , y + hh , w - 2 *ww , h - 2*hh , 1 , color ) ;                
            }
            break ;        
        
        
    }
        
    
    
}

//Function    : draw_rotate
//Description : display rotate icon according to the image and type
void draw_rotate ( GUI_CONTEXT * gui , int x , int y , int w , int h , IMAGE_DATA * img , int type , int ox , int oy , int angle )
{
    float fsin, fcos , theta , fx , fy , dx , dy ;
    int   rr_w , rr_h , xx , yy ; //旋转以后的大小
    int   i , j , color , alpha_mask ;
    
    alpha_mask = 0x8000 ;    
    if( img->bpp == 4 )
        alpha_mask = 0xFF000000 ;  
    
    theta = ( angle * 3.1415926 ) / 180.0 ;
    fsin  = sin(theta);
    fcos  = cos(theta);
    
    //计算目标图像大小，1/2
    xx = (img->width  - ox) ;
    yy = (img->height - oy);
    xx = xx > ox ? xx : ox ;
    yy = yy > oy ? yy : oy ;
    
    //计算大小，把旋转的中心放在处理后的图像的中心点上
    rr_w  = (int)(fabs( xx * fcos) + fabs( yy * fsin) + 1 ) ;
    rr_h  = (int)(fabs( xx * fsin) + fabs( yy * fcos) + 1 ) ;
    
    dx = ox  - rr_w * fcos - rr_h * fsin + 0.5;
    dy = oy  + rr_w * fsin - rr_h * fcos + 0.5;
    
    //计算偏移
    ox    = w/2 - rr_w ;
    oy    = h/2 - rr_h ;
    
    //旋转图像真实大小
    rr_h  = rr_h * 2;
    rr_w  = rr_w * 2;
    for ( j = 0; j < rr_h ; j++)
    {    
        for (i = 0; i < rr_w; i++)
        {
            fx =   i * fcos + j * fsin + dx ; //四次浮点乘法和四次浮点加法
            fy = - i * fsin + j * fcos + dy ;
            xx = (int)fx ;
            yy = (int)fy ;

            //不在范围之内
            if (xx >= img->width || xx < 0 || yy >= img->height || yy < 0)
                continue ;
                                    
            color = get_image_pixel( img , xx , yy ) ;
            if( type == OBJ_ROTATE_COPY )
            {                
                gui_draw_pixel( gui->surface , x + i + ox , y + j + oy , color ) ;      
                continue ;
            }        
            
            //OBJ_ROTATE_MIX
            if( color & alpha_mask )
                gui_draw_pixel( gui->surface , x + i + ox , y + j + oy , color ) ;        
        }
    }
}



//Function    : draw_symbol
//Description : display a symbol the screen
void draw_symbol ( GUI_CONTEXT * gui , int x , int y , int w , int h , int index , IMAGE_DATA * img , int fcolor , int bcolor )
{
    int yy , xx , i  , length ;
    U8  mask , value , *ptr ;

    length = ( img->width * img->height * img->bpp ) >> 3 ; 
    length = ( length + 15 ) / 16 * 16 ;  //16字节对齐
    if( index >= img->total ) 
        index  = 0 ;
    ptr    = img->data + length * index ;
    if( img->bpp > 1 ) //不是单色
    {
        gui_draw_icon( gui->surface , ptr , x , y , img->width , img->height ) ;        
        return ;
    } 
       
    //画单色图像    
    for( yy = 0 ; yy < img->height ; yy++ )
    {
        for( xx = 0 ; xx < img->width ; xx += 8 )
        {
             
             mask  = 0x80  ;     
             value = *ptr  ; 
             for( i = 0  ; i < 8 ; i++ )
             {
                 if( value & mask  )
                     draw_pixel( gui , x + xx + i , y + yy  , fcolor ) ;           //set the bits
                 else if( bcolor )
                     draw_pixel( gui , x + xx + i , y + yy  , bcolor < 0 ? 0 : bcolor ) ;   //clear the bits
                 
                 mask = mask >> 1 ;    
             }
             ptr++ ;                               
        }
    }
    
}


//Function   : draw_text
//Description: show message from positon x and y ,
void  draw_text ( GUI_CONTEXT * gui , int  x , int y , char * msg , int fcolor , int bcolor , int fsize )
{
    int     ret ;
    U16     buf[128] ;
    GUI_RECT rect ;

    if ( gui->font == NULL )
        return ;
        
    ret = gui_wide_char( (U8*)msg , buf , 128 ) ;
    rect.x = x ;
    rect.y = y ;
    rect.width  = ret * 2 * fsize ;
    rect.height = fsize ;

    
    //画出字符串    
    gui_take_font( gui->font ) ;    
    gui_font_attribute( gui->font , fsize , 0 ) ; 
    
    gui_draw_text( gui->surface , gui->font , &rect , buf , ret , GUI_TEXT_LEFT ,
                    fcolor , bcolor ) ;  
                    
    gui_release_font( gui->font ) ;
    
}


// Function : draw_text_rect
// Description : draw text ont rect , support seperating string 
void draw_text_rect ( GUI_CONTEXT * gui , int x , int y , int w , int h , char * msg ,  int color , int fsize )
{
    int     ret ;
    U16     buf[128] ;
    GUI_RECT rect ;
           
    if ( gui->font == NULL || *msg == 0 )
        return ;
    
    ret = gui_wide_char( (U8*)msg , buf , 128 ) ;
    rect.x = x ;
    rect.y = y ;
    rect.width  = w ;
    rect.height = h ;

    
    //画出字符串    
    gui_take_font( gui->font ) ;    
    gui_font_attribute( gui->font , fsize , 0 ) ; 
    
    gui_draw_text( gui->surface , gui->font , &rect , buf , ret , GUI_TEXT_HCENTER | GUI_TEXT_VCENTER  ,
                   color , 0 ) ;  
                    
    gui_release_font( gui->font ) ;
    
}



// Function : draw_gui_object
// Description : draw a gui object according method
void  draw_gui_object( GUI_CONTEXT * gui , GUI_OBJECT * obj ,  int op , GUI_OBJECT * parent )
{
    int front , back , i , x , y  ;
    char buffer[32] ;
     
    if( obj->visible == 0 ) 
        return ; 
    
    //调整下坐标，相对parent的
    x = obj->x ;
    y = obj->y ;    
    if( parent != NULL )
    {                
        x += parent->x ;    
        y += parent->y ;  
                
        //检查是否需要刷新PAGE的背景        
        if( parent->type == GUI_PAGE_TYPE && op == OP_LOST_FOCUS )            
        {
            GUI_PAGE * page = (GUI_PAGE*)parent ;
            if( page->back != NULL )
                draw_part_back( gui,x,y,obj->x,obj->y,obj->width,obj->height,page->back) ;
        }  
    }
     
    switch( obj->type )
    {
        case GUI_TEXT_TYPE :
            //Clear it if need
            if( obj->bcolor )
                draw_rect( gui , x , y , obj->width , obj->height , obj->bcolor , obj->bcolor ) ; 
              
            if( obj->attribute == 0 )
                draw_text( gui , x , y , ((GUI_STATIC_TEXT*)obj)->text , obj->fcolor , obj->bcolor , obj->font ) ;                
            else
                draw_text_rect( gui , x , y , obj->width , obj->height , ((GUI_STATIC_TEXT*)obj)->text , obj->fcolor , obj->font ) ;
            break ;
        
        case GUI_BUTTON_TYPE :
            if( op == OP_GET_FOCUS )
            {
                back  = GUI_FOCUS_COLOR ;
                front = obj->bcolor     ;    
                
                //获取焦点，有焦点的话需要绘制            
                if( ((GUI_BUTTON*)obj)->focus ) 
                    draw_background( gui , x , y , obj->width  , obj->height , ((GUI_BUTTON*)obj)->focus , OBJ_BACK_COPY ) ;                    
                else
                    draw_rect( gui , x , y , obj->width , obj->height , back , back ) ;                    
                                                            
            }else {
                back  = ((GUI_BUTTON*)obj)->checked ? obj->fcolor : obj->bcolor ;
                front = ((GUI_BUTTON*)obj)->checked ? obj->bcolor : obj->fcolor ;                                
                if( ((GUI_BUTTON*)obj)->focus == NULL ) 
                    draw_rect( gui , x , y , obj->width , obj->height , back , back ) ;                    
            }                        
            draw_text_rect( gui , x , y , obj->width , obj->height , ((GUI_BUTTON*)obj)->text , front , obj->font ) ;
            break ;
                
            
        case GUI_SELECT_TYPE :
            if( op == OP_GET_FOCUS )
            {
                back  = GUI_FOCUS_COLOR ;
                
                //获取焦点，有焦点的话需要绘制
                if( ((GUI_SELECT*)obj)->focus ) 
                    draw_background( gui , x , y , obj->width  , obj->height , ((GUI_SELECT*)obj)->focus , OBJ_BACK_COPY ) ;                    
                else if( back )                                
                    draw_rect( gui , x , y , obj->width , obj->height , back , back ) ;
            
            }else{
                back  = obj->bcolor ;                
                if( ((GUI_SELECT*)obj)->focus == NULL && back ) 
                    draw_rect( gui , x , y , obj->width , obj->height , back , back ) ;          
            }
                                        
            draw_text_rect( gui , x , y , obj->width , obj->height , ((GUI_SELECT*)obj)->items[ ((GUI_SELECT*)obj)->current ] , obj->fcolor , obj->font ) ;
            break ;  
        
        case GUI_DIGIT_TYPE :
            if( op == OP_GET_FOCUS )
            {
                back  = GUI_FOCUS_COLOR ;
                front = obj->bcolor     ;
            }else{
                back  = obj->bcolor ;
                front = obj->fcolor     ;
            }
            
            if( obj->attribute )
            { 
                draw_rect( gui , x , y , obj->width , obj->height , back , back ) ;                 
                if( obj->attribute == 1 )
                    sprintf( buffer , "%d%%" , ((GUI_DIGIT*)obj)->current_data ) ; 
                else
                    sprintf( buffer , "%d" , ((GUI_DIGIT*)obj)->current_data ) ;
            }else{
                draw_rect( gui , x , y , obj->width , obj->height , obj->fcolor , back ) ;
                sprintf( buffer , "%d" , ((GUI_DIGIT*)obj)->current_data ) ;            
            }
            
            draw_text_rect( gui , x , y , obj->width , obj->height , buffer , front , obj->font ) ;            
            break ; 
        
        
        
        case GUI_PAGE_TYPE :
            if( op == OP_DRAW_OBJ )
            {
                //有背景图片则绘制图片
                if( ((GUI_PAGE*)obj)->back ) 
                    draw_background( gui , x , y , obj->width  , obj->height ,((GUI_PAGE*)obj)->back , obj->attribute ) ;                    
                else if( !( obj->fcolor == 0 && obj->bcolor == 0 ) )    
                    draw_rect( gui , x  , y , obj->width , obj->height , obj->fcolor , obj->bcolor ) ;
                
                //draw children
                for( i = 0 ; i < ((GUI_PAGE*)obj)->count ; i++ )
                    draw_gui_object( gui , ((GUI_PAGE*)obj)->children[i] , OP_DRAW_OBJ , obj ) ;  
                                                                    
            }else {                                
                i = ((GUI_PAGE*)obj)->current ;
                if( i >= 0  && i < ((GUI_PAGE*)obj)->count ) 
                    draw_gui_object( gui , ((GUI_PAGE*)obj)->children[i] , op , obj ) ;                                 
            }
            break ;
                 
        
        case GUI_ICON_TYPE :
            if( op == OP_DRAW_OBJ ) //居中显示
            {   
                IMAGE_DATA * img = ((GUI_ICON*)obj)->icon ; 
                if( img == NULL )
                    break ;
                
                x += ( obj->width  - img->width  ) / 2 ;
                y += ( obj->height - img->height ) / 2 ;
                if( obj->attribute == OBJ_ICON_MIX )
                    draw_background( gui , x , y , obj->width  , obj->height , img , OBJ_BACK_BLEND ) ;                                    
                else
                    draw_symbol( gui , x , y , 0 , 0 ,  0 , img , obj->fcolor , obj->bcolor ) ;                                           
            }
            break ; 
            
        case GUI_SYMBOL_SELECT_TYPE :
            if( op == OP_GET_FOCUS )
            {
                back  = GUI_FOCUS_COLOR ;
            }else{
                back  = obj->bcolor ;
            }
            draw_symbol(  gui , x , y , 0 , 0 , ((GUI_SYMBOL_SELECT*)obj)->current ,
                         ((GUI_SYMBOL_SELECT*)obj)->icon , obj->fcolor , back  ) ;
            break ; 
            
        case GUI_MENU_TYPE :
            if( obj->attribute == 0 )
            {
                //多页面切换方式
                if( ((GUI_MENU*)obj)->back ) 
                    draw_symbol(  gui , x , y , 0 , 0 , ((GUI_MENU*)obj)->current , ((GUI_MENU*)obj)->back , 0 , 0  ) ;
                break ;
            }
            
            if( op == OP_DRAW_OBJ )
            {                
                //有背景图片则绘制图片
                if( ((GUI_MENU*)obj)->back ) 
                    draw_background( gui , x , y , obj->width  , obj->height ,((GUI_MENU*)obj)->back , obj->attribute ) ;                    
                                 
            }else if( op == OP_LOST_FOCUS ){
                
                GUI_MENU * menu = (GUI_MENU*)obj ;
                if( menu->current >= 0 && menu->back && menu->focus )
                {
                    i  = menu->current ;
                    x += menu->children[i].x ;
                    y += menu->children[i].y ;
                    draw_part_back( gui , x , y , 
                                    menu->children[i].x , menu->children[i].y , 
                                    menu->focus->width  , menu->focus->height , 
                                    menu->back ) ;
                }
            }else{
                //GET FOCUS
                i  = ((GUI_MENU*)obj)->current ;
                if( i >= 0 && ((GUI_MENU*)obj)->focus )
                {
                    x += ((GUI_MENU*)obj)->children[i].x ;
                    y += ((GUI_MENU*)obj)->children[i].y ;                
                    draw_background( gui , x , y , obj->width  , obj->height , ((GUI_MENU*)obj)->focus , OBJ_BACK_BLEND ) ;                                    
                }
            }
            break ;   
            
        case GUI_ROTATE_TYPE :
            if( op == OP_DRAW_OBJ ) //居中显示
            {   
                GUI_ROTATE * rr = (GUI_ROTATE*)obj ; 
                if( rr->icon == NULL )
                    break ;                
                draw_rotate( gui , x , y , obj->width  , obj->height , rr->icon , obj->attribute , rr->rotate_x , rr->rotate_y , rr->rotate_angle ) ;                                        
            }
            break ;    
            
        case GUI_SLIDE_TYPE :            
            {
                GUI_SLIDE * slide = ( GUI_SLIDE *)obj ;
                
                //首先清除背景
                i = 0 ;
                if( slide->ind != NULL )
                    i = slide->ind->width/2 + 1 ;
                draw_rect( gui , x - i  , y  , obj->width + 2 * i  , obj->height , obj->bcolor , obj->bcolor ) ;                       
                
                i = ( slide->current - slide->min_data ) * ( obj->width - 4 ) / ( slide->max_data - slide->min_data ) ;
                i = i + 2 ;               
                draw_rect( gui , x  , y + 3 , obj->width , obj->height - 6 , slide->back , slide->back ) ;                       
                draw_rect( gui , x + 2 , y + 4 , i  , obj->height - 8 , obj->fcolor , obj->fcolor ) ;                       
                
                i = x + i ; 
                if( slide->ind != NULL )
                    draw_background( gui , i - slide->ind->width/2 , y , obj->width  , obj->height , slide->ind , obj->attribute ) ; 
            }
            break ;   
            
        case GUI_BORDER_TYPE :
            front = obj->bcolor ;
            if( op == OP_DRAW_OBJ ) //居中显示
            {   
                IMAGE_DATA * img = ((GUI_BORDER*)obj)->back ; 
                if( img != NULL )
                    draw_symbol( gui , x + ( obj->width  - img->width  ) / 2 , 
                                       y + ( obj->height - img->height ) / 2 , 
                                       0 , 0 ,  0 , img , obj->fcolor , obj->bcolor ) ;                                                           
            }else if( op == OP_GET_FOCUS ){
                front = obj->fcolor ;
            }
            
            switch( obj->attribute )
            {
                case OBJ_BORDER_DASH :
                    for( i = 0 ; i < ((GUI_BORDER*)obj)->thickness ; i++ )
                    {
                        int  dw , dh ;
                        dw = obj->width  / 4 ;
                        dh = obj->height / 4 ;

                        draw_line( gui, x , y + i                 ,  x +  dw ,  y + i                , front );
                        draw_line( gui, x , y + obj->height - i   ,  x +  dw ,  y + obj->height - i  , front );

                        draw_line( gui, x + obj->width - dw , y + i                 ,  x + obj->width  ,  y + i                , front );
                        draw_line( gui, x + obj->width - dw , y + obj->height - i   ,  x + obj->width  ,  y + obj->height - i  , front );

                        draw_line( gui, x + i , y                      ,  x +  i ,  y + dh           , front );
                        draw_line( gui, x + i , y + obj->height - dh   ,  x +  i ,  y + obj->height  , front );

                        draw_line( gui, x + obj->width - i , y                      ,  x + obj->width - i ,  y + dh           , front );
                        draw_line( gui, x + obj->width - i , y + obj->height - dh   ,  x + obj->width - i ,  y + obj->height  , front );
                    }
                    break ;

                default :
                    //画框
                    for( i = 0 ; i < ((GUI_BORDER*)obj)->thickness ; i++ )
                        draw_rect( gui , x+i , y+i , obj->width-2*i , obj->height-2*i , front , 0 ) ;
                    break ;
            }
            break ;                                                             
    }
}


// Function : gui_child_event
// Description : handle child object event 
//               Up/Down    = Change Data
//               S          = Ok
//               Timer      = Do repeat check
static int  gui_child_event( GUI_CONTEXT * gui , GUI_OBJECT * obj , int msg , int press_down , GUI_OBJECT * parent )
{
    OSD_GUI_MESSAGE  simple ;
    GUI_SYMBOL_SELECT  * sym ;
    GUI_SELECT  * sel ;
    GUI_SLIDE   * slide ;
    GUI_DIGIT * digit ; 
    GUI_MENU  * menu  ; 
    int old , ret ;
        
    simple.header.receiver =  APP_APPGUI ;            
    simple.header.sender   =  APP_APPGUI ;        
    simple.header.command  =  0    ;
    simple.header.length   =  8    ;
    simple.data    =  0    ;    
    simple.pdata   =  NULL ; 

    ret = 0 ;   
    switch( msg )
    {                        
        //确认键
        case OSD_GUI_CONFIRM :           
            if( !press_down )
                break ;
            
            if( obj->type == GUI_BUTTON_TYPE 
                || obj->type == GUI_ICON_TYPE 
                || obj->type == GUI_BORDER_TYPE ) //Accept S 
            {                
                simple.header.command =  obj->message_id ;
                simple.pdata   =  obj ;  
                                  
            }else if( obj->type == GUI_MENU_TYPE )
            { 
                simple.header.command =  obj->message_id ;
                simple.data    =  (( GUI_MENU *)obj)->current   ; 
                simple.pdata   =  obj ;                    
            }
            break ;
        
                
        case OSD_GUI_LEFT  :            
        case OSD_GUI_RIGHT :    
            if( obj->type != GUI_MENU_TYPE || !press_down )
                break ;
                
            menu = ( GUI_MENU *) obj ;
            
            //如果不是多页面切换方式，那么需要处理OP_LOST_FOCUS进行局部更新
            if( obj->attribute )
                draw_gui_object( gui , obj , OP_LOST_FOCUS , parent) ;                
            menu->current =  msg == OSD_GUI_RIGHT ? menu->current + 1 : menu->current - 1 ;
            if( menu->current >= menu->count ) 
                menu->current = 0 ;
            if( menu->current < 0 )
                menu->current = menu->count - 1 ;                        
            draw_gui_object( gui , obj , OP_GET_FOCUS  , parent) ; //refresh new item
            
            ret = 1 ;
                
            simple.header.command =  menu->switch_id ;  
            simple.data    =  menu->current   ;  
            simple.pdata   =  obj  ;    
            
            break ;
            
        case OSD_GUI_UP   :            
        case OSD_GUI_DOWN :
            if( obj->type == GUI_BUTTON_TYPE && press_down ) //Accept S 
            {                
                simple.header.command =  obj->message_id ;
                simple.pdata   =  obj ;  
                                  
            }else if( obj->type == GUI_SELECT_TYPE && press_down )
            {
                sel = ( GUI_SELECT * ) obj ;
                
                old  =  sel->current ;
                sel->current = msg == OSD_GUI_UP ? sel->current + 1 : sel->current - 1 ;
                if( sel->current >= sel->count )
                    sel->current  = sel->count - 1 ;
                if( sel->current < 0 )
                    sel->current  = 0 ;
                
                if( old == sel->current )
                    break ;
                    
                draw_gui_object( gui , obj , OP_GET_FOCUS , parent) ; //refresh new item
                ret = 1 ;
                
                simple.header.command =  obj->message_id ;  
                simple.data    =  sel->current   ;  
                simple.pdata   =  obj ;    
                
            }else if( obj->type == GUI_ICON_TYPE && press_down ){
            
                simple.header.command =  obj->message_id ;
                simple.pdata   =  obj ; 
                            
            }else if( obj->type == GUI_DIGIT_TYPE ){ //Press Up is OK 
                digit = ( GUI_DIGIT * ) obj ;                
                digit->current_step  =  press_down ? digit->min_step : 0 ; //load step if key press down
                                                                    //reset step if key press up
                digit->current_delay =  3 ;  //wait 3 times
                                                                    
                old  =  digit->current_data ;  
                digit->current_dir  =  msg == OSD_GUI_UP ? 1 : -1 ;                                                
                digit->current_data =  old + digit->current_step * digit->current_dir  ; 
                if( digit->current_data < digit->min_data )
                    digit->current_data = digit->min_data ;
                else if( digit->current_data > digit->max_data )
                    digit->current_data = digit->max_data ;   
                
                if( old == digit->current_data )
                    break ; 
                    
                draw_gui_object( gui , obj , OP_GET_FOCUS , parent ) ; //refresh new item            
                ret = 1 ;
                                                              
                simple.header.command =  obj->message_id       ;  
                simple.data    =  digit->current_data   ;  
                simple.pdata   =  obj ;    
                
            }else if( obj->type == GUI_SYMBOL_SELECT_TYPE && press_down )
            {
                sym  = ( GUI_SYMBOL_SELECT * ) obj ;
                
                old  =  sym->current ;
                sym->current = msg == OSD_GUI_UP ? sym->current + 1 : sym->current - 1 ;
                if( sym->current >= sym->icon->total )
                    sym->current  = 0 ; 
                if( sym->current < 0 )
                    sym->current  = sym->icon->total - 1 ;
                    
                if( old == sym->current )
                    break ;   
                        
                draw_gui_object( gui , obj , OP_GET_FOCUS , parent ) ; //refresh new item      
                                
                ret = 1 ;                
                simple.header.command =  obj->message_id ;  
                simple.data    =  sym->current    ;  
                simple.pdata   =  obj ;    
                
            }else if( obj->type == GUI_SLIDE_TYPE && press_down ){                
                slide = ( GUI_SLIDE * ) obj ;
      
                slide->current = msg == OSD_GUI_UP ? slide->current + slide->step : slide->current - slide->step ;
                if( slide->current < slide->min_data )
                    slide->current = slide->min_data ;
                if( slide->current > slide->max_data )
                    slide->current = slide->max_data ;
                    
                //Draw current position 
                draw_gui_object( gui , obj , OP_GET_FOCUS , parent ) ; //refresh position
                                
                ret = 1 ;                
                simple.header.command =  obj->message_id ;  
                simple.data    =  slide->current  ;  
                simple.pdata   =  obj ; 
                
            }
            break ;
                                  
        case OSD_GUI_REPEAT_TIMER :
            if( obj->type == GUI_DIGIT_TYPE )
            {
                digit = ( GUI_DIGIT * ) obj ;   
                
                if( digit->current_step == 0 )
                    break ;
                
                //等待一段时间，检查是否重复按键
                if( digit->current_delay > 0 )
                {
                    digit->current_delay-- ;
                    break ;
                }
                
                digit->current_step = digit->current_step * 2 ; 
                if( digit->current_step > digit->max_step )
                    digit->current_step = digit->max_step ;
                
                old  =  digit->current_data ;                                                                 
                digit->current_data =  old   + digit->current_step * digit->current_dir  ; 
                if( digit->current_data < digit->min_data )
                    digit->current_data = digit->min_data ;
                else if( digit->current_data > digit->max_data )
                    digit->current_data = digit->max_data ;   
                                
                if( old == digit->current_data )
                    break ; 
                
                draw_gui_object( gui , obj , OP_GET_FOCUS , parent) ; //refresh new item     
                ret = 1 ;
                                                                     
                simple.header.command =  obj->message_id      ;  
                simple.data    =  digit->current_data  ;  
                simple.pdata   =  obj ;     
            }
            break ;  
                    
                    
    }
    
    if( simple.header.command )
        core_send_message( (KERNEL_MESSAGE*)&simple  ) ;     
    
    return ret ;    
}


// Function : gui_page_next_child
//Description : get next child
static int   gui_page_next_child( GUI_PAGE * page , int dir ) 
{
    OSD_GUI_MESSAGE  simple ;
    
    GUI_OBJECT * obj ;
    int i , id ;
    
    id = page->current ;
    
    simple.header.receiver=  APP_APPGUI ;            
    simple.header.sender  =  APP_APPGUI ;        
    simple.header.command =  page->obj.message_id ;
    simple.header.length  =  8    ;
    simple.data    =  id   ;    
    simple.pdata   =  page ;     
        
    for( i = 0 ; i < page->count ; i++ )
    {
        id  = id + dir  ;

        //检查是否到边缘，并且PAGE需要获取这个事件，直接返回保持当前控件
        if( ( id < 0 || id >= page->count ) && simple.header.command !=  0 )
            break ;
        
        //回绕处理
        if( id < 0 ) id = page->count - 1 ;                            
        if( id >= page->count ) id = 0 ;
                
        obj = page->children[ id ] ;
        if( obj->type == GUI_TEXT_TYPE || obj->visible == 0 || obj->message_id <= 0 )
            continue ;
            
        return id ;
    }
    
    //需要发送消息
    if( simple.header.command != 0 )
        core_send_message( (KERNEL_MESSAGE*)&simple  ) ;
    
    return page->current ;
}



// Function : gui_page_event
// Description : handle page event 
int  gui_page_event  ( GUI_CONTEXT * gui , GUI_OBJECT * obj ,  int msg , int press_down ) 
{   
    GUI_PAGE * page = ( GUI_PAGE * ) obj ;
    int dat , ret ;
    
    ret = 0 ;
    if( obj->type != GUI_PAGE_TYPE )
        return ret ;
    
    //如果当前不是MENU控件，左右键消息给它去处理为移动焦点    
    if(  page->children[ page->current ]->type != GUI_MENU_TYPE && 
        ( msg == OSD_GUI_LEFT || msg == OSD_GUI_RIGHT ) && press_down ) 
    {        
        //Move Focus        
        dat = gui_page_next_child( page , msg == OSD_GUI_LEFT ? -1 : 1 ) ;
        if( dat != page->current  ) 
        {            
            //控件切换，发出LOST/GET FOCUS消息
            draw_gui_object( gui ,  page->children[ page->current ] , OP_LOST_FOCUS , obj ) ;
            draw_gui_object( gui ,  page->children[ dat ] , OP_GET_FOCUS , obj ) ;
            page->current = dat ;
            ret = 1 ;  
        }                    
        return ret ;
           
    }
            
    return gui_child_event( gui , page->children[ page->current ] , msg , press_down , obj ) ;        
}



// Function : create_gui_object
// Description : create one gui object
GUI_OBJECT * create_gui_object( int type , int x , int y , int w , int h , int msg , int fc , int bc )
{
    GUI_OBJECT * obj ;
    int len ;
    switch( type )
    {
        case GUI_TEXT_TYPE :
            len = sizeof(GUI_STATIC_TEXT) ;
            break ;
            
        case GUI_BUTTON_TYPE :
            len = sizeof(GUI_BUTTON) ;
            break ;
            
        case GUI_SELECT_TYPE :
            len = sizeof(GUI_SELECT) ;
            break ;
            
        case GUI_DIGIT_TYPE :
            len = sizeof(GUI_DIGIT) ;
            break ;
                         
        case GUI_PAGE_TYPE   :
            len = sizeof(GUI_PAGE);
            break ;
                
        case GUI_ICON_TYPE  :
            len = sizeof(GUI_ICON);
            break ;
            
        case GUI_SYMBOL_SELECT_TYPE :
            len = sizeof(GUI_SYMBOL_SELECT);
            break ;  
        
        case GUI_MENU_TYPE :
            len = sizeof( GUI_MENU ) ;
            break ;

        case GUI_ROTATE_TYPE :
            len = sizeof( GUI_ROTATE ) ;
            break ;
        
        case GUI_SLIDE_TYPE :
            len = sizeof( GUI_SLIDE ) ;
            break ;    
        
        case GUI_BORDER_TYPE :
            len = sizeof( GUI_BORDER ) ;
            break ;
                
        default :
            len = sizeof(GUI_OBJECT) ;
            break ;
    }
    
    obj = (GUI_OBJECT*)malloc( len ) ;
    if( obj == NULL )    
        return NULL ;
               
    obj->type  = type ;
    obj->x     = x    ;
    obj->y     = y    ;
    obj->width = w    ;
    obj->height= h    ;    
    obj->fcolor= fc   ;
    obj->bcolor= bc   ;
    obj->font  = 16   ;  //default size
    obj->visible    = 1   ;
    obj->message_id = msg ;
    obj->attribute  = 0   ;
        
    memset( obj + 1 , 0 , len - sizeof(GUI_OBJECT) ) ;
    
    return obj ;
}





