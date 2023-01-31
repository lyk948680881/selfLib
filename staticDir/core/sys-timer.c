/****************************************************************************
   
      File : timer.c
Description: 用户定时器，以tick为单位，最多支持2048个


****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include "sys-core.h"
#include "sys-service.h"

#define  MAX_USER_TIMER        2048
#define  TIMER_ACTIVE        0
#define  TIMER_PAUSE         1

typedef struct _tagTIMER
{
    int       timer_id       ;    
    int       message        ;
    int       receiver       ;
    TIMER_CALLBACK   callback;
    void            *data    ;
    
    int       state          ;
    int       old_ticks      ;
    int       timer_type     ;
    int       ticks          ;
    
    

    struct  _tagTIMER  * next ;
    
}USER_TIMER ;

void * core_timer_loop( void * arg ) ;

static USER_TIMER    timer_data[ MAX_USER_TIMER ] ;
static USER_TIMER    * available_timer , * active_timer  ;
static pthread_mutex_t   timer_mutex ;


//  Function  : core_timer_init
//Description : init the timer data and timer queque
//              < 0 , fail
int  core_timer_init ( void )
{
    int i ;
    LOG_PRINTF("%s %d", __FUNCTION__ , __LINE__ );
    //init timer queque
    for( i = 0 ; i < ( MAX_USER_TIMER - 1 ) ; i++ )
    {
        timer_data[i].timer_id   =  MAX_USER_TIMER + i ;
        timer_data[i].next       =  &timer_data[i+1]   ;
    }

    timer_data[ MAX_USER_TIMER - 1 ].timer_id =  MAX_USER_TIMER + MAX_USER_TIMER - 1 ;
    timer_data[ MAX_USER_TIMER - 1 ].next     =  NULL ;

    available_timer  = timer_data  ;
    active_timer     = NULL        ;

    /* +++++ 创建信号量 +++++++  */
    pthread_mutex_init( &timer_mutex , NULL );  
    LOG_PRINTF("%s %d", __FUNCTION__ , __LINE__ );
    core_create_thread( "Core Timer" , core_timer_loop , NULL ) ;    
    return 1 ;
}


//   Function  : core_timer_remove
// Description : remove timer from active queque
//               this function run under semphone  protect
void core_timer_remove( USER_TIMER * timer )
{
    USER_TIMER * ptr  ;
    int find ;

    ptr  = active_timer ;
    find = 0 ;

    // remove from the active queque
    if( active_timer == timer )
    {
        active_timer = timer->next ;
        find         = 1 ;
    }else{
        while( ptr != NULL )
        {
           if( ptr->next == timer )
           {
                ptr->next = timer->next ;
                find      = 1 ;
                break;
           }
           ptr = ptr->next ;
        }
    }

    if( find == 0 )
        return ;

    // move to end of available queque
    timer->next = NULL     ;
    ptr         = available_timer ;    
    if( available_timer == NULL )
    {
        available_timer = timer ;
    }else{
        while( ptr->next != NULL )
            ptr  = ptr->next  ;            
        ptr->next        = timer ;        
    }
    
    timer->timer_id += MAX_USER_TIMER ;
    if( timer->timer_id  < MAX_USER_TIMER )
        timer->timer_id += MAX_USER_TIMER ;

}

//   Function  : core_timer_start
// Description : start a user timer
//               message ---  when timer out , this message will be sent out
//               ticks   ---  how long will the timer be
//               type    ---  normal or period
//     returns : 0  , timer start fail
//               otherwise , success
int  core_timer_start( int message , int ticks , int type , void *pdata , TIMER_CALLBACK func )
{
    USER_TIMER * timer  ;



    if( available_timer == NULL )        /*no timer space left*/
        return 0 ;

    pthread_mutex_lock( &timer_mutex ) ;

    timer            =  available_timer ;
    available_timer  =  available_timer->next ;

    timer->callback       = func    ;
    timer->data           = pdata   ;
    timer->message        = message ;
    timer->state          = TIMER_ACTIVE ;
    timer->old_ticks      = ticks   ;
    timer->timer_type     = type    ;
    timer->ticks          = ticks   ;

    timer->next  = active_timer ;
    active_timer = timer        ;

    pthread_mutex_unlock(&timer_mutex);

    return timer->timer_id ;
}

//   Function  : core_timer_message
// Description : start a user timer which will send msg to receiver module
//               message ---  when timer out , this message will be sent out
//               ticks   ---  how long will the timer be
//               type    ---  normal or period
//     returns : 0  , timer start fail
//               otherwise , success
int  core_timer_message( int receiver , int message , int ticks , int type )
{
    USER_TIMER * timer  ;

    if( available_timer == NULL )        /*no timer space left*/
        return 0 ;

    pthread_mutex_lock( &timer_mutex ) ;

    timer            =  available_timer ;
    available_timer  =  available_timer->next ;

    timer->callback       = NULL    ;
    timer->data           = NULL    ;
    timer->receiver       = receiver;
    timer->message        = message ;
    timer->state          = TIMER_ACTIVE ;
    timer->old_ticks      = ticks   ;
    timer->timer_type     = type    ;
    timer->ticks          = ticks   ;

    timer->next  = active_timer ;
    active_timer = timer        ;

    pthread_mutex_unlock(&timer_mutex);

    return timer->timer_id ;
}


