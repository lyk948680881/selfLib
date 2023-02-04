#ifndef __FRAME_H__
#define __FRAME_H__
/*注意！！ 保存文件时为网络字节序　*/
#define  FRAME_SECONDS_OFFSET   946684800     //从19700101到20000101的偏移秒数
#define  MAGIC_FLAG             0x33cccc33
/*注意！！！　CPU字节序，对外需要转换　*/
typedef struct
{
    int    magic    ;   //0x33cccc33
    int    type     ;
    int    length   ;
    int    width    ;   //视频时有效
    int    height   ;   //视频时有效
    int    fps      ;
    int    ch       ;
    int    seconds  ;
    int    ticks    ;  // 1 秒之内的ticks，单位10ms    
}FRAME_HEADER ;

typedef struct
{
    int    magic    ;   //0x33cccc33
    int    type ;
    int    length ;
    int    width    ;   //视频时有效
    int    height   ;   //视频时有效
    int    fps      ;
    int    ch  ;
    int    seconds    ;
    int    ticks      ;  // 1 秒之内的ticks，单位10ms  
    int    vtemp ;
    int    nuc_back ;    
    int    K_para ;
    int    M_para ;
    int    C_para ;
    int    temp_offset ;
    int    ambient ;
    int    emiss ;
    int    dummy[40];

    char mtlib[64] ;
}TEMP_FRAME_HEADER ;


#endif
