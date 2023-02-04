#ifndef _HIVINTEK_SDK_
#define _HIVINTEK_SDK_

/* 用户 gui 宽度 */
#define MAX_USER_GUI_WIDTH     720
/* 用户 gui 高度 */
#define MAX_USER_GUI_HEIGHT    576

//压缩码流帧格式
typedef struct
{    
    int channel    ;   //通道号
    int type       ;   //帧类型
    unsigned long long int  pts        ;   //时间戳
    int length     ;   //大小    
    int width      ;   //宽度
    int height     ;   //高度            
    int fps        ;   //帧率
    int sample     ;   //音频采样率  ，忽略
    
    int phy_cp     ;   // 物理地址，用户忽略
    void *ref      ;   // 保留项，  用户忽略    
    void * stream  ;   //stream地址如果非0的话
    
    
    int phy_addr[3] ;  
    int vir_addr[3] ; 
    int stride[3]   ; 
        
    int rec_stat   ;   //灰度数据从usb过来，会带来rec按键和pwm rec的状态
    int snap_count ;   //灰度数据从usb过来，会带来snap按键次数和pwm snap 次数的累加
    
    //测温数据相关
    int nuc_back ;
    int K_para ;
    int M_para ;
    int C_para ;
    int vtemp ;
    char * mtlib ;
        
}FRAME_INFO ;


///////////////////////////////////////////////////////////////////
//回调出去的帧类型，注意使用方法
typedef struct _tagHIVINTEK_FR
{
    //内部计数
    int            reference ;       
    FRAME_INFO     frame  ;    
    struct _tagHIVINTEK_FR * next;   //链表用
}HIVINTEK_FRAME ;

//回调函数类型定义
typedef  int ( * FUNC_HIVINTEK_CALLBACK ) ( HIVINTEK_FRAME * fr ,  void * para ) ;

#define MAX_USER_GUI_IDX    5 //最大画布数

/* 设置OSD字体大小，影响后续的文本大小  */
int hivintek_gui_font_size( int size ) ;

/*  设置当前的操作画布：后续的画布相关动作均作用在当前画布上 ，缺省画布idx=0 */
int hivintek_gui_set_current( int idx ) ;
int hivintek_gui_get_current( void    ) ; //获取当前的画布

/* 设置OSD画布大小 ，缺省为720*576，在xxx_gui_init之前调用有效 ，之后无效 */
int hivintek_gui_size( int w , int h );

/* gui 初始化函数 ， 使用以下函数之前一定要调用该函数 */
int hivintek_gui_init( );

/* 清除 gui上的所有内容 */
int hivintek_gui_clear( );

/*设置OSD画布位置 ，缺省为(0,0)，可以动态调整*/
int hivintek_gui_position( int x , int y ) ;


/* 画框函数
   参数
   x ， y : 左上点坐标
   w ， h : 框的宽度和高度
   fill   : 框内是否填充，0: 不填充 1 : 填充
   fcolor : rgb1555类型的颜色
*/
int hivintek_gui_draw_rect( int x , int y, int w, int h,  int fill, int fcolor );

/* 画字符串函数
   参数
   text   : 要写的字符，不要超过32个
   x ， y : 左上点坐标
   fcolor : rgb1555类型的颜色
*/
int hivintek_gui_draw_text( char *text , int x, int y , int fcolor);

/* 画圆函数
   参数
   x ， y : 圆心坐标
   r : 圆的半径
   fill   : 框内是否填充，0: 不填充 1 : 填充
   fcolor : rgb1555类型的颜色
*/
int hivintek_gui_draw_circle ( int x, int y, int r, int fill, int fcolor );



/* 画函线数
   参数
   x0 ， y0 : 直线起点坐标
   x1 ,   y1 : 直线终点坐标
   fcolor : rgb1555类型的颜色
*/
int hivintek_gui_draw_line ( int x0, int y0, int x1, int y1 , int fcolor );

/* 画点
   参数
   x ， y : 点的坐标
   fcolor : rgb1555类型的颜色
*/
int hivintek_gui_draw_pixel ( int x, int y, int fcolor );

