
#ifndef _HAL_PRIVATE_H_
#define _HAL_PRIVATE_H_

#include "buffer.h"
#include "image.h"

#define  INTERNAL_PIX_FMT      IMAGE_RGB888 //IMAGE_RGB888 //IMAGE_ARGB8888 

typedef struct
{
    int  valid  ;     
    std::shared_ptr<easymedia::ImageBuffer> buffer ; 
}RV1126_HAL_BUFFER ;


typedef struct
{        
    std::shared_ptr<easymedia::Flow> flow ; 
}RV1126_HAL_PRIV ;

typedef int  ( * ARGB_OBJECT_OPERATE ) ( void * addr , int width , int height , int bpp , int stride , void * para ) ;
typedef int  ( * YUYV_VIDEO_OPERATE  ) ( void * addr , int width , int height , int bpp , int stride , void * para ) ;
typedef int  ( * ARGB_VIDEO_OPERATE  ) ( void * video , void * para ) ;


////
#define  OSD_OP_CREATE     20000 
#define  OSD_OP_GETPTR     20001
#define  OSD_OP_UPDATE     20002
#define  OSD_OP_REMOVE     20003
#define  OSD_OP_START      20004
#define  OSD_OP_HALT       20005

#define  OSD_OP_CALLBACK   20010

#define  IMG_OP_ZOOM       20020


#define  OSD_PLANE_BPP     4
typedef struct
{
    int    id     ;    
    int    enable ;    
    int    x      ;
    int    y      ;
    int    width  ;
    int    height ;
    int    alpha  ;

    int  * data   ;   
}OSD_PLANE_INFO ;

typedef struct
{
    int  x ;
    int  y ;
    int  width ;
    int  height;
}IMAGE_ZOOM_INFO ;


typedef struct
{
    ARGB_OBJECT_OPERATE  argb  ;
    YUYV_VIDEO_OPERATE   yuyv  ;
    ARGB_VIDEO_OPERATE   video ;
    
    void  * para ;
}IMAGE_CALLBACK_INFO ;


typedef struct
{
    OSD_PLANE_INFO info ;    
    std::shared_ptr<easymedia::ImageBuffer> buffer ;
}OSD_PLANE ;


#define  VO_OP_SET_PATTERN     20000
#define  VO_OP_GET_PATTERN     20001


////GLOBAL TOOLS FOR RGA
int    VpssRgaBlit( std::shared_ptr<easymedia::ImageBuffer> src, std::shared_ptr<easymedia::ImageBuffer> dst,
                    ImageRect *src_rect = nullptr, ImageRect *dst_rect = nullptr, int rotation = 0 ,  int blend = 0);
                     
int    VpssRgbFill( std::shared_ptr<easymedia::ImageBuffer> dst , ImageRect *rect , int color = 0 ) ;
int Y16CovertNV12(std::shared_ptr<easymedia::ImageBuffer> src, std::shared_ptr<easymedia::ImageBuffer> dst);


#endif

