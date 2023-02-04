#ifndef __FRAME_H__
#define __FRAME_H__
/*ע�⣡�� �����ļ�ʱΪ�����ֽ���*/
#define  FRAME_SECONDS_OFFSET   946684800     //��19700101��20000101��ƫ������
#define  MAGIC_FLAG             0x33cccc33
/*ע�⣡������CPU�ֽ��򣬶�����Ҫת����*/
typedef struct
{
    int    magic    ;   //0x33cccc33
    int    type     ;
    int    length   ;
    int    width    ;   //��Ƶʱ��Ч
    int    height   ;   //��Ƶʱ��Ч
    int    fps      ;
    int    ch       ;
    int    seconds  ;
    int    ticks    ;  // 1 ��֮�ڵ�ticks����λ10ms    
}FRAME_HEADER ;

typedef struct
{
    int    magic    ;   //0x33cccc33
    int    type ;
    int    length ;
    int    width    ;   //��Ƶʱ��Ч
    int    height   ;   //��Ƶʱ��Ч
    int    fps      ;
    int    ch  ;
    int    seconds    ;
    int    ticks      ;  // 1 ��֮�ڵ�ticks����λ10ms  
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
