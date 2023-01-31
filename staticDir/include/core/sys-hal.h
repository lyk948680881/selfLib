/****************************************************************************
  Copyright (C), 2011, Robert Fan

  Histroy  :
            1)  Created by fanshaohua   2011/08/10
  Description :
           硬件/平台虚拟层
             a. 所有驱动有共同的接口函数
             b. 不同类型的驱动参数不一样
             
           驱动分为：
               输入 -- 视频/音频/...
               输出 -- 视频/音频/...
               编码 -- H264/JPEG/...
               解码 -- H264/JPEG/...  
               加速 -- 2D图形加速
*/

#ifndef  __SYS_HAL__
#define  __SYS_HAL__

#ifdef __cplusplus
    extern "C" {
#endif

#include "sys-core.h"

//通用的驱动接口定义，同类的驱动可以生成多个实例
typedef struct _tagHAL_DRIVER 
{
    //标准头部
    char     name [16] ;
    char     description[16] ;
    int      ports     ;
    
    int    ( * init   ) ( struct _tagHAL_DRIVER * drv , int port , void * lpara ) ;        
    int    ( * exit   ) ( struct _tagHAL_DRIVER * drv , int port ) ; 
       
    int    ( * create ) ( struct _tagHAL_DRIVER * drv , int port , int ch , int data , void * lpara ) ;    
    int    ( * destroy) ( struct _tagHAL_DRIVER * drv , int port , int ch , void * lpara ) ;       
    int    ( * ioctl  ) ( struct _tagHAL_DRIVER * drv , int port , int ch , int op   , int data , void * lpara ) ;    
    int    ( * bind   ) ( struct _tagHAL_DRIVER * drv , int port , int ch , int data , void * lpara ) ;    
    
    int    ( * start  ) ( struct _tagHAL_DRIVER * drv , int port , int ch , int flag ) ;
        
    int    ( * send   ) ( struct _tagHAL_DRIVER * drv , int port , int ch , int data , void * pdata , int flag ) ;
        
    struct _tagHAL_DRIVER  * next ;
    
    //驱动相关数据
    //TODO: ....
                         
}HAL_DRIVER ;

//通用的HAL object定义
typedef struct
{
    int    port       ;
    int    channel    ;
    HAL_DRIVER  * drv ;
}HAL_OBJECT ;

int  HAL_init  ( void ) ;
int  HAL_exit  ( void ) ;
int  register_driver  ( HAL_DRIVER * drv , int port , void * para ) ; //port =-1，不自动初始化端口
int  unregister_driver( const char *name ) ;
HAL_DRIVER * find_driver( const char * name ) ;
int  HAL_list_driver  ( char *buf  ) ;

//HAL DRV通用函数调用
int    HAL_drv_init    ( HAL_DRIVER * drv , int port , void * lpara ) ;        
int    HAL_drv_exit    ( HAL_DRIVER * drv , int port ) ;    
int    HAL_drv_create  ( HAL_DRIVER * drv , int port , int ch , int data , void * lpara ) ;    
int    HAL_drv_destroy ( HAL_DRIVER * drv , int port , int ch , void * lpara ) ;       
int    HAL_drv_ioctl   ( HAL_DRIVER * drv , int port , int ch , int op   , int data , void * lpara ) ;    
int    HAL_drv_bind    ( HAL_DRIVER * drv , int port , int ch , int flag , HAL_OBJECT * in ) ;    
int    HAL_drv_start   ( HAL_DRIVER * drv , int port , int ch , int flag ) ;    
int    HAL_drv_send    ( HAL_DRIVER * drv , int port , int ch , int data , void * pdata , int flag ) ;



////////////////////////////////////////////////////////////////////////////
//    以下定义硬件虚拟层的通用接口参数
////////////////////////////////////////////////////////////////////////////
//公共帧类型定义
#define  VIDEO_I_FRAME        0x00        //H264
#define  VIDEO_P_FRAME        0x01
#define  VIDEO_B_FRAME        0x02
#define  AUDIO_ADPCM_FRAME    0x03        //ADPCM
#define  AUDIO_G711_FRAME     0x04
#define  AUDIO_PCM_FRAME      0x05
#define  AUDIO_AAC_FRAME      0x09        //AAC
#define  VIDEO_I_H265         0x10        //H265
#define  VIDEO_P_H265         0x11     
#define  VIDEO_B_H265         0x12

#define  VIDEO_JPEG_FRAME     0x81
#define  VIDEO_RAW16_FRAME    0xA1
#define  VIDEO_TEMP_FRAME     0xA2        //带温度信息的帧
#define  VIDEO_GRAY_FRAME     0xA3
#define  VIDEO_YUV_VI_FRAME   0xA4
#define  VIDEO_YUV_VPSS_FRAME 0xA5


//公共ioctl op定义
#define  OP_GET_PRIVATE       -1    //获取内部参数 
#define  OP_CONFIG_CHANNEL     0    //动态配置通道参数
#define  OP_GET_FRAME          1    //获取原始数据帧，调用时如果CH=-1，表示当前端口下全部通道数据，
                                    //否则为指定通道；timeout单位毫秒，timout<0阻塞模式获取数据
#define  OP_RELEASE_FRAME      2    //释放原始数据帧
#define  OP_CONFIG_MASK        3    //设置屏蔽区域
#define  OP_ENABLE_MASK        4    //打开关闭屏蔽区域
#define  OP_CONFIG_OVERLAY     5    //设置叠加OSD
#define  OP_ENABLE_OVERLAY     6    //打开关闭叠加OSD
#define  OP_UPDATE_BITMAP      7    //更新图像
#define  OP_CONFIG_PORT_MASK   8    //设置端口屏蔽区域
#define  OP_ENABLE_PORT_MASK   9    //打开关闭端口屏蔽区域

#define  OP_MOTION_SET_PARA    10   //设置移动侦测参数
#define  OP_MOTION_GET_STATE   11   //获取移动侦测状态


#define  OP_CREATE_OVERLAY     12    //创建叠加osd
#define  OP_DESTROY_OVERLAY    13    //销毁叠加osd

#define  OP_GET_CANVAS_INFO    14   //获取画布
#define  OP_UPDATE_CANVAS      15   //更新画布

#define  OP_CTRL_VPSS_NR       16

#define  OP_CSC_BRIGHT         17
#define  OP_CSC_CONTRAST       18
#define  OP_ZOOM_PARA          19
#define  OP_DEINTERLACE        20

#define  OP_SEND_FRAME         22    //向通道发送帧数据

#define  OP_ALLOC_VB_FRAME     30    //创建一个VB帧

#define  OP_ISP_AUTO_SHUTTER   48
#define  OP_CTRL_DEFOG         50    //去雾
#define  OP_DEHAZE_DISABLE              52    //去雾关闭
#define  OP_DEHAZE_AUTO_MODE            53    //自动去雾

#define  OP_IMAGE_SHARPNESS             62    //图像锐度，提高清晰度

#define   MAX_VI_WIDTH        640 
#define   MAX_VI_HEIGHT       640
#define   MAX_IVE_WIDTH       (1920)
#define   MAX_IVE_HEIGHT      (1080)

#define   MAX_VPSS_OUT_WIDTH  MAX_IVE_WIDTH
#define   MAX_VPSS_OUT_HEIGHT MAX_IVE_HEIGHT
#define   MAX_VO_WIDTH        MAX_IVE_WIDTH
#define   MAX_VO_HEIGHT       MAX_IVE_HEIGHT


//私有ioctl op定义
#define  OP_GET_LUMA           0x1001    //获取通道亮度统计
#define  OP_SHOW_HIDE          0x1002    //显示隐藏通道
#define  OP_CHANNEL_SPEED      0x1003    //设置显示的速度
#define  OP_CHANNEL_PAUSE      0x1004    //冻结恢复
#define  OP_CHANNEL_STEP       0x1005    //步进
#define  OP_CHANNEL_ZOOM       0x1006    //ZOOM
#define  OP_SET_USER_PIC       0x1007    //设置用户插入的图片，用于无视频输入时显示
#define  OP_CTRL_USER_PIC      0x1008    //控制User图片是否显示
#define  OP_GET_VI_INTCNT      0x1009    //获取VI中断数
#define  OP_GET_VI_FOCUS       0x100a    
#define  OP_SET_VI_FOCUS       0x100b    

#define  OP_SET_GUI_FONT       0x1010    //设置OSD FONT
#define  OP_ADD_GUI_PAGE       0x1011    //添加OSD PAGE
#define  OP_DEL_GUI_PAGE       0x1012    //删除OSD PAGE
#define  OP_LOCK_GUI_OBJ       0x1013    //修改OSD OBJ属性之前使用
#define  OP_UNLOCK_GUI_OBJ     0x1014    //修改OSD OBJ属性之后使用，释放
#define  OP_REG_CALLBACK       0x1015    //为vpss的rgb32 video增加一个回调接口

//输入类型定义
#define  VIDEO_INPUT_BT656     0 
#define  VIDEO_INPUT_BT601     1
#define  VIDEO_INPUT_BT1120    2
#define  VIDEO_INPUT_DC        3
#define  VIDEO_INPUT_SC2310    4
#define  VIDEO_INPUT_IMX178    5
#define  VIDEO_INPUT_P2050     6
#define  VIDEO_INPUT_IMX415    7
#define  VIDEO_INPUT_IMX577    8
#define  VIDEO_INPUT_GC4653    9


//输入模式
#define  VIDEO_MODE_BYPASS     0
#define  VIDEO_MODE_SCALE      1


//输出类型
#define  VIDEO_OUTPUT_BT656    0
#define  VIDEO_OUTPUT_BT1120   1
#define  VIDEO_OUTPUT_VGA      2
#define  VIDEO_OUTPUT_CVBS     3
#define  VIDEO_OUTPUT_LCD      4
#define  VIDEO_OUTPUT_YPBPR    5

//视频格式
#define  VIDEO_PAL             0
#define  VIDEO_NTSC            1
#define  VIDEO_720P            2
#define  VIDEO_1080I           3
#define  VIDEO_1080P           4
#define  VIDEO_800x600         5
#define  VIDEO_1024x768        6
#define  VIDEO_1280x1024       7



#define  VENC_TYPE_H265        0
#define  VENC_TYPE_H264        1


#define  USB_VI_IDX_RAW         0
#define  USB_VI_IDX_GRAY        1
#define  USB_VI_IDX_YUV         2

#define  IMAGE_ARGB_FMT         0  //图像，ARGB/RGB
#define  IMAGE_NV12_FMT         1  //视频，YUV420SP

//视频屏蔽区域
typedef struct
{
    int enable ;
    int x ;
    int y ;
    int width  ;
    int height ;
    int color  ;
}VIDEO_MASK_CFG ;


//视频叠加区域
typedef struct
{
    int x ;
    int y ;
    int width  ;
    int height ;
    int color  ;
    int alpha  ;     
}VIDEO_OVERLAY_CFG ;

//视频OSD格式
typedef struct
{
    int  width  ;
    int  height ;
    int  format ;   //暂时不用
    char * data ;
}VIDEO_OSD_BITMAP ;

typedef struct
{
    unsigned int  phy_addr ;
    unsigned int  vir_addr;
    unsigned int  w ;
    unsigned int  h ;
    unsigned int stride ;
}VIDEO_OVERLAY_CANVAS ;


//视频输入端口配置
typedef struct
{
    int   input_type   ;       //输入类型
    int   input_mode   ;       //
    int   input_format ;       //PAL/NTSC
    int   data_width   ;       //数据位宽    
    int   max_channel  ;       //最大通道数
    int   frame_width ;
    int   frame_height ;
}LIVE_VIDEO_INPUT ;


//视频输出端口配置
typedef struct
{
    int   output_type   ;       //输入类型    
    int   output_format ;       //
    int   data_width    ;       //数据位宽    
    int   max_channel   ;       //最大通道数
}LIVE_VIDEO_OUTPUT ;


//通道属性
typedef struct
{
    int   x         ;         //画面大小
    int   y         ;
    int   width     ;
    int   height    ; 
    int   fps       ;         //帧率
    int   mode ;
    int   type;
}LIVE_VIDEO_CHANNEL ;

//ZOOM属性
typedef struct
{
    int   zoom      ;  //not used
    int   x         ;
    int   y         ;
    int   width     ;
    int   height    ;
}LIVE_VIDEO_ZOOM ;

//视频帧格式定义
typedef struct
{
    int port       ;   //端口号
    int channel    ;   //通道号
    int width      ;
    int height     ;
    int format     ;   //YUV时说明为422 420格式 暂不用    
    U64 pts        ;   //时间戳
    int reference  ;
    
    U32 phy_addr[3]     ;   //图像YUV的物理地址
    U32 vir_addr[3]     ;   //图像YUV的虚拟地址
    U32 stride [3]      ;   //    
    
    int platform[64];  //平台相关的内容信息,最大256个字节
}LIVE_VIDEO_FRAME ;


typedef struct
{
    int width  ;
    int height ;
    int format ;  //指定图像格式
}VIDEO_VPSS_PARAM  ;


//ive 通道属性
typedef struct
{
    int width ;
    int height ;
}VIDEO_IVE_PARA ;

typedef struct
{
    int fv1 ;
    int fv2 ;
}VIDEO_FOCUS_PARA ;

#define  OP_IVE_CTRL_OUTLINE          0x8000
#define  OP_IVE_ADD_ALPHA             0x8001
#define  OP_IVE_YUV_BLEND             0x8002
#define  OP_IVE_LUT                   0x8005
#define  OP_IVE_CTRL_BLIT             0x8006 //scale csc copy blend  with global alpha
#define  OP_IVE_MEM_FILL              0x8007
#define  OP_IVE_MEM_COPY              0x8008
#define  OP_IVE_U32_BUF               0x8009
#define  OP_IVE_GET_ONE_BUF           0x800A 
#define  OP_IVE_GET_OBJ_RECTS         0x8010



#define  IVE_OUTLINE_PORT             0


#define  MAX_IVE_U32_NUM             12
#define  MAX_IVE_RANDOM_NUM          32

typedef struct
{
    U32 phyAddr[3];
    U8 *virAddr[3];
    int w ;
    int h ;
    int stride[3] ; 
    int bpp       ;
    void *  ref   ; //指向内部的资源    
    U64 pts       ;   //时间戳
}VIDEO_MEM_INFO ;




////////////////////////////////////////////////////////////////////////////
///////2. 音频 Live Audio(PCM)
///////
//Live Audio对于HAL的实现框架
//      AudioInput = {  "AudioInput" , "" , NN ,
//                       init( drv , port , LIVE_AUDIO_PORT * attr ) ,       //初始化音频端口
//                       exit( drv , port ) ,              //退出
//                       NULL ,
//                       NULL ,
//                       ioctl ( ... ) ,                        //控制
//                               ioctl( drv , port ,ch , OP_GET_FRAME , timeout , LIVE_AUDIO_FRAME *v )
//                               ioctl( drv , port ,ch , OP_RELEASE_FRAME , 0 , LIVE_AUDIO_FRAME *v )
//                       NULL ,
//                       start ( drv , port ,ch , enable/disable ) ,            //通道控制
//                       NULL ,
//                       
//                       NN个数据
//                       {
//                          LIVE_AUDIO_PORT     attr ,             //内部数据
//                          ....
//                       }
//                    }
//
//      AudioOutput = {  "AudioOutput" , "" , NN ,
//                       init( drv , port , LIVE_AUDIO_PORT * attr ) ,       //初始化音频端口
//                       exit( drv , port ) ,              //退出
//                       NULL ,
//                       NULL ,
//                       NULL ,
//                       bind  ( drv , port , ch , bind/unbind , HAL_OBJECT * in) ,  //绑定到输入
//                       start ( drv , port , ch , enable/disable ) ,            //通道控制
//                       send  ( drv , port , ch , 0 , LIVE_AUDIO_FRAME *v , 0 ) , //向通道发送数据
//                       
//                       NN个数据
//                       {
//                          LIVE_AUDIO_PORT     attr ,             //内部数据
//                          ....
//                       }
//                    }
//

//私有ioctl op定义

#define  LIVE_AUDIO_LPCM    0
#define  LIVE_AUDIO_AAC     1
#define  LIVE_AUDIO_G711A   2
#define  LIVE_AUDIO_G711U   3

//音频端口配置
typedef struct
{
    int sample_rate   ;    //采用率
    int sample_width  ;    //位宽
    int sample_format ;    //音频格式 
    int sample_count  ;    //每包采样个数，整包大小=count*width/8
    int sample_channels ;  //通道数
}LIVE_AUDIO_PORT ;




////////////////////////////////////////////////////////////////////////////
///////3. 视频编解码 Video Codec
///////
//Video Codec对于HAL的实现框架
//      VideoEncoder = {  "XXXEnc" , "" , 1 ,
//                       init( drv , 0 , VIDEO_CODEC *enc ) ,       //初始化视频编码器
//                       exit( drv , 0 ) ,              //退出
//                       create(  drv , 0 , ch , 0 , VIDEO_ENCODER_CHANNEL * v ) , //创建视频编码通道
//                       destroy( drv , 0 , ch , NULL ) ,                 //销毁视频编码通道 
//                       ioctl ( ... ) ,                        //控制
//                               ioctl( drv , 0 ,ch , OP_GET_FRAME , timeout , AV_STREAM_FRAME *v )
//                               ioctl( drv , 0 ,ch , OP_RELEASE_FRAME , 0 , AV_STREAM_FRAME *v )
//                               ioctl( drv , 0 ,ch , OP_CHANNEL_SET_ATTR , 0 , VIDEO_ENCODER_CHANNEL *v )
//                               ioctl( drv , 0 ,ch , OP_CONFIG_OVERLAY , index , VIDEO_OVERLAY_CFG * o )    
//                               ioctl( drv , 0 ,ch , OP_ENABLE_OVERLAY , index , int  * flag ) 
//                               ioctl( drv , 0 ,ch , OP_UPDATE_BITMAP  , index , VIDEO_OSD_BITMAP * bitmap ) 
//                               ioctl( drv , 0 ,ch , OP_REQUEST_I_FRAME , 0 , 0 ) 
//                       bind  ( drv , 0 ,ch , bind/unbind , HAL_OBJECT * v) ,  //绑定到输入
//                       start ( drv , 0 ,ch , enable/disable ) ,            //通道控制
//                       send  ( drv , 0 ,ch , 0 , LIVE_VIDEO_FRAME *v , block/noblock ) , //向通道发送数据
//                       
//                       VIDEO_CODEC       attr ,             //内部数据
//                       VIDEO_ENCODER_CHANNEL  channel[..]
//                       ....
//                    }
//
//      VideoDecoder = {  "XXXDec" , "" , 1 ,
//                       init( drv , 0 , VIDEO_CODEC *enc ) ,       //初始化视频解码器
//                       exit( drv , 0 ) ,              //退出
//                       create(  drv , 0 ,ch , 0 , VIDEO_DECODER_CHANNEL * v ) , //创建视频解码通道
//                       destroy( drv , 0 ,ch , NULL ) ,                 //销毁视频解码通道 
//                       ioctl ( ... ) ,                        //控制
//                               ioctl( drv , 0 ,ch , OP_GET_FRAME , timeout , LIVE_VIDEO_FRAME *v )
//                               ioctl( drv , 0 ,ch , OP_RELEASE_FRAME , 0 , LIVE_VIDEO_FRAME *v )
//                       bind  ( drv ,0 , ch , bind/unbind , HAL_OBJECT * v) ,  //绑定到输入
//                       start ( drv ,0 , ch , enable/disable ) ,            //通道控制
//                       send  ( drv ,0 , ch , 0 , AV_STREAM_FRAME *v , block/noblock ) , //向通道发送数据
//                       
//                       VIDEO_CODEC       attr ,             //内部数据
//                       VIDEO_DECODER_CHANNEL  channel[..]
//                       ....
//                    }


//私有ioctl op定义
#define  OP_CHANNEL_SET_ATTR      0x2000    //设置编码器动态参数
#define  OP_REQUEST_I_FRAME       0x2001    //请求编码一个I帧
#define  OP_JPEG_SNAP_SHOT        0x2002    //让JPEG编一帧数据


//视频编解码器参数
typedef struct
{
    int input_type   ;   //输入图像格式
}VIDEO_CODEC ;

//视频编码通道参数
typedef struct
{    
    int width       ;    //宽度
    int height      ;    //高度
    int bitrate     ;    //码流大小 
    int quality     ;    //质量 1 ~ 5 ，越小越好
    int fps         ;    //帧率 
    int cbr         ;    //cbr/vbr
    int gop         ;    //I frame 间隔
    int venc_type   ;    // 0 : H265  1 : H264
}VIDEO_ENCODER_CHANNEL ;


//视频解码通道参数
typedef struct
{    
    int width       ;    //宽度
    int height      ;    //高度
}VIDEO_DECODER_CHANNEL ;



//压缩码流帧格式
typedef struct
{    
    int channel    ;   //通道号
    int type       ;   //帧类型
    U64 pts        ;   //时间戳
    int length     ;   //大小    
    int width      ;   //宽度
    int height     ;   //高度            
    int fps        ;   //帧率
    int sample     ;   //音频采样率  

    U32 phy_cp     ;   //为了hivintek-sdk中的定义位置不变,补齐一位
    void *ref      ;
    void *stream   ;   //stream地址如果非0的话    



    
    U32 phy_addr[3] ;  
    void * vir_addr[3] ; 
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
        
    //struct timeval catchtime;               
}AV_STREAM_FRAME ;


////////////////////////////////////////////////////////////////////////////
///////3. 音频编解码 Audio Codec
///////
//Audio Codec对于HAL的实现框架
//      AudioEncoder = {  "XXXEnc" , "" , 1 ,
//                       init( drv , 0 , AUDIO_CODEC *enc ) ,       //初始化视频编码器
//                       exit( drv , 0 ) ,              //退出
//                       create(  drv , 0 ,ch , 0 , AUDIO_CODEC_CHANNEL * v ) , //创建视频编码通道
//                       destroy( drv , 0 ,ch , NULL ) ,                 //销毁视频编码通道 
//                       ioctl ( ... ) ,                        //控制
//                               ioctl( drv , 0 ,ch , OP_GET_FRAME , 0 , AV_STREAM_FRAME *v )
//                               ioctl( drv , 0 , ch , OP_RELEASE_FRAME , 0 , AV_STREAM_FRAME *v )
//                       bind  ( drv , 0 ,ch , bind/unbind , HAL_OBJECT * v) ,  //绑定到输入
//                       start ( drv , 0 ,ch , enable/disable ) ,            //通道控制
//                       send  ( drv , 0 ,ch , 0 , LIVE_AUDIO_FRAME *v , block/noblock ) , //向通道发送数据
//                       
//                       AUDIO_CODEC     attr ,             //内部数据
//                       AUDIO_CODEC_CHANNEL  channel[..]
//                       ....
//                    }
//
//      AudioDecoder = {  "XXXDec" , "" , 1 ,
//                       init( drv , 0 , AUDIO_CODEC *enc ) ,       //初始化视频解码器
//                       exit( drv , 0 ) ,              //退出
//                       create(  drv , 0 ,ch , 0 , AUDIO_CODEC_CHANNEL * v ) , //创建视频解码通道
//                       destroy( drv , 0 ,ch , NULL ) ,                 //销毁视频解码通道 
//                       ioctl ( ... ) ,                        //控制
//                               ioctl( drv , 0 ,ch , OP_GET_FRAME , 0 , LIVE_AUDIO_FRAME *v )
//                               ioctl( drv , 0 ,ch , OP_RELEASE_FRAME , 0 , LIVE_AUDIO_FRAME *v )
//                       bind  ( drv , 0 ,ch , bind/unbind , HAL_OBJECT * v) ,  //绑定到输入
//                       start ( drv , 0 ,ch , enable/disable ) ,            //通道控制
//                       send  ( drv , 0 ,ch , 0 , AV_STREAM_FRAME *v , block/noblock ) , //向通道发送数据
//                       
//                       AUDIO_CODEC     attr ,             //内部数据
//                       AUDIO_CODEC_CHANNEL  channel[..]
//                       ....
//                    }

//私有ioctl op定义

//音频编解码器参数
typedef struct
{    
    int  max_channel   ;   //最大个数
}AUDIO_CODEC ;

//音频编解码通道参数
typedef struct
{   
    int mode         ;    //模式，不用 
    int bitrate      ;    //码流
    int sample       ;    //采用率
    int channels     ;    //1-单声道 2-立体声
}AUDIO_CODEC_CHANNEL ;


typedef struct {
    int enable ;
    int sensitivity ;
    int viChn ;
    int width ;
    int height ;
}HI_MOTION_INPUT ;




////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
///////4. 2D图像加速 2D ACCEL
///////
//2D ACCEL对于HAL的实现框架
//      HWAccel = {  "Accel2D" , "" , 1 ,
//                       init( drv , 0 , NULL ) ,       //初始
//                       exit( drv , 0 ) ,              //退出
//                       NULL ,
//                       NULL ,
//                       ioctl ( ... ) ,                        //控制
//                               ioctl( drv , 0 , 0 , ACCEL_SURF_BLT   , 0 , ACCEL_SURFACE_BLT *para )
//                               ioctl( drv , 0 , 0 , ACCEL_SURF_BLEND   , 0 , ACCEL_SURFACE_BLEND *para )
//                               ioctl( drv , 0 , 0 , ACCEL_SURF_COPY   , 0 , ACCEL_SURFACE_COPY *para )
//                               ioctl( drv , 0 , 0 , ACCEL_SURF_FILL   , 0 , ACCEL_SURFACE_FILL *para )
//                               ioctl( drv , 0 , 0 , ACCEL_FB_COLOKEY   , fd   , int * colorkey  )
//                               ioctl( drv , 0 , 0 , ACCEL_FB_ALPHA     , fd   , int * alpha  )
//                               ioctl( drv , 0 , 0 , ACCEL_FB_SHOW      , fd   , int * show/hide  )
//                       NULL ,
//                       NULL ,
//                       NULL ,
//                       ....
//                    }

//私有ioctl op定义
#define  ACCEL_SURF_BLT      0x3000        //在Surface之间Blt
#define  ACCEL_SURF_BLEND    0x3001        //
#define  ACCEL_SURF_COPY     0x3002
#define  ACCEL_SURF_FILL     0x3003
#define  ACCEL_SURF_CLUT     0x3004
#define  ACCEL_SURF_RESIZE   0x3005
#define  ACCEL_SURF_OPT      0x3006
#define  ACCEL_SURF_IR_COLORBAR  0x3007
#define  ACCEL_SURF_VI_COLORBAR  0x3008


#define  ACCEL_FB_COLOKEY    0x3010       //控制FrameBuffer设备接口
#define  ACCEL_FB_ALPHA      0x3011
#define  ACCEL_FB_SHOW       0x3012
#define  ACCEL_FB_COMPRESS   0x3013
#define  ACCEL_FB_CHALPHA    0x3014


#define  ACCEL_UPDATE_MEM    0x3020

#define  ACCEL_COLORBAR_IR_ALPHA       0
#define  ACCEL_COLORBAR_VI_ALPHA       1

 
//Surface Blt参数
typedef struct
{
    void **  src   ;   //源GUI_SURFACE
    int      count ;          //个数
    void  *  dst   ;   //目标GUI_SURFACE
    void  *  rect  ;   //目标区域GUI_RECT
}ACCEL_SURFACE_BLT ;

//Blend参数 
typedef struct
{
    void  *  src   ;   //源 GUI_SURFACE   
    void  *  dst   ;   //目标GUI_SURFACE
    void     *  rect  ;   //目标区域GUI_RECT
    int       alpha   ;
}ACCEL_SURFACE_BLEND ;

//Copy参数
typedef struct
{
    void  *  src   ;   //源GUI_SURFACE    
    void  *  dst   ;   //目标GUI_SURFACE
    void     *  s_rect  ;   //源区域GUI_RECT
    void     *  d_rect  ;   //目标区域GUI_RECT
    int       scale     ;  //是否缩放
}ACCEL_SURFACE_COPY ;


typedef struct 
{
    void * src ;
    void * dst ;
    void * s_rect ;
    void * d_rect ;
    void * src2  ;
    int    scale ;
}ACCEL_SURFACE_ROP ;




//Resize参数
typedef struct
{
    void  *  src   ;   //源GUI_SURFACE    
    void  *  dst   ;   //目标GUI_SURFACE
    void     *  s_rect  ;   //源区域GUI_RECT
    void     *  d_rect  ;   //目标区域GUI_RECT
}ACCEL_SURFACE_RESIZE ;

//Fill参数
typedef struct
{
    void     *  src   ;   //目标GUI_SURFACE    
    void     *  rect  ;   //目标区域GUI_RECT    
    int        color  ;   //
}ACCEL_SURFACE_FILL ;



//clut参数
typedef struct
{
    void * src ;    // 源GUI_SURFACE 
    void * dst ;    // 目标GUI_SURFACE
    void * rect ;   //区域 GUI_RECT
    int    type ;
}ACCEL_SURFACE_CLUT ;



typedef struct
{
    U32 phy_addr;
    void* vir_addr;
    int len ;
}ACCEL_UPDATE_MEM_INFO;







////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
///////5. ISP 图像处理
///////
//ISP对于HAL的实现框架
//      HWIsp = {  "VideoISP" , "" , 1 ,
//                       init( drv , 0 , NULL ) ,       //初始
//                       exit( drv , 0 ) ,              //退出
//                       NULL ,
//                       NULL ,
//                       ioctl ( ... ) ,                        //控制
//                               ioctl( drv , 0 , 0 , ISP_WRITE_REG   , reg , int * value )
//                               ioctl( drv , 0 , 0 , ISP_READ_REG    , reg , int * value )
//                               ioctl( drv , 0 , 0 , ISP_LOAD_CFG    , 0   , char * file )
//                               ioctl( drv , 0 , 0 , ISP_SAVE_CFG    , 0   , char * file )
//                       NULL ,
//                       NULL ,
//                       NULL ,
//                       ....
//                    }

//私有ioctl op定义
#define  ISP_WRITE_REG           0x4000
#define  ISP_READ_REG            0x4001
#define  ISP_LOAD_CFG            0x4002    //读入配置
#define  ISP_SAVE_CFG            0x4003    //保持配置

/*********************************************************
 *
 *   配置文件格式  REG:VALUE 
 */ 


#ifdef __cplusplus
    }
#endif

#endif

