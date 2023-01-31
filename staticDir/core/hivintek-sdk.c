#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "sys-service.h"
#include "sys-config.h"
#include "sys-core.h"
#include "sys-hal.h"
#include "sys-gui.h"
#include "sys-list.h"
#include "app-sysctrl.h"
#include "app-vinput.h"
#include "app-codec.h"
#define _USE_INSIDE_
#include "hivintek-sdk.h"


#define USER_GUI_BACKCOLOR     0

static GUI_SURFACE user_surface[ MAX_USER_GUI_IDX ] = {
    {0 , NULL , 0 , 0 , MAX_USER_GUI_WIDTH , MAX_USER_GUI_HEIGHT , MAX_USER_GUI_WIDTH*2 , 2 , 1 , { 0 , 0 , MAX_USER_GUI_WIDTH , MAX_USER_GUI_HEIGHT } , NULL } ,
    {0 , NULL , 0 , 0 , MAX_USER_GUI_WIDTH , MAX_USER_GUI_HEIGHT , MAX_USER_GUI_WIDTH*2 , 2 , 1 , { 0 , 0 , MAX_USER_GUI_WIDTH , MAX_USER_GUI_HEIGHT } , NULL } ,
    {0 , NULL , 0 , 0 , MAX_USER_GUI_WIDTH , MAX_USER_GUI_HEIGHT , MAX_USER_GUI_WIDTH*2 , 2 , 1 , { 0 , 0 , MAX_USER_GUI_WIDTH , MAX_USER_GUI_HEIGHT } , NULL } ,
    {0 , NULL , 0 , 0 , MAX_USER_GUI_WIDTH , MAX_USER_GUI_HEIGHT , MAX_USER_GUI_WIDTH*2 , 2 , 1 , { 0 , 0 , MAX_USER_GUI_WIDTH , MAX_USER_GUI_HEIGHT } , NULL } ,
    {0 , NULL , 0 , 0 , MAX_USER_GUI_WIDTH , MAX_USER_GUI_HEIGHT , MAX_USER_GUI_WIDTH*2 , 2 , 1 , { 0 , 0 , MAX_USER_GUI_WIDTH , MAX_USER_GUI_HEIGHT } , NULL } ,
};

static int user_font_size   = 24 ;
static int user_current_idx = 0  ;


int hivintek_gui_set_current( int idx )
{
    if( idx >= 0 && idx < MAX_USER_GUI_IDX )
        user_current_idx = idx ;
    return 0 ;
}

int hivintek_gui_get_current( void    )
{
    return user_current_idx ;
}


int hivintek_gui_size( int w , int h )
{
    if( user_surface[user_current_idx].vir_addr == NULL )
    {
        user_surface[user_current_idx].width  = w ;
        user_surface[user_current_idx].height = h ;
    }
    return 0;
}

int hivintek_gui_font_size( int dat )
{
    user_font_size = dat ;
    return 0;
}

int hivintek_gui_init( )
{
    GUI_SURFACE * surf = user_surface + user_current_idx ;
    
    VINPUT_USER_GUI_MSG * user_gui ;
    KERNEL_MESSAGE      msg ;
    
    int ret ;

    surf->stride = surf->width * surf->bpp ;
    surf->clip.width  = surf->width ;
    surf->clip.height = surf->height ;
    
    surf->vir_addr    = (char *)malloc( surf->width * surf->height * surf->bpp );    
    if ( surf->vir_addr == NULL )
        return 0;
    
    //Config it
    user_gui = ( VINPUT_USER_GUI_MSG * ) &msg ;
    core_fill_message(  (MESSAG_HEADER *)&msg , SYS_VINPUT, SYS_VINPUT, SYSVIN_SET_USER_GUI, sizeof(VINPUT_USER_GUI_MSG)  );
    user_gui->channel =   user_current_idx + VIN_OSD_USERGUI_EX1  ;
    user_gui->alpha   =   0 ;
    user_gui->x       =   surf->x ;
    user_gui->y       =   surf->y ;
    user_gui->width   =   surf->width ;
    user_gui->height  =   surf->height ;
    user_gui->bcolor  =   USER_GUI_BACKCOLOR ;
    user_gui->fcolor  =   0 ;    
    ret = core_send_message( &msg ); 
    
    //Enable it
    user_gui->enable  =   1 ;
    user_gui->head.command = SYSVIN_ENABLE_USER_GUI ;
    ret = core_send_message( &msg ); 
    
    SHM_CONFIG * shmcfg = (SHM_CONFIG *)GET_SYS_SHMCFG();
    user_font_size  = shmcfg_get_integer( shmcfg , "SYSTEM", "UserFont" , 24 ) ; 
       
    return ret;
}