/*
   将准备好的gui图片放到界面上显示
*/
int hivintek_gui_update( );

/*
   注册一个回调函数，底层有编码数据帧时会调用此回调函数
   参数
   cb  回调函数，回调函数中最好不要占用时间过久，以免影响取数
   para 要传给回调函数的参数包
*/
int hivintek_register_venc_callback( FUNC_HIVINTEK_CALLBACK cb, void *para );

/*
   注册一个回调函数，底层有jpeg图片时会调用此回调函数
   参数
   cb  回调函数，回调函数中最好不要占用时间过久，以免影响取数
   para 要传给回调函数的参数包
*/
int hivintek_register_jpeg_callback( FUNC_HIVINTEK_CALLBACK cb, void *para );

/*
   注册一个回调函数，底层有原始灰度数据时会调用此回调函数
   参数
   cb  回调函数，回调函数中最好不要占用时间过久，以免影响取数
   para 要传给回调函数的参数包
*/
int hivintek_register_frame_callback( FUNC_HIVINTEK_CALLBACK cb, void *para );


/*
   注册一个回调函数，底层有灰度视频数据时会调用此回调函数
   参数
   cb  回调函数，回调函数中最好不要占用时间过久，以免影响取数
   para 要传给回调函数的参数包
*/
int hivintek_register_video_callback( FUNC_HIVINTEK_CALLBACK cb, void *para );

/*
  锁定数据帧，以便处理程序处理
*/
int hivintek_lock_frame( HIVINTEK_FRAME *frame );

/*
  解锁数据帧，以便处理程序处理
*/

int hivintek_unlock_frame( HIVINTEK_FRAME *frame );



#ifndef _USE_INSIDE_
/*模块间消息结构——头部定义*/
typedef struct
{
    int  sender   ;      //发送模块
    int  receiver ;      //接受模块
    int  command  ;      //命令字
    int  length   ;      //内容长度
}MESSAG_HEADER2 ;

typedef struct
{
    int  sender   ;      //发送模块
    int  receiver ;      //接受模块
    int  command  ;      //命令字
    int  length   ;      //内容长度
    char data[128] ;     //内容，不一定为128字节
}KERNEL_MESSAGE2   ;


#endif

//消息队列句柄
typedef void *   HIVINTEK_MSG_Q;
//创建消息队列
HIVINTEK_MSG_Q *hivintek_create_msg_q ( int maxMsgLen );

/**********************************
    发送消息到消息队列
    参数
    q : 队列句柄
    len : 要发送的消息长度
    msg : 要发送的消息
***********************************/
int             hivintek_send_message( HIVINTEK_MSG_Q *q, int len, char *msg );

/**********************************
    从消息队列接受消息
    参数
    q : 队列句柄
    len : 要接受的消息最大长度
    msg : 接受到的消息
***********************************/
int             hivintek_receive_message( HIVINTEK_MSG_Q *q, int len, char *msg );

//线程函数定义
typedef  void *( * HIVINTEK_THREAD_FUNC )( void* )  ;
//启动一个线程
int hivintek_create_thread ( char *name, HIVINTEK_THREAD_FUNC func, void *para );



//方便内部模块直接传送数据
typedef struct _tagHIVINTEK_SERVICE
{
    int ( * service_message ) ( void * priv , int sender , void * msg ) ;
    int ( * service_command ) ( void * priv , int op , int len , void * ibuf , void * obuf ) ;
            
    void * private_data ;       //私有变量
    
}HIVINTEK_SERVICE ;


//将服务函数私有数据挂到系统中
//返回 0 : 失败 ， 1 :成功
int hivintek_register_service ( char *name, int id, HIVINTEK_SERVICE *service );

/**********************************
    发送报警信息到消息管理服务器，让服务器再发送给网络
    返回
    0 : 失败
    1 : 成功
    参数
    alarm : 报警内容，用户自行定义
    len : 要发送的报警内容的长度，最长128字节
***********************************/
int hivintek_alarm_message ( void *alarm , int len  );



#endif
