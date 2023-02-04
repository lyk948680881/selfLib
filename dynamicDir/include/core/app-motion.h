#ifndef __APP_MOTION_H__
#define __APP_MOTION_H__

#ifdef __cplusplus
    extern "C" {
#endif
#define  SYS_VI_MOTION                 4 

        
#define  SYS_MOTION_MSG                (SYS_VI_MOTION<<16)
#define  SYSMOTION_REG_CB              (SYS_MOTION_MSG + 1)
#define  SYSMOTION_UNREG_CB            (SYS_MOTION_MSG + 2)
#define  SYSMOTION_SET_PARA            (SYS_MOTION_MSG + 3)
    

typedef  int ( * STATE_CALLBACK ) ( void *priv, int ch, int state ) ;

typedef struct _tagSYSMOTION_CALLBACK
{
    int ch ;
    void* param ;
    STATE_CALLBACK callback ;
    struct _tagSYSMOTION_CALLBACK * next ; 
}SYSMOTION_CALLBACK ;

#define    MOTION_PORT_IRIMAGE      0
#define    MOTION_PORT_VISIABLE     1
#define    GET_MOTION_VI_CH(port)   (port == 0 ? 0 : 0 )


#ifdef __cplusplus
    }
#endif

#endif
