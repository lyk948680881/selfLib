#ifndef __SYS_MESSAGE_H__
#define __SYS_MESSAGE_H__



//键盘消息定义
#define  SYS_KEY_MESSAGE                ( 0x3000 )
#define  KEYBOARD_UP                   (SYS_KEY_MESSAGE + 0)
#define  KEYBOARD_DOWN                 (SYS_KEY_MESSAGE + 1)
#define  KEYBOARD_RIGHT                (SYS_KEY_MESSAGE + 2)
#define  KEYBOARD_LEFT                 (SYS_KEY_MESSAGE + 3)
#define  KEYBOARD_M                    (SYS_KEY_MESSAGE + 4)
#define  KEYBOARD_F                    (SYS_KEY_MESSAGE + 5)
#define  KEYBOARD_S                    (SYS_KEY_MESSAGE + 6)
#define  KEYBOARD_C                    (SYS_KEY_MESSAGE + 7)
#define  KEYBOARD_AUX                  (SYS_KEY_MESSAGE + 8)
#define  KEYBOARD_AUX_U                (SYS_KEY_MESSAGE + 16)
#define  KEYBOARD_AUX_D                (SYS_KEY_MESSAGE + 17)
#define  KEYBOARD_AUX_R                (SYS_KEY_MESSAGE + 18)
#define  KEYBOARD_AUX_L                (SYS_KEY_MESSAGE + 19)
#define  KEYBOARD_AUX_M                (SYS_KEY_MESSAGE + 20)
#define  KEYBOARD_AUX_F                (SYS_KEY_MESSAGE + 21)
#define  KEYBOARD_AUX_S                (SYS_KEY_MESSAGE + 22)
#define  KEYBOARD_AUX_C                (SYS_KEY_MESSAGE + 23)
#define  KEYBOARD_CORE_M               (SYS_KEY_MESSAGE + 24)

#define  KEYBOARD_M_VL                 (SYS_KEY_MESSAGE + 25)
#define  KEYBOARD_M_IR                 (SYS_KEY_MESSAGE + 26)
#define  KEYBOARD_M_FUSIONA            (SYS_KEY_MESSAGE + 27)
#define  KEYBOARD_M_FUSIONB            (SYS_KEY_MESSAGE + 28)
#define  KEYBOARD_M_PIP                (SYS_KEY_MESSAGE + 29)
#define  KEYBOARD_M_PIPA               (SYS_KEY_MESSAGE + 30)
#define  KEYBOARD_M_PIPB               (SYS_KEY_MESSAGE + 31)
#define  KEYBOARD_SET_PORT             (SYS_KEY_MESSAGE + 32)
#define  KEYBOARD_CLEAR_PORT           (SYS_KEY_MESSAGE + 33)


//消息体定义 48 Bytes
typedef struct
{
    int  sender   ;      //发送模块
    int  receiver ;      //接受模块
    int  command  ;      //命令字
    int  length   ;      //内容长度
    int  pressed     ;
    int  slice;
}KEY_MESSAGE ;

#endif
