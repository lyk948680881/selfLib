/****************************************************************************
  Copyright (C), 2011, Robert Fan

  Histroy  :
            1)  Created by fanshaohua   2011/08/10

*/

#ifndef __APP_SYSCTL__
#define __APP_SYSCTL__

#ifdef __cplusplus
    extern "C" {
#endif

#define  SYSTEM_CTRL             0         
#define  APP_SYSCTRL_MSG        ( SYSTEM_CTRL << 16 )

//Message for sysctrl
#define  SYSCTRL_OK_MSG                ( APP_SYSCTRL_MSG + 0 )
#define  SYSCTRL_FAIL_MSG              ( APP_SYSCTRL_MSG + 1 )
#define  SYSCTRL_GET_SHMCFG            ( APP_SYSCTRL_MSG + 2 )
#define  SYSCTRL_GET_FONT              ( APP_SYSCTRL_MSG + 3 )

#define  SYSCTRL_LOAD_MODULE           ( APP_SYSCTRL_MSG + 5 )
#define  SYSCTRL_UNLOAD_MODULE         ( APP_SYSCTRL_MSG + 6 )
#define  SYSCTRL_LIST_MODULE           ( APP_SYSCTRL_MSG + 7 )
#define  SYSCTRL_LIST_SERVICE          ( APP_SYSCTRL_MSG + 8 )
#define  SYSCTRL_LIST_DRIVER           ( APP_SYSCTRL_MSG + 9 )
#define  SYSCTRL_LIST_THREAD           ( APP_SYSCTRL_MSG + 10 )

#define  SYSCTRL_CTRL_LOGMSG           ( APP_SYSCTRL_MSG + 20 )

#define  GET_SYS_SHMCFG()   (void*)core_do_command( SYSTEM_CTRL , SYSCTRL_GET_SHMCFG , 0 , NULL , NULL )
#define  GET_SYS_FONT()     (void*)core_do_command( SYSTEM_CTRL , SYSCTRL_GET_FONT , 0 , NULL , NULL )

CORE_SERVICE * sysctrl_service( void ) ;


#ifdef __cplusplus
    }
#endif

#endif
