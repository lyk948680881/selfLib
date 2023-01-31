/********************************************************************************************
 *        Copyright (C), 2011, Robert Fan
 *
 *        使用POSIX中的pthread函数来模拟VxWorks中的msgQLib,semLib
 *
 *
 */
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>

#include "sys-core.h"

#define  _VXWORKS_ASSERT(a)  CORE_ASSERT(a)

/////////////////////////////////////////////////////////////////////////////////////////////
//  信号量函数定义
//
#define  SEM_VALID     0x53454D41             //SEMA
#define  _VXWORKS_BSEM       0x00             //二进制信号量
#define  _VXWORKS_CSEM       0x01             //COUNT信号量
typedef struct
{
    int                semValid  ;
    int                semType   ;
    int                semCount  ;
    pthread_mutex_t    semMutex  ;            //Mutex用来包含整个SEM的数据结果
    pthread_cond_t     semCond   ;            //用于等待事件发生
}_VXWORKS_SEM ;



//   Function :  semCreate
//Description :  create a semaphore of vxworks os
SEM_ID semCreate ( int  type ,  int initial )
{
    _VXWORKS_SEM * sem ;

    sem = ( _VXWORKS_SEM * ) malloc ( sizeof( _VXWORKS_SEM ) ) ;
    if( sem == NULL )
        return NULL ;

    sem->semValid  =  SEM_VALID ;
    pthread_mutex_init( &sem->semMutex , NULL ) ;
    pthread_cond_init ( &sem->semCond  , NULL ) ;
    sem->semType   =  type ;
    sem->semCount  =  initial ;

    return sem ;
}


SEM_ID semBCreate ( int  notUsed,  int initialState )
{
    return semCreate( _VXWORKS_BSEM , initialState ? SEM_FULL : SEM_EMPTY ) ;
}

SEM_ID semCCreate ( int  notUsed,  int initialCount )
{
    return semCreate( _VXWORKS_CSEM , initialCount ) ;
}

SEM_ID semMCreate ( int  notUsed )
{
    return semCreate( _VXWORKS_CSEM , 1 ) ;
}


//   Function :  semDelete
//Description :  delete a semaphore of vxworks os
int  semDelete  ( SEM_ID semId  )
{
    _VXWORKS_SEM * sem ;

    _VXWORKS_ASSERT( semId != NULL );

    sem = ( _VXWORKS_SEM * ) semId ;
    if( sem->semValid != SEM_VALID )
        return VX_ERROR ;

    pthread_mutex_lock  ( &sem->semMutex ) ;
    sem->semCount = 1 ;
    sem->semValid = 0 ;
    pthread_cond_broadcast( &sem->semCond ) ;    //唤醒所有的任务
    pthread_mutex_unlock  ( &sem->semMutex) ;

    usleep( 1000 * 5 ) ;                         //等待5 ms

    pthread_cond_destroy ( &sem->semCond  ) ;
    pthread_mutex_destroy( &sem->semMutex ) ;

    free( sem ) ;

    return VX_OK ;
}

//   Function :  semTake
//Description :  Take a semaphore of vxworks os
int    semTake    ( SEM_ID semId,  int    timeOut )
{
    _VXWORKS_SEM * sem ;

    _VXWORKS_ASSERT( semId != NULL );

    sem = ( _VXWORKS_SEM * ) semId ;
    if( sem->semValid != SEM_VALID )
        return VX_ERROR ;

    pthread_mutex_lock  ( &sem->semMutex ) ;
    if( sem->semCount == 0 )
    {
        if( timeOut == NO_WAIT )
        {
            pthread_mutex_unlock( &sem->semMutex ) ;
            return VX_ERROR ;
        }

        if( timeOut == WAIT_FOREVER )
        {
            while( sem->semCount == 0 )
                pthread_cond_wait( &sem->semCond , &sem->semMutex ) ;
        }else{
            struct timeval   now;
            struct timespec  out;
            long   usec ;

            usec =  timeOut * 10 * 1000 ;         //Convert to microsecond
            gettimeofday( &now, (struct timezone *)NULL );
            usec =  usec + now.tv_usec ;
            if( usec > 1000000 )                  //大于1秒了
            {
                now.tv_sec = now.tv_sec + usec / 1000000 ;
                usec       = usec % 1000000 ;
            }
            out.tv_sec  = now.tv_sec ;
            out.tv_nsec = usec * 1000;

            if( pthread_cond_timedwait( &sem->semCond , &sem->semMutex , &out ) != VX_OK )
            {
                //time out now
                pthread_mutex_unlock( &sem->semMutex ) ;
                return VX_ERROR ;
            }
        }

        //现在获得了信号量了
        if( sem->semValid != SEM_VALID )  //在等待期间，有可能信号量被删除
            return VX_ERROR ;
    }



    //存在信号量，直接返回
    sem->semCount-- ;
    pthread_mutex_unlock( &sem->semMutex ) ;

    return VX_OK ;

}