int hivintek_gui_position( int x , int y )
{
    GUI_SURFACE * surf = user_surface + user_current_idx ;
    VINPUT_USER_GUI_MSG * user_gui ;
    KERNEL_MESSAGE      msg ;
    int ret ;
    
    user_gui = ( VINPUT_USER_GUI_MSG * ) &msg ;
    
    surf->x = x ;
    surf->y = y ;
        
    core_fill_message(  (MESSAG_HEADER *)&msg , SYS_VINPUT, SYS_VINPUT, SYSVIN_SET_USER_GUI, sizeof(VINPUT_USER_GUI_MSG)  );
    user_gui->channel =   user_current_idx + VIN_OSD_USERGUI_EX1  ;
    user_gui->alpha   =   0 ;
    user_gui->x       =   surf->x ;
    user_gui->y       =   surf->y ;
    user_gui->width   =   surf->width ;
    user_gui->height  =   surf->height ;
    user_gui->bcolor  =   USER_GUI_BACKCOLOR ;
    user_gui->fcolor  =   0 ;    
    ret = core_send_message( &msg );     
    
    return ret ;
}


int  hivintek_gui_clear( )
{
    GUI_SURFACE * surf = user_surface + user_current_idx ;
    
    if( surf->vir_addr )        
        memset( surf->vir_addr , USER_GUI_BACKCOLOR, surf->width * surf->height * surf->bpp );
    
    return 1;
}


int  hivintek_gui_draw_rect( int x , int y , int w , int h , int fill, int fcolor )
{
    GUI_SURFACE * surf = user_surface + user_current_idx ;
    
    if( surf->vir_addr == NULL )
        return 0 ;
        
    gui_draw_rect( surf , x, y, w, h, fill, fcolor );
    
    return 1 ;
    
}
int hivintek_gui_draw_text( char *text , int x, int y , int fcolor )
{
    GUI_SURFACE * surf = user_surface + user_current_idx ;
    
    if( surf->vir_addr == NULL )
        return 0 ;    
    
    int      ret ;
    U16     buf[128] ;
    GUI_FONT font ;
    GUI_RECT rect ;
       
    
    font      = (GUI_FONT)GET_SYS_FONT() ;  
    if ( font == NULL )
        return 0;
    
    ret = gui_wide_char( (U8*)text , buf , 128 ) ;
    rect.x = x ;
    rect.y = y ;
    rect.width  = ret * 2 * user_font_size ;
    rect.height = user_font_size ;

    
    //画出字符串    
    gui_take_font( font ) ;    
    gui_font_attribute( font , user_font_size , 0 ) ; 
    
    gui_draw_text( surf , font , &rect , buf , ret , GUI_TEXT_LEFT , fcolor , 0 ) ;  
                    
    gui_release_font( font ) ;
    
    
    return 1;
    
}


int hivintek_gui_draw_line ( int x0, int y0, int x1, int y1 , int fcolor )
{
    GUI_SURFACE * surf = user_surface + user_current_idx ;
    
    if( surf->vir_addr == NULL )
        return 0 ;   
    gui_draw_line( surf , x0, y0, x1, y1 , fcolor );
    
    return 1 ;
}

int hivintek_gui_draw_pixel( int x, int y, int fcolor )
{
    GUI_SURFACE * surf = user_surface + user_current_idx ;
    
    if( surf->vir_addr == NULL )
        return 0 ; 
        
    gui_draw_pixel( surf , x, y, fcolor );
    
    return 1;
}

int hivintek_gui_draw_circle ( int x, int y, int r, int fill, int fcolor )
{
    GUI_SURFACE * surf = user_surface + user_current_idx ;
    
    if( surf->vir_addr == NULL )
        return 0 ; 
        
        
    gui_draw_circle( surf , x, y, r, fill, fcolor ) ;
    
    return 1;
}


int  hivintek_gui_update( )
{
    GUI_SURFACE * surf = user_surface + user_current_idx ;
    
    VINPUT_USER_GUI_MSG * user_gui ;
    KERNEL_MESSAGE      msg ;
    int ret ;
        
    if( surf->vir_addr == NULL )
        return 0 ; 
        
    
    user_gui = ( VINPUT_USER_GUI_MSG * ) &msg ;    
    core_fill_message(  (MESSAG_HEADER *)&msg, SYS_VINPUT, SYS_VINPUT, SYSVIN_UPDATE_USER_GUI, sizeof(VINPUT_USER_GUI_MSG )  );
    user_gui->channel   =  user_current_idx + VIN_OSD_USERGUI_EX1  ;
    user_gui->width     =  surf->width  ;
    user_gui->height    =  surf->height ;
    user_gui->data      =  surf->vir_addr ;
    ret = core_send_message( &msg );  
    
    return ret ;
}


