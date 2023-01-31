
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>


#include "sys-core.h"
#include "sys-service.h"
#include "sys-hal.h"
#include "app-syslog.h"
#include "app-sysctrl.h"


static CORE_SERVICE   * core_service = NULL ;
static pthread_mutex_t  service_mutex    ;

/////////////////////////////////////////////////////////
//     Function : service_init
//  Description : init internal data
//        Input :
//       Output :
int service_init( void )
{
    core_service = NULL ;
    pthread_mutex_init( &service_mutex  , NULL );  
    
    //load internal service into system       
    register_service( sysctrl_service() ) ;
	register_service( syslog_service()  ) ;   
	return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : service_exit
//  Description : exit internal data , 不会被调用到
//        Input :
//       Output :
int service_exit( void )
{    
    CORE_SERVICE  * ptr ;

    //free module list    
    ptr = core_service ;
    while( ptr != NULL )
    {
        if( ptr->stop )
            ptr->stop( ptr->private_data ) ;
        ptr  = ptr->next ;
    }
    
    pthread_mutex_destroy( &service_mutex ) ;    
    core_service = NULL  ;

    return 1 ;
}


/////////////////////////////////////////////////////////
//     Function : register_service
//  Description : 
//        Input :
//       Output :
int  register_service  ( CORE_SERVICE  * s )
{
    CORE_SERVICE  * ptr  ;

    if( s->start )
    {    
        //如果有启动函数，则调用 
        if( !s->start( s->private_data ) )
           return 0  ;
    }
 
         
    s->next = NULL ;
    pthread_mutex_lock( &service_mutex ) ;
    if( core_service == NULL )
    {    
        core_service = s ;
    }else{
        ptr = core_service ;
        while( ptr->next != NULL )
            ptr = ptr->next ;
        ptr->next  = s ;  
    } 
    pthread_mutex_unlock(&service_mutex );   
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : unregister_service
//  Description : 
//        Input :
//       Output :
int  unregister_service( int id  )
{
    CORE_SERVICE  * app , *ptr ;
    
    if( core_service == NULL )
        return 0 ;
    
    app = core_service ;
    while( app )
    {
        if( app->id == id )
            break ;
        app = app->next ;
    }

    if( app == NULL )
    {
        LOG_PRINTF("[%s] %d is not in the system" , __FUNCTION__ , id ) ;
        return 0 ;
    }    
    
    if( app->stop )
        app->stop( app->private_data ) ;
        
    pthread_mutex_lock( &service_mutex ) ;
    ptr = core_service ;
    if( core_service == app )
    {
        core_service = app->next ;
    }else{
        while( ptr->next != app )
            ptr = ptr->next ;

        ptr->next = app->next ;
    }
    
    pthread_mutex_unlock(&service_mutex );     

    return 1 ;
}



/////////////////////////////////////////////////////////
//     Function : service_send_message
//  Description : send message to one module
//        Input :
//       Output :
//发送消息，无返回数据
int  core_send_message( KERNEL_MESSAGE * msg )
{
    CORE_SERVICE * app , * call = NULL ;
    int ret ;

    if( core_service == NULL )
        return 0 ;

    ret = 0 ;
    pthread_mutex_lock( &service_mutex ) ;
    app = core_service ;
    while( app != NULL )
    {
        //printf("app id [%d], receiver = %d, service message %p \r\n", app->id, msg->receiver, app->service_message );
        if( app->id == msg->receiver && app->service_message != NULL )
        {
            call = app ;
            break ;
        }
        app = app->next ;

    }
    pthread_mutex_unlock( &service_mutex ) ;

    if( call )
        ret = call->service_message( call->private_data , msg->sender , msg ) ;
    return ret ;
}

/////////////////////////////////////////////////////////
//     Function : core_do_command
//  Description : ask other module to execute one command
//        Input :
//       Output :
//执行一个命令，可以有返回值
int  core_do_command  ( int id , int op , int len , void * ibuf , void * obuf )
{
    CORE_SERVICE * app , * call = NULL ;
    int ret ;

    if( core_service == NULL )
        return 0 ;

    ret = 0 ;
    pthread_mutex_lock( &service_mutex ) ;
    app = core_service ;
    while( app != NULL )
    {
        if( app->id == id && app->service_command != NULL )
        {
            call = app ;
            break ;
        }
        app = app->next ;
    }
    pthread_mutex_unlock( &service_mutex ) ;

    if( call )
        ret = call->service_command( call->private_data , op , len , ibuf , obuf ) ;
    return ret ;
}


//     Function : core_fill_message
//  Description :
//        Input :
//       Output :
void core_fill_message( MESSAG_HEADER * h , int sender , int receiver , int cmd , int total ) 
{
    h->sender    = sender   ;
    h->receiver  = receiver ;
    h->command   = cmd      ;
    h->length    = total - sizeof(MESSAG_HEADER) ;
}

//     Function : core_list_service
//  Description :
//        Input :
//       Output :
int  core_list_service      ( char *buf ) 
{
    CORE_SERVICE  *ptr ;    
    int ret ;
    
    if( core_service == NULL )
        return 0 ;

    ret    = sprintf( buf , "Services : \n");
    ptr    = core_service ;
    while( ptr != NULL )
    {        
        ret   += sprintf( buf + ret , "  %-8d %-32s Priv:%p\n" ,  ptr->id , ptr->description , ptr->private_data ) ;
        ptr    = ptr->next ;
    }

    return ret + 1 ;      
}

unsigned long gettickcount(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec*1000 + ts.tv_nsec/1000000);
}


