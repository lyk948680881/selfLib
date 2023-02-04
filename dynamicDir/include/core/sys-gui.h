/****************************************************************************
  Copyright (C), 2011, Robert Fan

  Histroy  :
            1)  Created by fanshaohua   2011/08/10
    基本的图像和字体库
*/


#ifndef  ___SYS_GUI___
#define  ___SYS_GUI___
#include "sys-service.h"
#ifdef __cplusplus
    extern "C" {
#endif

#define  MAX_ONSCREEN_SURFACE   2
#define  MAX_MEM_INFO           128   //最大的内存块
#define  MAX_USER_SURFACE       64

#define  SD_DEV_NAME            "/dev/fb0"


#define SURFACE_OFFSET(s,x,y)   (s->vir_addr+s->stride*(y) + (x)*s->bpp) 

//矩形结构
typedef struct
{
    int x , y ;
    int width , height ;    
}GUI_RECT ;

//点结构
typedef struct
{
    int x , y ;
}GUI_POINT ;

//线结构
typedef struct
{
    GUI_POINT  p[2] ;
}GUI_LINE ;

//RECT operation
int   gui_rect_and_op( GUI_RECT * in1 , GUI_RECT * in2 , GUI_RECT *out ) ; //RECT 与
int   gui_rect_or_op ( GUI_RECT * in1 , GUI_RECT * in2 , GUI_RECT *out ) ; //RECT 或
int   gui_clip_line  ( GUI_RECT * rect, GUI_LINE * line ) ;  //根据RECT裁剪LINE
int   gui_point_in_rect( GUI_RECT * rect , GUI_POINT * p) ;



//GUI表面定义
typedef struct _tagGUI_SURFACE
{
    U32   phy_addr   ;  //物理地址
    void  *vir_addr   ;  //虚拟地址  
    int  x           ;  //位置
    int  y           ;  //位置
    int  width       ;
    int  height      ;
    int  stride      ;        
    int  bpp         ;       
    int  visible     ;  //是否显示
    
    GUI_RECT   clip    ;     
    HAL_DRIVER * accel ;    //硬件加速部分
}GUI_SURFACE ;


//记录内存分配情况
typedef struct 
{
    int  start  ;  //开始地址
    int  space  ;  //空间长度
}GUI_MEM_INFO ;


//GUI SCREEN定义
typedef struct
{
    U32   phy_addr   ;  //物理地址
    void *vir_addr   ;  //虚拟地址  
    U32   length     ;    
    
    int  width  ;
    int  height ;
    int  bpp    ;    
    
    //利用多余显存提供用户创建表面，这样获得的内存为物理内存
    //能够用于硬件加速处理
    GUI_MEM_INFO   mem_info [MAX_MEM_INFO]      ;    //控制内存分配情况
    GUI_SURFACE    user_surf[MAX_USER_SURFACE]  ;    //用户表面
                   
                           
    //内部显示表面    
    GUI_SURFACE surface[MAX_ONSCREEN_SURFACE] ;    //显示表面队列      
    int  back_index  ;                             //BackBuffer
        
    int  fd    ;    //   
    HAL_DRIVER * accel ;            
    
    pthread_mutex_t   screen_mutex   ;            
}GUI_SCREEN ;



//显示函数
GUI_SCREEN   * gui_create_display ( char * devname , int w , int h , int bpp ) ;
void           gui_destroy_display( GUI_SCREEN * screen ) ;
//创建OFF-screen的表面
GUI_SURFACE  * gui_create_surface ( GUI_SCREEN * screen , int x , int y , int w , int h ) ;
void           gui_destroy_surface( GUI_SCREEN * screen , GUI_SURFACE  * surf ) ;
GUI_SURFACE * gui_create_surface_by_meminfo ( VIDEO_MEM_INFO *mem , int bpp );
void          gui_destroy_blank_surface ( GUI_SURFACE *surf );
GUI_SURFACE * gui_current_surface( GUI_SCREEN * screen );
GUI_SURFACE * gui_get_surface( GUI_SCREEN *screen , int index );
void gui_modify_surface_param ( GUI_SURFACE *surf , int w, int h, int bpp );
void gui_modify_surface_window ( GUI_SURFACE *surf , int x, int y, int w, int h, GUI_RECT *old );

//将表面内容叠加到显存中
int          gui_display_update  ( GUI_SCREEN * screen , GUI_RECT * rect , GUI_SURFACE ** surface , int num ) ;
int          gui_display_flip    ( GUI_SCREEN * screen ) ;   //切换显示内容
int          gui_display_colorkey( GUI_SCREEN * screen , int colorkey ) ;
int          gui_display_alpha   ( GUI_SCREEN * screen , U8 alpha0 , U8 alpha1 ) ;   //修改透明度
int          gui_display_show    ( GUI_SCREEN * screen , int show ) ; //打开关闭


//获取当前的Backbuffer
GUI_SURFACE  * gui_display_surface( GUI_SCREEN * screen  ) ;
void           gui_set_clip_rect  ( GUI_SURFACE * src  , GUI_RECT *clip ) ;
void * gui_surf_offset ( GUI_SURFACE *surface, int x, int y );
GUI_RECT  gui_get_clip_rect  ( GUI_SURFACE * src );
void      gui_surface_hide ( GUI_SURFACE * surf , int visible );



//表面操作函数--需要硬件加速
int  gui_surface_copy ( GUI_SURFACE * src , GUI_SURFACE * dst , GUI_RECT * src_rect , GUI_RECT * dst_rect , int scale ) ; //支持CLIP，使用了dst->clip
int  gui_surface_fill ( GUI_SURFACE * surface , GUI_RECT * rect  , U32 data ) ;
int  gui_surface_blend( GUI_SURFACE * src , GUI_SURFACE * dst , GUI_RECT * rect  , int alpha ) ;
int  gui_surface_clut ( GUI_SURFACE * src, GUI_SURFACE *dst , int type  );
int  gui_surface_resize ( GUI_SURFACE *src, GUI_SURFACE *dst , GUI_RECT *src_rect, GUI_RECT *dst_rect );
int  gui_surface_merge  ( GUI_SURFACE *src1 , GUI_SURFACE *src2, GUI_SURFACE *dst );
int  gui_surface_blt    ( GUI_SURFACE *dst, GUI_RECT * rect , GUI_SURFACE ** surface , int num ) ;
//基本图形函数
int  gui_get_pixel  ( GUI_SURFACE * surface , int x , int y ) ;
void gui_draw_pixel ( GUI_SURFACE * surface , int x , int y , int color) ;
void gui_draw_pixel_2bpp ( GUI_SURFACE * surface , int x , int y , int color)  ; //with 2bpp
void gui_draw_line  ( GUI_SURFACE * surface , int x0, int y0, int x1, int y1, int color ) ;
void gui_draw_rect  ( GUI_SURFACE * surface , int x , int y , int w , int h , int fill , int color ) ;
void gui_draw_circle( GUI_SURFACE * surface , int x , int y , int r , int fill , int color ) ;
void gui_draw_polygon( GUI_SURFACE * surface , GUI_POINT * point , int num ,  int color ) ;
void gui_flood_fill ( GUI_SURFACE * surface , int x , int y , int color );
void gui_draw_icon ( GUI_SURFACE * surface , void *icon, int x, int y, int w, int h );

//////////////////////////////////////////////////////////////
#define  TTF_STYLE_BOLD           0x01
#define  TTF_STYLE_ITALIC         0x02
#define  TTF_STYLE_UNDERLINE      0x04

#define  GUI_TEXT_LEFT     0           //左对齐
#define  GUI_TEXT_HCENTER  1           //水平居中对齐　　　
#define  GUI_TEXT_RIGHT    2           //右对齐　　　
#define  GUI_TEXT_WRAP     0x10        //自动换行
#define  GUI_TEXT_VCENTER  0x20        //垂直居中

typedef void *  GUI_FONT   ;

//文字编码转换iconv，使用内部缓存from/to可以为同一个地址，调用者保证内存足够
int        gui_char_convert( char * from , char * to , int len , char * srcfmt , char *dstfmt ) ; 

//文字显示
int        gui_font_init  ( char * local ) ;
int        gui_font_exit  ( void ) ;
int        gui_wide_char  ( U8 * str , U16 * out , int count ) ; //count为转化后的wide char个数


GUI_FONT   gui_open_font  ( char * ascii , char *extra ,  int size , int style ) ;
int        gui_close_font ( GUI_FONT  font ) ;

//FONT操作的信号量
int        gui_take_font      ( GUI_FONT font ) ;
int        gui_release_font   ( GUI_FONT font ) ;

int        gui_font_attribute ( GUI_FONT  font , int size , int style ) ;
int        gui_font_height( GUI_FONT font ) ;

//bcolor为0时表示透明　str为UNICODE字符串
int  gui_draw_text (  GUI_SURFACE * surf , GUI_FONT  font , GUI_RECT * rect , 
                      U16 * str , int count , int mode , int fcolor , int bcolor ) ; 



//获取大小count的字符串显示大小
//height = 字符显示高度
//return = 字符显示宽度
int gui_text_size( GUI_FONT  font , U16 * str , int count , int *height ) ;


//获取大小count的字符串显示位置
//    y  = 最后一行显示y位置
//return = 最后一个字符显示的x位置
int  gui_text_position (  GUI_FONT font , U16 * str , int count , int *y );

//获取大小count的字符子串显示位置，返回长度按width进行分段
//start  = 最后一段的字符开始位置
//    y  = 最后一段字符显示的Y位置
//return = 最后一个字符显示的x位置
int  gui_text_position_adv(  GUI_FONT font , U16 * str , int count , int width , int *y , int *start  );

void gui_create_overlay_gui ( int x, int y, int w, int h );
GUI_SURFACE *gui_get_overlay_surface( );
void gui_update_overlay_surface( );
void gui_close_overlay_surface();





//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//GUI OBJECT定义
//
#define GUI_TEXT_TYPE    0
#define GUI_BUTTON_TYPE  1
#define GUI_SELECT_TYPE  2
#define GUI_DIGIT_TYPE   3
#define GUI_PAGE_TYPE    4
#define GUI_ICON_TYPE    5
#define GUI_SYMBOL_SELECT_TYPE   6  //与ICON不同，可以有多个图案选择，产生事件
#define GUI_MENU_TYPE    7          //菜单项
#define GUI_ROTATE_TYPE  8          //可以旋转的ICON，注意实际产生的图像与角度有关系
#define GUI_SLIDE_TYPE   9
#define GUI_BORDER_TYPE  10         //边框

#define MAX_CHILD        32


//控件背景填充类型
#define OBJ_BACK_NONE     0
#define OBJ_BACK_BLEND    1  //根据1555方式，1=COPY 0=OR
#define OBJ_BACK_COPY     2
#define OBJ_BACK_SCALE    3  //9宫格方法
#define OBJ_BACK_SCALE_H  4  //水平拉伸， 0不复制

//GUI显示缓存定义
typedef struct _tagGUI_CONTEXT
{    
    GUI_SURFACE * surface ;    
    GUI_FONT      font    ;
}GUI_CONTEXT   ;


//图片资源定义 -- ICON/SYMBOL
typedef struct
{
    int  total   ;  //同样的图片个数，可以有多个
    int  width   ;
    int  height  ;
    int  bpp     ;  // 
    U8   data[4] ;  //数据区长度=w*h*bpp/8
}IMAGE_DATA ;


typedef struct _tagGUI_OBJECT 
{
    int type       ; //控件类型
    int visible    ; //可见状态    
    int message_id ; //消息
    int x      ;
    int y      ; 
    int width  ;
    int height ;
    int fcolor ;   //前景颜色
    int bcolor ;   //背景颜色 0 = 表示透明
    int font   ;   //字体大小  
    int attribute ; //属性，每个控件定义不一样 
}GUI_OBJECT ;

#define STATIC_TEXT_NORMAL 0  //普通模式 
#define STATIC_TEXT_RECT   1  //在一个区域里显示
//静态文本
typedef struct 
{
    GUI_OBJECT  obj  ;    
    char   text[64]  ;
}GUI_STATIC_TEXT ;

//按键控件
typedef struct 
{
    GUI_OBJECT  obj  ;            
    int    checked   ;     //是否选择了
    char   text[32]  ;
    IMAGE_DATA * focus ;   //FOCUS图片
}GUI_BUTTON ;


//选择控件
typedef struct
{
    GUI_OBJECT  obj  ; 
    
    int    current ;    
    int    count   ;    //对象个数
    char  *items[ MAX_CHILD ] ;    
    char   buffer[512] ;  //对象保存缓存
    
    IMAGE_DATA * focus ;   //FOCUS图片
}GUI_SELECT ;

//数字选择控件
typedef struct
{
    GUI_OBJECT  obj  ; 
    
    int   min_data ;  //最小值
    int   max_data ;  //最大值
    int   min_step ;  //每次变化的最小步长
    int   max_step ;  //最大步长
    
    int   current_data  ;  
    int   current_step  ; //当前步长
    int   current_dir   ; //当前调整方向
    int   current_delay ; //等待重复按键计数 
}GUI_DIGIT ;



//页面控件
typedef struct
{
    GUI_OBJECT  obj  ;     
    int  current ;                
    int  count   ;  //子控件个数 >= 1
    GUI_OBJECT  * children[MAX_CHILD] ;  
    IMAGE_DATA  * back  ;   //背景图片     
}GUI_PAGE ;


#define OBJ_ICON_COPY   0
#define OBJ_ICON_MIX    1  //根据1555方式，1=COPY 0=Transparent
//图片控件
typedef struct
{
    GUI_OBJECT  obj    ;       
    IMAGE_DATA * icon  ;   //背景图片                   
}GUI_ICON ;


//符号控件
typedef struct
{
    GUI_OBJECT  obj   ; 
    int current   ;//当前选择对象    
    IMAGE_DATA * icon ;   //背景图片                 
}GUI_SYMBOL_SELECT ;


//菜单控件
typedef struct
{
    GUI_OBJECT  obj  ; 
    int  switch_id   ; //切换时发出的事件命令    
    int  current ;                
    int  count   ;  //子控件个数 >= 1
    GUI_POINT  children[MAX_CHILD] ;
    IMAGE_DATA * back  ;   //背景图片 
    IMAGE_DATA * focus ;   //FOCUS图片    
}GUI_MENU ;


//控件背景填充类型
#define OBJ_ROTATE_COPY   0
#define OBJ_ROTATE_MIX    1  //根据ARGB方式，1=COPY 0=Transparent
//旋转ICON控件
typedef struct
{
    GUI_OBJECT    obj ;  
    int      rotate_x ;   
    int      rotate_y ;   
    int      rotate_angle ;   //角度
    IMAGE_DATA * icon ;   //图片    
}GUI_ROTATE ;


//滑块控件控件
typedef struct
{
    GUI_OBJECT    obj ;
    int      back     ;
    int      min_data ;   
    int      max_data ;   
    int      current  ; 
    int      step     ;
    IMAGE_DATA * ind  ;   //图片    
}GUI_SLIDE ;

#define   OBJ_BORDER_NORMAL 0
#define   OBJ_BORDER_DASH   1

//边框控件控件
typedef struct
{
    GUI_OBJECT     obj ;
    int      thickness ; //边框宽度
    IMAGE_DATA *  back ; //居中显示
}GUI_BORDER ;



void draw_pixel ( GUI_CONTEXT * gui , int x , int y , int color) ;
void draw_line  ( GUI_CONTEXT * gui , int fx , int fy , int tx,  int ty , int color) ;
void draw_rect  ( GUI_CONTEXT * gui , int x , int y , int w , int h , int fcolor , int bcolor  ) ;
void draw_symbol    ( GUI_CONTEXT * gui , int x , int y , int w , int h , int index , IMAGE_DATA * img , int fcolor , int bcolor ) ;
void draw_text      ( GUI_CONTEXT * gui , int  x , int y , char * msg , int fcolor , int bcolor , int fsize ) ;
void draw_text_rect ( GUI_CONTEXT * gui , int x , int y , int w , int h , char * msg ,  int color , int fsize ) ;
void draw_background( GUI_CONTEXT * gui , int x , int y , int w , int h , IMAGE_DATA * img , int type ) ;
void draw_rotate    ( GUI_CONTEXT * gui , int x , int y , int w , int h , IMAGE_DATA * img , int type , int ox , int oy , int angle ) ;


#define  OP_DRAW_OBJ      0
#define  OP_GET_FOCUS     1
#define  OP_LOST_FOCUS    2
void  draw_gui_object ( GUI_CONTEXT * gui , GUI_OBJECT * obj ,  int op  , GUI_OBJECT * parent ) ; 
GUI_OBJECT * create_gui_object( int type , int x , int y , int w , int h , int msg , int fc , int bc ) ;


////图片资源库定义
#define  GET_ICON(a)   (IMAGE_DATA*)get_icon_data(a,strlen(a))

int  open_icon_lib ( char * name ) ;
void close_icon_lib( void ) ;
U8 * get_icon_data ( char * id , int len ) ;


////////////////////GUI OSD事件处理/////////////////////
#define  OSD_MSG                ( 0x1000 )
#define  OSD_GUI_CONFIRM        ( OSD_MSG + 1 )
#define  OSD_GUI_CANCEL         ( OSD_MSG + 2 )
#define  OSD_GUI_LEFT           ( OSD_MSG + 3 )
#define  OSD_GUI_RIGHT          ( OSD_MSG + 4 )
#define  OSD_GUI_UP             ( OSD_MSG + 5 )
#define  OSD_GUI_DOWN           ( OSD_MSG + 6 )
#define  OSD_GUI_CONFIRM_LONG   ( OSD_MSG + 7 )
#define  OSD_COMPASS_CORRECT    ( OSD_MSG + 8 )
#define  OSD_COMPASS_RESULT     ( OSD_MSG + 9 )

#define  OSD_GUI_REPEAT_TIMER   ( OSD_MSG + 100 )
#define  OSD_GUI_ANIMATE_TIMER  ( OSD_MSG + 101 )


////ARGB MASK
#define  OSD_ALPHA_MASK    0xFF000000
#define  OSD_COLOR_MASK    0x00FFFFFF
#define  OSD_COLOR_TRANS   0x00000000

////RGB常用颜色定义
#define  OSD_RGB_RED     0xFF0000 
#define  OSD_RGB_GREEN   0x00FF00
#define  OSD_RGB_BLUE    0x0000FF
#define  OSD_RGB_YELLOW  0xFFFF00  //黄色
#define  OSD_RGB_CYAN    0x00FFFF  //青色
#define  OSD_RGB_CRIMSON 0xFF00FF  //
#define  OSD_RGB_BLACK   0x040404  //防止透色
#define  OSD_RGB_WHITE   0xFFFFFF
#define  OSD_RGB_GRAY    0x808080

#define  OSD_ALPHA_COLOR(a,c) (((a)<<24)|(c))



////注意：GUI消息体，固定发送到APP_APPGUI模块//////////
typedef struct
{
    MESSAG_HEADER  header ;
    int    data  ;
    void * pdata ;
}OSD_GUI_MESSAGE ;

int   gui_page_event  ( GUI_CONTEXT * gui , GUI_OBJECT * obj ,  int msg , int data ) ; 


#ifdef __cplusplus
}
#endif

#endif