//   Function :  semGive
//Description :  Release a semaphore of vxworks os
int    semGive    ( SEM_ID semId  )
{
    _VXWORKS_SEM * sem ;

    _VXWORKS_ASSERT( semId != NULL );

    sem = ( _VXWORKS_SEM * ) semId ;
    if( sem->semValid != SEM_VALID )
        return VX_ERROR ;

    pthread_mutex_lock  ( &sem->semMutex ) ;

    if( sem->semValid != SEM_VALID )
    {
        //信号量已经被删除
        pthread_mutex_unlock( &sem->semMutex ) ;
        return VX_ERROR ;
    }

    if( sem->semCount == 0 )
    {
        sem->semCount++ ;
        pthread_cond_signal( &sem->semCond ) ;    //唤醒所有的任务
    }else{
        if( sem->semType == _VXWORKS_CSEM )
            sem->semCount++ ;
    }
    pthread_mutex_unlock( &sem->semMutex ) ;

    return VX_OK ;
}

int semGetCount(  SEM_ID semId )
{
    _VXWORKS_SEM * sem ;

    _VXWORKS_ASSERT( semId != NULL );

    sem = ( _VXWORKS_SEM * ) semId ;
    if( sem->semValid != SEM_VALID )
        return VX_ERROR ;

    return sem->semCount ;
}







/////////////////////////////////////////////////////////////////////////////////////////////
//  消息队列函数定义
//
#define  MSGQ_VALID     0x4D534751             //MSGQ
typedef struct
{
    int       msgValid  ;

    int       msgWrite  ;
    int       msgRead   ;
    int       msgLength ;
    int       msgMaxNum ;

    SEM_ID    msgRecv   ;               //接受消息
    SEM_ID    msgSend   ;               //发送消息
    SEM_ID    msgMutex  ;               //消息更新的互斥    

    int       msgSendErr ;
    int       msgRecvErr ;

    char    * msgBuffer ;
}_VXWORKS_MSGQ ;


//   Function :  msgQCreate
//Description :  Create a message queque
MSG_Q_ID msgQCreate( int maxMsgs, int maxMsgLength,  int notUsed )
{
    _VXWORKS_MSGQ * msgQ ;

    msgQ = ( _VXWORKS_MSGQ * ) malloc( sizeof(_VXWORKS_MSGQ) ) ;
    if( msgQ == NULL )
        return NULL ;

    msgQ->msgBuffer = ( char* ) malloc( maxMsgs * maxMsgLength ) ;
    if( msgQ->msgBuffer == NULL )
    {
        free( msgQ ) ;
        return NULL ;
    }

    msgQ->msgValid  = MSGQ_VALID ;
    msgQ->msgWrite  = 0          ;
    msgQ->msgRead   = 0          ;
    msgQ->msgSendErr= 0          ;
    msgQ->msgRecvErr= 0          ;
    msgQ->msgLength = maxMsgLength ;
    msgQ->msgMaxNum = maxMsgs      ;

    msgQ->msgRecv   = semCCreate(  0 , 0         ) ;
    msgQ->msgSend   = semCCreate(  0 , maxMsgs   ) ;
    msgQ->msgMutex  = semBCreate(  0 , 1         ) ; //FULL
    return msgQ ;
}