int hivintek_register_jpeg_callback( FUNC_HIVINTEK_CALLBACK cb, void *para )
{
    SYSENC_CALLBACK enc_cb ;
    int id ;

    
    //注册回调函数
    enc_cb.channel = -1        ;  //所有通道
    enc_cb.type    = FR_PICTURE ;  //jpeg图片数据   
    enc_cb.param   = para ;
    enc_cb.callback= (FRAME_CALLBACK)cb ;
    id = SYSENC_CALLBACK_REG( &enc_cb ) ;  
    
    
    return id ;
}

int hivintek_register_venc_callback( FUNC_HIVINTEK_CALLBACK cb, void *para )
{
    SYSENC_CALLBACK enc_cb ;
    int id ;

    
    //注册回调函数
    enc_cb.channel = 0        ;  //主码流
    enc_cb.type    = FR_VIDEO ;  //视频编码数据   
    enc_cb.param   = para ;
    enc_cb.callback= (FRAME_CALLBACK)cb ;
    id = SYSENC_CALLBACK_REG( &enc_cb ) ;  
    
    
    return id ;
}


int hivintek_register_frame_callback( FUNC_HIVINTEK_CALLBACK cb, void *para )
{
    SYSENC_CALLBACK enc_cb ;
    int id ;

    
    //注册回调函数
    enc_cb.channel = -1        ;  //所有通道
    enc_cb.type    = FR_YUV ;  //原始数据   
    enc_cb.param   = para ;
    enc_cb.callback= (FRAME_CALLBACK)cb ;
    id = SYSENC_CALLBACK_REG( &enc_cb ) ;  
    
    
    return id ;
}


int hivintek_register_video_callback( FUNC_HIVINTEK_CALLBACK cb, void *para )
{
    SYSVIN_CALLBACK vin_cb ;
    int id ;

    
    //注册回调函数
    vin_cb.channel = -1        ;  //所有通道
    vin_cb.type    = FR_YUV ;  //原始数据   
    vin_cb.param   = para ;
    vin_cb.callback= (VINPUT_CALLBACK)cb ;
    id = SYSVIN_CALLBACK_REG( &vin_cb ) ;  
    
    
    return id ;
}



int hivintek_lock_frame(HIVINTEK_FRAME * frame)
{
    if( frame->frame.type >= VIDEO_GRAY_FRAME )
    {
        SYSVIN_FR_ADDREF( (SYSENC_FRAME *)frame ) ;       
    }else{    
        SYSENC_FR_ADDREF( (SYSENC_FRAME *)frame ) ;      
    }
    return 1 ;
}


int hivintek_unlock_frame( HIVINTEK_FRAME *frame )
{
    if( frame->frame.type >= VIDEO_GRAY_FRAME )
    {
        SYSVIN_FR_SUBREF((SYSENC_FRAME *)frame) ;
    }else{
        SYSENC_FR_SUBREF((SYSENC_FRAME *)frame) ;
    }
    return 1;
}


HIVINTEK_MSG_Q *hivintek_create_msg_q (  int maxMsgLength  )
{
    HIVINTEK_MSG_Q *q;
    q = (HIVINTEK_MSG_Q *)msgQCreate( 32 , maxMsgLength, 0 ) ;
    
    return q;
}
int hivintek_send_message( HIVINTEK_MSG_Q *q, int len, char *buf )
{
    return send_message_nowait( q, len, buf );
}
int  hivintek_receive_message( HIVINTEK_MSG_Q *q, int len, char *buf )
{
    return receive_message( q, len, buf );
}



int hivintek_create_thread ( char *name, HIVINTEK_THREAD_FUNC func, void *para )
{
    return core_create_thread( name, (CORE_THREAD_FUNC)func, para );
}



int hivintek_register_service ( char *name, int id,  HIVINTEK_SERVICE *service )
{
    CORE_SERVICE *core = (CORE_SERVICE *)malloc ( sizeof(CORE_SERVICE) );
    
    if ( core == NULL )
        return 0 ;
        
    core->private_data   = service->private_data ;
    core->start          = NULL ;
    core->stop           = NULL ;
    core->service_message  = service->service_message ;
    core->service_command  = service->service_command ;
    core->id               = id ;
    core->description      = (char *)malloc( strlen(name) + 1 );
    strcpy( core->description , name );
    
    return register_service ( core ) ;
    
    
}

#define   APP_EVENT_MSG           ( APP_EVENT << 16 )
#define   EVENT_USER_ALARM        ( APP_EVENT_MSG + 8 )
int hivintek_alarm_message ( void *alarm , int len  )
{
    int ret ;
    KERNEL_MESSAGE msg ;
    
    core_fill_message ( (MESSAG_HEADER *)&msg, APP_USER1, APP_EVENT, EVENT_USER_ALARM, sizeof(MESSAG_HEADER) + len );
    memcpy( msg.data, (char *)alarm , len );
    
    ret = core_send_message ( &msg ) ;
    return ret ;
}