//  Function  : core_timer_stop
//Description : delete a timer
void core_timer_stop ( int  timer_id )
{
    USER_TIMER * timer  ;

    if( timer_id == 0 )
        return ;

    pthread_mutex_lock( &timer_mutex ) ;

    timer =  &timer_data[ timer_id % MAX_USER_TIMER ] ;
    core_timer_remove( timer ) ;

    pthread_mutex_unlock(&timer_mutex);
}

//  Function : core_timer_get_ticks
//Decritpiton: get time left ticks
int core_timer_get_ticks( int timer_id )
{
    USER_TIMER * timer  ;

    if( timer_id == 0 )
        return 0 ;

    timer =  &timer_data[ timer_id % MAX_USER_TIMER ] ;

    return timer->ticks   ;
}

//  Function : core_timer_set
//Decritpiton: set time  ticks
void core_timer_set  ( int timer_id , int ticks )
{
    USER_TIMER * timer ;

    if( timer_id  ==  0 )
        return ;

    pthread_mutex_lock( &timer_mutex ) ;

    timer =  &timer_data[ timer_id % MAX_USER_TIMER ] ;
    timer->old_ticks = ticks ;
    timer->ticks     = ticks ;

    pthread_mutex_unlock(&timer_mutex);    
}

//  Function : core_timer_reset
//Description: reset a timer
void core_timer_reset( int  timer_id )
{
    USER_TIMER * timer ;

    if( timer_id  ==  0 )
        return ;

    pthread_mutex_lock( &timer_mutex ) ;

    timer =  &timer_data[ timer_id % MAX_USER_TIMER ] ;
    timer->ticks = timer->old_ticks ;

    pthread_mutex_unlock(&timer_mutex);

}

//  Function : core_timer_pause
//Description: pause current timer
void core_timer_pause( int  timer_id , int flag )
{
    USER_TIMER * timer ;

    if( timer_id  ==  0 )
        return ;

    pthread_mutex_lock( &timer_mutex ) ;

    timer =  &timer_data[ timer_id % MAX_USER_TIMER ] ;
    timer->state = flag ? TIMER_PAUSE : TIMER_ACTIVE ;

    pthread_mutex_unlock(&timer_mutex);
}

//  Function : core_timer_loop
//Description: timer task entry
void * core_timer_loop( void * arg )
{
    USER_TIMER * timer , * ptr ;
    int        interval , delay  ;    
    struct timeval tvSelect;

    interval  =  10 ;
    
    while( 1 )
    {
        //usleep( interval *1000*10) ;     /* usleep有误差 */
        tvSelect.tv_sec = 0;
        tvSelect.tv_usec = interval * 10000;
        select(0, NULL, NULL, NULL, &tvSelect);/* delay task for some ticks */
        delay = interval       ;
        interval = 10          ;

        //start to do timer function
        pthread_mutex_lock( &timer_mutex ) ;

        timer = active_timer ;
        while( timer != NULL )
        {
            if( timer->state == TIMER_ACTIVE )
                timer->ticks  -= delay  ;
            ptr  = timer->next ;

            if( timer->ticks <= 0 )
            {
                if( timer->callback )  //run callback
                    timer->callback( timer->message , timer->data ) ;
                else {
                    MESSAG_HEADER  msg  ;
                    msg.command  = timer->message  ;
                    msg.receiver = timer->receiver ;
                    msg.sender   = -1              ;
                    msg.length   = 0               ;
                    core_send_message((KERNEL_MESSAGE *)&msg) ;
                }
                if( timer->timer_type == TIMER_PERIOD )
                    timer->ticks  = timer->old_ticks ;     /* reload ticks */
                else  {
                    core_timer_remove( timer ) ;
                }
            }

            //find out minium interval for delay
            if( timer->ticks > 0 && timer->ticks < interval )
                    interval = timer->ticks ;

            timer  = ptr ;
        }

        pthread_mutex_unlock(&timer_mutex);
    }

    return NULL ;
}


//   Function : core_timer_show
//Description : display timer status
void core_timer_show( void )
{
    USER_TIMER * timer ;

    pthread_mutex_lock( &timer_mutex ) ;

    timer = active_timer ;
    printf("\n  Timer Status List :\n");
    while( timer != NULL )
    {
        printf("   Timer ID  = %d \n" , timer->timer_id ) ;
        printf("     ->  Message command = 0x%04X\n" , timer->message  ) ;
        printf("     ->         receiver = 0x%04X\n" , timer->receiver ) ;
        printf("     ->       call back  = 0x%08X\n" , (unsigned int)timer->callback ) ;
        printf("     ->       Timer Type = %d\n"     , timer->timer_type ) ;
        printf("     ->      Timer Ticks = %d\n"     , timer->ticks      ) ;
        timer = timer->next ;
    }

    pthread_mutex_unlock(&timer_mutex);
}