//   Function :  msgQSend
//Description :  send a message to message queque
int msgQSend       ( MSG_Q_ID msgQId ,  char * buffer,  int nBytes, int timeOut, int noUsed )
{
    _VXWORKS_MSGQ * msgQ ;
    int len ;

    _VXWORKS_ASSERT( msgQId != NULL );

    msgQ = ( _VXWORKS_MSGQ * ) msgQId ;
    if( msgQ->msgValid != MSGQ_VALID )
        return VX_ERROR ;

    if( semTake( msgQ->msgSend , timeOut ) != VX_OK )
    {
        msgQ->msgSendErr++ ;
        return VX_ERROR ;
    }
        
    //有空间
    semTake( msgQ->msgMutex , WAIT_FOREVER ) ;
    len  = nBytes > msgQ->msgLength ? msgQ->msgLength : nBytes ;
    memcpy( msgQ->msgBuffer + msgQ->msgWrite * msgQ->msgLength , buffer , len ) ;
    msgQ->msgWrite = ( msgQ->msgWrite + 1 ) % msgQ->msgMaxNum ;
    semGive( msgQ->msgMutex ) ;


    semGive( msgQ->msgRecv  ) ;       //Wake up the block task

    return VX_OK ;
}


//   Function :  msgQReceive
//Description :  receive a message from message queque
int msgQReceive    ( MSG_Q_ID msgQId ,  char * buffer,  int maxNBytes,  int timeOut )
{
    _VXWORKS_MSGQ * msgQ ;
    int len ;

    _VXWORKS_ASSERT( msgQId != NULL );

    msgQ = ( _VXWORKS_MSGQ * ) msgQId ;
    if( msgQ->msgValid != MSGQ_VALID )
        return VX_ERROR ;

    if( semTake( msgQ->msgRecv , timeOut ) != VX_OK )
    {
        msgQ->msgRecvErr++ ;
        return VX_ERROR ;
    }

    //有消息了
    semTake( msgQ->msgMutex , WAIT_FOREVER ) ;
    len  = maxNBytes > msgQ->msgLength ? msgQ->msgLength : maxNBytes ;
    memcpy( buffer , msgQ->msgBuffer + msgQ->msgRead * msgQ->msgLength , len ) ;
    msgQ->msgRead = ( msgQ->msgRead + 1 ) % msgQ->msgMaxNum ;
    semGive( msgQ->msgMutex ) ;
    
    semGive( msgQ->msgSend ) ;        //释放了一个消息空间

    return len ;
}

//   Function :  msgQDelete
//Description :  delete a message queque
int msgQDelete     ( MSG_Q_ID msgQId )
{
    _VXWORKS_MSGQ * msgQ ;

    _VXWORKS_ASSERT( msgQId != NULL );

    msgQ = ( _VXWORKS_MSGQ * ) msgQId ;
    if( msgQ->msgValid != MSGQ_VALID )
        return VX_ERROR ;

    semDelete( msgQ->msgSend  ) ;
    semDelete( msgQ->msgRecv  ) ;
    msgQ->msgValid  =  0 ;

    free( msgQ->msgBuffer ) ;
    free( msgQ ) ;

    return VX_OK ;
}

//   Function :  msgQNumMsgs
//Description :  get the message count in the queque
int msgQNumMsgs    ( MSG_Q_ID msgQId )
{
    _VXWORKS_MSGQ * msgQ ;

    _VXWORKS_ASSERT( msgQId != NULL );

    msgQ = ( _VXWORKS_MSGQ * ) msgQId ;
    if( msgQ->msgValid != MSGQ_VALID )
        return VX_ERROR ;

    return semGetCount( msgQ->msgRecv ) ;
}

//   Function :  msgQReceive
//Description :  receive a message from message queque
int msgQShow       ( MSG_Q_ID msgQId )
{
    _VXWORKS_MSGQ * msgQ ;

    _VXWORKS_ASSERT( msgQId != NULL );

    msgQ = ( _VXWORKS_MSGQ * ) msgQId ;
    if( msgQ->msgValid != MSGQ_VALID )
        return 0 ;

    printf("\nMessage Queque = 0x%08X " , (int)msgQ ) ;
    printf("\n   msg quequed = %d " , msgQNumMsgs(msgQId)   ) ;
    printf("\n   send errors = %d " , msgQ->msgSendErr ) ;
    printf("\n   recv errors = %d " , msgQ->msgRecvErr ) ;

    return 0 ;
}



