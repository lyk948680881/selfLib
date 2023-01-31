
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdarg.h>

#include "sys-core.h"
#include "sys-service.h"
#include "sys-hal.h"


static HAL_DRIVER     * HAL_driver = NULL ;
static pthread_mutex_t  driver_mutex    ;


/////////////////////////////////////////////////////////
//     Function : HAL_init
//  Description : init internal data
//        Input :
//       Output :
int HAL_init( void )
{
    HAL_driver = NULL ;
    pthread_mutex_init( &driver_mutex  , NULL );  
        
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : HAL_exit
//  Description : exit internal data , 不会被调用到
//        Input :
//       Output :
int HAL_exit( void )
{    
    HAL_DRIVER  * ptr ;
    int i ;

    //free module list    
    ptr = HAL_driver ;
    while( ptr != NULL )
    {   
        for( i = 0 ; i < ptr->ports ; i++ )
            HAL_drv_exit( ptr , i ) ;
          
        ptr   = ptr->next ;        
    }
         
    pthread_mutex_destroy( &driver_mutex ) ;    
    HAL_driver = NULL  ;

    return 1 ;
}


/////////////////////////////////////////////////////////
//     Function : register_driver
//  Description : 
//        Input :
//       Output :
int  register_driver  ( HAL_DRIVER  * drv , int port , void * para )
{
    HAL_DRIVER  * ptr  ;

    if( port >= 0 )  //自动初始化  
    {    
        if( !HAL_drv_init( drv , port , para ) )
             return 0  ;
    }
             
    drv->next = NULL ;
    pthread_mutex_lock( &driver_mutex ) ;
    if( HAL_driver == NULL )
    {    
        HAL_driver = drv ;
    }else{
        ptr = HAL_driver ;
        while( ptr->next != NULL )
        {
            if( strcmp( ptr->name , drv->name ) == 0  ) //名字相同
            {
                //不需要增加了，直接退出
                pthread_mutex_unlock(&driver_mutex );   
                return 1 ;
            }    
            ptr = ptr->next ;
        }
        ptr->next  = drv ;  
    } 
    pthread_mutex_unlock(&driver_mutex );   
    
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : unregister_driver
//  Description : 
//        Input :
//       Output :
int  unregister_driver( const char * name  )
{
    HAL_DRIVER  * drv , *ptr ;
    int i ;
                 
    drv = find_driver( name ) ;
    if( drv == NULL )
        return 0 ;

    //clear all the ports now 
    for( i = 0 ; i < drv->ports ; i++ )
        HAL_drv_exit( drv , i ) ;

    pthread_mutex_lock( &driver_mutex ) ;
    ptr = HAL_driver ;
    if( HAL_driver == drv )
    {
        HAL_driver = drv->next ;
    }else{
        while( ptr->next != drv )
            ptr = ptr->next ;

        ptr->next = drv->next ;
    }
    pthread_mutex_unlock(&driver_mutex );        

    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : find_driver
//  Description : 
//        Input :
//       Output :
HAL_DRIVER * find_driver( const char * name )
{
    HAL_DRIVER  * drv ;
    
    if( HAL_driver == NULL )
        return NULL ;
    
    drv = HAL_driver ;
    while( drv )
    {
        if( strcmp( drv->name , name ) == 0  )
            break ;
        drv = drv->next ;
    }

    if( drv == NULL )
    {
        LOG_PRINTF("[%s] %s is not in the system" , __FUNCTION__ , name ) ;        
    }        
    
    return drv ;
}


//     Function : HAL_list_driver
//  Description :
//        Input :
//       Output :
int  HAL_list_driver      ( char *buf ) 
{
    HAL_DRIVER  *ptr ;    
    int ret ;
    
    if( HAL_driver == NULL )
        return 0 ;

    ret    = sprintf( buf , "Drivers : \n");
    ptr    = HAL_driver ;
    while( ptr != NULL )
    {        
        ret   += sprintf( buf + ret , "  %-16s %-16s %d \n" ,  ptr->name  , ptr->description , ptr->ports ) ;
        ptr    = ptr->next ;
    }

    return ret + 1 ;      
}


//     Function : HAL_drv_init
//  Description :
//        Input :
//       Output :
int    HAL_drv_init    ( HAL_DRIVER * drv , int port , void * lpara )
{
    if( drv->init )
        return drv->init( drv , port , lpara ) ;
    return 1 ;
}

//     Function : HAL_drv_exit
//  Description :
//        Input :
//       Output :       
int    HAL_drv_exit    ( HAL_DRIVER * drv , int port ) 
{
    if( drv->exit )
        return drv->exit(drv ,port) ;
    return 1 ;    
}

//     Function : HAL_drv_create
//  Description :
//        Input :
//       Output :     
int HAL_drv_create  ( HAL_DRIVER * drv , int port , int ch , int data , void * lpara )
{
    if( drv->create )
        return drv->create(drv,port,ch,data,lpara) ;
    return 1 ;     
}

//     Function : HAL_drv_destroy
//  Description :
//        Input :
//       Output : 
int    HAL_drv_destroy ( HAL_DRIVER * drv , int port , int ch , void * lpara )
{
    if( drv->destroy )
        return drv->destroy(drv,port,ch,lpara) ;
    return 1 ;      
}
//     Function : HAL_drv_ioctl
//  Description :
//        Input :
//       Output : 
int    HAL_drv_ioctl   ( HAL_DRIVER * drv , int port , int ch , int op   , int data , void * lpara )
{
    if( drv->ioctl )
        return drv->ioctl(drv,port,ch,op,data,lpara) ;
    return 1 ;      
}

//     Function : HAL_drv_bind
//  Description :
//        Input :
//       Output : 
int    HAL_drv_bind    ( HAL_DRIVER * drv , int port , int ch , int flag , HAL_OBJECT * obj )
{
    if( drv->bind )
        return drv->bind(drv,port,ch,flag,obj) ;
    return 1 ;     
}

//     Function : HAL_drv_start
//  Description :
//        Input :
//       Output : 
int    HAL_drv_start   ( HAL_DRIVER * drv , int port , int ch , int flag )
{
    if( drv->start )
        return drv->start(drv,port,ch,flag) ;
    return 1 ;      
}

//     Function : HAL_drv_send
//  Description :
//        Input :
//       Output : 
int    HAL_drv_send    ( HAL_DRIVER * drv , int port , int ch , int data , void * pdata , int flag )
{
    if( drv->send )
        return drv->send(drv,port,ch,data,pdata,flag) ;
    return 1 ;     
}


