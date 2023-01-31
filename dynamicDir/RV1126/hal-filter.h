
#ifndef EASYMEDIA_HAL_PRIVATE_H_
#define EASYMEDIA_HAL_PRIVATE_H_

#include <vector>
#include <mutex>

#include "buffer.h"
#include "filter.h"
#include "image.h"

#include "RockchipRga.h"

namespace easymedia{


/////////////////////////////////////////////////////////
//完成 
//     1.图像亮度对比度处理 (输入是YUV，对Y处理) 
//     2.格式转换 =>ARGB  
//     3.图像电子放大
//     4.OSD叠加  a. 直接绘制OSD对象(hook)   b. 区域方式
//     5.关闭、打开控制
class VpssFilter : public Filter 
{
    public:
        VpssFilter(const char *param);
        virtual ~VpssFilter() = default ;
        static const char *  GetFilterName() { return "vpss-filter"; }
        virtual int Process(std::shared_ptr<MediaBuffer> input,
                            std::shared_ptr<MediaBuffer> &output) override;
        virtual int IoCtrl (unsigned long int request , ...) override ;
                 
    private:
        std::vector<OSD_PLANE>  osdPlanes ;
        std::mutex        dataMutex    ;    
        
        ImageRect         sourceRect   ; //控制ZOOM的区域，定义源大小

        int               enableFlag   ;
                
        IMAGE_CALLBACK_INFO   imageCallback  ;   //对ARGB/YUV图像进行回调

        int     FindOsd   ( int id ) ;
        int     CreateOsd ( OSD_PLANE_INFO * osd ) ;
        int     GetOsdPtr ( OSD_PLANE_INFO * osd ) ;    
        int     UpdateOsd ( OSD_PLANE_INFO * osd ) ; //
        int     DestroyOsd( int id ) ;    

};


/////////////////////////////////////////////////////////
////ImageFilter只做帧率控制、裁剪和格式转换，为每个独立通道创建
class ImageFilter : public Filter 
{
    public:
        ImageFilter(const char *param);
        virtual ~ImageFilter() = default ;
        static const char *  GetFilterName() { return "image-filter"; }
        virtual int Process(std::shared_ptr<MediaBuffer> input,
                            std::shared_ptr<MediaBuffer> &output) override;
        virtual int IoCtrl (unsigned long int request , ...) override ;
        
     private :  
        int   rotation       ;
        
        ImageRect         sourceRect   ; //控制ZOOM的区域，定义源大小
          
        int   dropPattern    ;  //bit : 00110010110011 需要通过的帧
        int   maskFrameData  ;
        
};

class DummyFlow : public Flow
{
    public:
        DummyFlow(const char *param);
        virtual ~DummyFlow();
        static const char *GetFlowName() { return "dummy_flow"; }
};    

}

#endif

