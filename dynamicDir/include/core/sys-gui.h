/****************************************************************************
  Copyright (C), 2011, Robert Fan

  Histroy  :
            1)  Created by fanshaohua   2011/08/10
    ������ͼ��������
*/


#ifndef  ___SYS_GUI___
#define  ___SYS_GUI___
#include "sys-service.h"
#ifdef __cplusplus
    extern "C" {
#endif

#define  MAX_ONSCREEN_SURFACE   2
#define  MAX_MEM_INFO           128   //�����ڴ��
#define  MAX_USER_SURFACE       64

#define  SD_DEV_NAME            "/dev/fb0"


#define SURFACE_OFFSET(s,x,y)   (s->vir_addr+s->stride*(y) + (x)*s->bpp) 

//���νṹ
typedef struct
{
    int x , y ;
    int width , height ;    
}GUI_RECT ;

//��ṹ
typedef struct
{
    int x , y ;
}GUI_POINT ;

//�߽ṹ
typedef struct
{
    GUI_POINT  p[2] ;
}GUI_LINE ;

//RECT operation
int   gui_rect_and_op( GUI_RECT * in1 , GUI_RECT * in2 , GUI_RECT *out ) ; //RECT ��
int   gui_rect_or_op ( GUI_RECT * in1 , GUI_RECT * in2 , GUI_RECT *out ) ; //RECT ��
int   gui_clip_line  ( GUI_RECT * rect, GUI_LINE * line ) ;  //����RECT�ü�LINE
int   gui_point_in_rect( GUI_RECT * rect , GUI_POINT * p) ;



//GUI���涨��
typedef struct _tagGUI_SURFACE
{
    U32   phy_addr   ;  //�����ַ
    void  *vir_addr   ;  //�����ַ  
    int  x           ;  //λ��
    int  y           ;  //λ��
    int  width       ;
    int  height      ;
    int  stride      ;        
    int  bpp         ;       
    int  visible     ;  //�Ƿ���ʾ
    
    GUI_RECT   clip    ;     
    HAL_DRIVER * accel ;    //Ӳ�����ٲ���
}GUI_SURFACE ;


//��¼�ڴ�������
typedef struct 
{
    int  start  ;  //��ʼ��ַ
    int  space  ;  //�ռ䳤��
}GUI_MEM_INFO ;


//GUI SCREEN����
typedef struct
{
    U32   phy_addr   ;  //�����ַ
    void *vir_addr   ;  //�����ַ  
    U32   length     ;    
    
    int  width  ;
    int  height ;
    int  bpp    ;    
    
    //���ö����Դ��ṩ�û��������棬������õ��ڴ�Ϊ�����ڴ�
    //�ܹ�����Ӳ�����ٴ���
    GUI_MEM_INFO   mem_info [MAX_MEM_INFO]      ;    //�����ڴ�������
    GUI_SURFACE    user_surf[MAX_USER_SURFACE]  ;    //�û�����
                   
                           
    //�ڲ���ʾ����    
    GUI_SURFACE surface[MAX_ONSCREEN_SURFACE] ;    //��ʾ�������      
    int  back_index  ;                             //BackBuffer
        
    int  fd    ;    //   
    HAL_DRIVER * accel ;            
    
    pthread_mutex_t   screen_mutex   ;            
}GUI_SCREEN ;



//��ʾ����
GUI_SCREEN   * gui_create_display ( char * devname , int w , int h , int bpp ) ;
void           gui_destroy_display( GUI_SCREEN * screen ) ;
//����OFF-screen�ı���
GUI_SURFACE  * gui_create_surface ( GUI_SCREEN * screen , int x , int y , int w , int h ) ;
void           gui_destroy_surface( GUI_SCREEN * screen , GUI_SURFACE  * surf ) ;
GUI_SURFACE * gui_create_surface_by_meminfo ( VIDEO_MEM_INFO *mem , int bpp );
void          gui_destroy_blank_surface ( GUI_SURFACE *surf );
GUI_SURFACE * gui_current_surface( GUI_SCREEN * screen );
GUI_SURFACE * gui_get_surface( GUI_SCREEN *screen , int index );
void gui_modify_surface_param ( GUI_SURFACE *surf , int w, int h, int bpp );
void gui_modify_surface_window ( GUI_SURFACE *surf , int x, int y, int w, int h, GUI_RECT *old );

//���������ݵ��ӵ��Դ���
int          gui_display_update  ( GUI_SCREEN * screen , GUI_RECT * rect , GUI_SURFACE ** surface , int num ) ;
int          gui_display_flip    ( GUI_SCREEN * screen ) ;   //�л���ʾ����
int          gui_display_colorkey( GUI_SCREEN * screen , int colorkey ) ;
int          gui_display_alpha   ( GUI_SCREEN * screen , U8 alpha0 , U8 alpha1 ) ;   //�޸�͸����
int          gui_display_show    ( GUI_SCREEN * screen , int show ) ; //�򿪹ر�


//��ȡ��ǰ��Backbuffer
GUI_SURFACE  * gui_display_surface( GUI_SCREEN * screen  ) ;
void           gui_set_clip_rect  ( GUI_SURFACE * src  , GUI_RECT *clip ) ;
void * gui_surf_offset ( GUI_SURFACE *surface, int x, int y );
GUI_RECT  gui_get_clip_rect  ( GUI_SURFACE * src );
void      gui_surface_hide ( GUI_SURFACE * surf , int visible );



//�����������--��ҪӲ������
int  gui_surface_copy ( GUI_SURFACE * src , GUI_SURFACE * dst , GUI_RECT * src_rect , GUI_RECT * dst_rect , int scale ) ; //֧��CLIP��ʹ����dst->clip
int  gui_surface_fill ( GUI_SURFACE * surface , GUI_RECT * rect  , U32 data ) ;
int  gui_surface_blend( GUI_SURFACE * src , GUI_SURFACE * dst , GUI_RECT * rect  , int alpha ) ;
int  gui_surface_clut ( GUI_SURFACE * src, GUI_SURFACE *dst , int type  );
int  gui_surface_resize ( GUI_SURFACE *src, GUI_SURFACE *dst , GUI_RECT *src_rect, GUI_RECT *dst_rect );
int  gui_surface_merge  ( GUI_SURFACE *src1 , GUI_SURFACE *src2, GUI_SURFACE *dst );
int  gui_surface_blt    ( GUI_SURFACE *dst, GUI_RECT * rect , GUI_SURFACE ** surface , int num ) ;
//����ͼ�κ���
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

#define  GUI_TEXT_LEFT     0           //�����
#define  GUI_TEXT_HCENTER  1           //ˮƽ���ж��롡����
#define  GUI_TEXT_RIGHT    2           //�Ҷ��롡����
#define  GUI_TEXT_WRAP     0x10        //�Զ�����
#define  GUI_TEXT_VCENTER  0x20        //��ֱ����

typedef void *  GUI_FONT   ;

//���ֱ���ת��iconv��ʹ���ڲ�����from/to����Ϊͬһ����ַ�������߱�֤�ڴ��㹻
int        gui_char_convert( char * from , char * to , int len , char * srcfmt , char *dstfmt ) ; 

//������ʾ
int        gui_font_init  ( char * local ) ;
int        gui_font_exit  ( void ) ;
int        gui_wide_char  ( U8 * str , U16 * out , int count ) ; //countΪת�����wide char����


GUI_FONT   gui_open_font  ( char * ascii , char *extra ,  int size , int style ) ;
int        gui_close_font ( GUI_FONT  font ) ;

//FONT�������ź���
int        gui_take_font      ( GUI_FONT font ) ;
int        gui_release_font   ( GUI_FONT font ) ;

int        gui_font_attribute ( GUI_FONT  font , int size , int style ) ;
int        gui_font_height( GUI_FONT font ) ;

//bcolorΪ0ʱ��ʾ͸����strΪUNICODE�ַ���
int  gui_draw_text (  GUI_SURFACE * surf , GUI_FONT  font , GUI_RECT * rect , 
                      U16 * str , int count , int mode , int fcolor , int bcolor ) ; 



//��ȡ��Сcount���ַ�����ʾ��С
//height = �ַ���ʾ�߶�
//return = �ַ���ʾ���
int gui_text_size( GUI_FONT  font , U16 * str , int count , int *height ) ;


//��ȡ��Сcount���ַ�����ʾλ��
//    y  = ���һ����ʾyλ��
//return = ���һ���ַ���ʾ��xλ��
int  gui_text_position (  GUI_FONT font , U16 * str , int count , int *y );

//��ȡ��Сcount���ַ��Ӵ���ʾλ�ã����س��Ȱ�width���зֶ�
//start  = ���һ�ε��ַ���ʼλ��
//    y  = ���һ���ַ���ʾ��Yλ��
//return = ���һ���ַ���ʾ��xλ��
int  gui_text_position_adv(  GUI_FONT font , U16 * str , int count , int width , int *y , int *start  );

void gui_create_overlay_gui ( int x, int y, int w, int h );
GUI_SURFACE *gui_get_overlay_surface( );
void gui_update_overlay_surface( );
void gui_close_overlay_surface();





//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//GUI OBJECT����
//
#define GUI_TEXT_TYPE    0
#define GUI_BUTTON_TYPE  1
#define GUI_SELECT_TYPE  2
#define GUI_DIGIT_TYPE   3
#define GUI_PAGE_TYPE    4
#define GUI_ICON_TYPE    5
#define GUI_SYMBOL_SELECT_TYPE   6  //��ICON��ͬ�������ж��ͼ��ѡ�񣬲����¼�
#define GUI_MENU_TYPE    7          //�˵���
#define GUI_ROTATE_TYPE  8          //������ת��ICON��ע��ʵ�ʲ�����ͼ����Ƕ��й�ϵ
#define GUI_SLIDE_TYPE   9
#define GUI_BORDER_TYPE  10         //�߿�

#define MAX_CHILD        32


//�ؼ������������
#define OBJ_BACK_NONE     0
#define OBJ_BACK_BLEND    1  //����1555��ʽ��1=COPY 0=OR
#define OBJ_BACK_COPY     2
#define OBJ_BACK_SCALE    3  //9���񷽷�
#define OBJ_BACK_SCALE_H  4  //ˮƽ���죬 0������

//GUI��ʾ���涨��
typedef struct _tagGUI_CONTEXT
{    
    GUI_SURFACE * surface ;    
    GUI_FONT      font    ;
}GUI_CONTEXT   ;


//ͼƬ��Դ���� -- ICON/SYMBOL
typedef struct
{
    int  total   ;  //ͬ����ͼƬ�����������ж��
    int  width   ;
    int  height  ;
    int  bpp     ;  // 
    U8   data[4] ;  //����������=w*h*bpp/8
}IMAGE_DATA ;


typedef struct _tagGUI_OBJECT 
{
    int type       ; //�ؼ�����
    int visible    ; //�ɼ�״̬    
    int message_id ; //��Ϣ
    int x      ;
    int y      ; 
    int width  ;
    int height ;
    int fcolor ;   //ǰ����ɫ
    int bcolor ;   //������ɫ 0 = ��ʾ͸��
    int font   ;   //�����С  
    int attribute ; //���ԣ�ÿ���ؼ����岻һ�� 
}GUI_OBJECT ;

#define STATIC_TEXT_NORMAL 0  //��ͨģʽ 
#define STATIC_TEXT_RECT   1  //��һ����������ʾ
//��̬�ı�
typedef struct 
{
    GUI_OBJECT  obj  ;    
    char   text[64]  ;
}GUI_STATIC_TEXT ;

//�����ؼ�
typedef struct 
{
    GUI_OBJECT  obj  ;            
    int    checked   ;     //�Ƿ�ѡ����
    char   text[32]  ;
    IMAGE_DATA * focus ;   //FOCUSͼƬ
}GUI_BUTTON ;


//ѡ��ؼ�
typedef struct
{
    GUI_OBJECT  obj  ; 
    
    int    current ;    
    int    count   ;    //�������
    char  *items[ MAX_CHILD ] ;    
    char   buffer[512] ;  //���󱣴滺��
    
    IMAGE_DATA * focus ;   //FOCUSͼƬ
}GUI_SELECT ;

//����ѡ��ؼ�
typedef struct
{
    GUI_OBJECT  obj  ; 
    
    int   min_data ;  //��Сֵ
    int   max_data ;  //���ֵ
    int   min_step ;  //ÿ�α仯����С����
    int   max_step ;  //��󲽳�
    
    int   current_data  ;  
    int   current_step  ; //��ǰ����
    int   current_dir   ; //��ǰ��������
    int   current_delay ; //�ȴ��ظ��������� 
}GUI_DIGIT ;



//ҳ��ؼ�
typedef struct
{
    GUI_OBJECT  obj  ;     
    int  current ;                
    int  count   ;  //�ӿؼ����� >= 1
    GUI_OBJECT  * children[MAX_CHILD] ;  
    IMAGE_DATA  * back  ;   //����ͼƬ     
}GUI_PAGE ;


#define OBJ_ICON_COPY   0
#define OBJ_ICON_MIX    1  //����1555��ʽ��1=COPY 0=Transparent
//ͼƬ�ؼ�
typedef struct
{
    GUI_OBJECT  obj    ;       
    IMAGE_DATA * icon  ;   //����ͼƬ                   
}GUI_ICON ;


//���ſؼ�
typedef struct
{
    GUI_OBJECT  obj   ; 
    int current   ;//��ǰѡ�����    
    IMAGE_DATA * icon ;   //����ͼƬ                 
}GUI_SYMBOL_SELECT ;


//�˵��ؼ�
typedef struct
{
    GUI_OBJECT  obj  ; 
    int  switch_id   ; //�л�ʱ�������¼�����    
    int  current ;                
    int  count   ;  //�ӿؼ����� >= 1
    GUI_POINT  children[MAX_CHILD] ;
    IMAGE_DATA * back  ;   //����ͼƬ 
    IMAGE_DATA * focus ;   //FOCUSͼƬ    
}GUI_MENU ;


//�ؼ������������
#define OBJ_ROTATE_COPY   0
#define OBJ_ROTATE_MIX    1  //����ARGB��ʽ��1=COPY 0=Transparent
//��תICON�ؼ�
typedef struct
{
    GUI_OBJECT    obj ;  
    int      rotate_x ;   
    int      rotate_y ;   
    int      rotate_angle ;   //�Ƕ�
    IMAGE_DATA * icon ;   //ͼƬ    
}GUI_ROTATE ;


//����ؼ��ؼ�
typedef struct
{
    GUI_OBJECT    obj ;
    int      back     ;
    int      min_data ;   
    int      max_data ;   
    int      current  ; 
    int      step     ;
    IMAGE_DATA * ind  ;   //ͼƬ    
}GUI_SLIDE ;

#define   OBJ_BORDER_NORMAL 0
#define   OBJ_BORDER_DASH   1

//�߿�ؼ��ؼ�
typedef struct
{
    GUI_OBJECT     obj ;
    int      thickness ; //�߿���
    IMAGE_DATA *  back ; //������ʾ
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


////ͼƬ��Դ�ⶨ��
#define  GET_ICON(a)   (IMAGE_DATA*)get_icon_data(a,strlen(a))

int  open_icon_lib ( char * name ) ;
void close_icon_lib( void ) ;
U8 * get_icon_data ( char * id , int len ) ;


////////////////////GUI OSD�¼�����/////////////////////
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

////RGB������ɫ����
#define  OSD_RGB_RED     0xFF0000 
#define  OSD_RGB_GREEN   0x00FF00
#define  OSD_RGB_BLUE    0x0000FF
#define  OSD_RGB_YELLOW  0xFFFF00  //��ɫ
#define  OSD_RGB_CYAN    0x00FFFF  //��ɫ
#define  OSD_RGB_CRIMSON 0xFF00FF  //
#define  OSD_RGB_BLACK   0x040404  //��ֹ͸ɫ
#define  OSD_RGB_WHITE   0xFFFFFF
#define  OSD_RGB_GRAY    0x808080

#define  OSD_ALPHA_COLOR(a,c) (((a)<<24)|(c))



////ע�⣺GUI��Ϣ�壬�̶����͵�APP_APPGUIģ��//////////
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

