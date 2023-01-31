/********************************************************************************************
 *        Copyright (C), 2011, Robert Fan
 *
 *        定义一组内存池管理函数
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
#include <dlfcn.h>

#include "sys-core.h"

#define  POOL_VALID    0x504F4F4C   //POOL



//内存块定义 = 16个字节
typedef struct _tagMEM_BLOCK   
{
    int   valid    ;   //标志
    int   length   ;   //大小
    
    U8    * addr   ;   //地址 = this + sizeof(MEM_BLOCK)
    struct _tagMEM_BLOCK  * next ;
}MEM_BLOCK ;

//内存池定义。 
//注意：重新写了信号量，以支持分配内存时支持timeout，参考sys-vxworks.c
typedef struct 
{
    MEM_BLOCK  * header    ;
        
    U8         * buffer    ;
    int          total     ;
    
    pthread_mutex_t   mutex ;        //Mutex用来包含整个SEM的数据结果
    pthread_cond_t    cond  ;        //用于等待事件发生    
    int             wanted  ;        //需要的连续内存块大小
}MEM_POOL_CTRL ;



///////////////////////////////////////////////////////////
//private function
///////////////////////////////////////////////////////////
//查找一个有指定大小的块
static  MEM_BLOCK  * pool_find_blk( MEM_POOL_CTRL * pool , int wanted )
{
    MEM_BLOCK  * blk ;
    
    if( pool->header == NULL )
        return NULL ;
            
    blk = pool->header  ;
    while( blk != NULL )
    {
        if( blk->length >= wanted )
            return blk ;
        
        blk = blk->next ;
    }     
    return NULL ;   
}


//合并到可用队列中，如果是连续的块将被合并
static  void  pool_attach( MEM_POOL_CTRL * pool ,  MEM_BLOCK  * blk )
{
    MEM_BLOCK * ptr ;
    
    if( pool->header == NULL )
    {
        pool->header = blk ;
        return ;    
    }
 
    //Step 1. 添加到合适的队列位置中去
    if( pool->header > blk )
    {    
        blk->next    = pool->header ;
        pool->header = blk          ;
    }else{
        ptr = pool->header ;
        while( ptr != NULL )
        {            
            if( ptr < blk ) 
            {
                if( ptr->next == NULL || ptr->next > blk )
                {    
                    //找到位置，插入
                    blk->next = ptr->next ;
                    ptr->next = blk       ;
                    break ;
                }                        
            }                
            ptr = ptr->next ;        
        }
    }        

    //Step2. 检查是否需要合并
    ptr = pool->header ;
    while( ptr && ptr->next )
    {
        if( ptr->length + ptr->addr >= (U8*)ptr->next  )
        {
            //合并
            blk          = ptr->next ;
            ptr->length  = blk->length + blk->addr - ptr->addr ;
            ptr->next    = blk->next ;
            continue ;
        }
                           
        ptr = ptr->next ; 
    }

    return ;
}

/////////////////////////////////////////////////////////
//     Function : 
//  Description : 创建一个大小为length内存池
//        Input :
//       Output :
MEM_POOL_ID  core_pool_create ( int length , char * buf )    
{
    MEM_POOL_CTRL * pool ;
    MEM_BLOCK  *blk ;
    
    pool = ( MEM_POOL_CTRL * ) malloc( sizeof(MEM_POOL_CTRL)) ;
    if( pool == NULL )
        return NULL ;
    
    length       =  ( length + 3 )/4*4   ;  //4Bytes对齐
    pool->buffer =  (U8*)buf ;
    if( pool->buffer == NULL )
        pool->buffer =  (U8*)malloc( length );            
    if( pool->buffer == NULL )
    {
        free( pool ) ;    
        return NULL  ;    
    }
    
    pool->total  = length ;    
    
    //初始化第1个内存块
    blk  = ( MEM_BLOCK * ) pool->buffer ;
    blk->valid  = POOL_VALID ;
    blk->length = length - sizeof(MEM_BLOCK)       ;
    blk->addr   = pool->buffer + sizeof(MEM_BLOCK) ;
    blk->next   = NULL       ;
    pool->header= blk        ;
    
    
    pthread_mutex_init( &pool->mutex , NULL ) ;
    pthread_cond_init ( &pool->cond  , NULL ) ;    
    
    return (MEM_POOL_ID)pool ;
}

/////////////////////////////////////////////////////////
//     Function : core_pool_destroy
//  Description : 销毁一个内存池
//        Input :
//       Output :
void         core_pool_destroy( MEM_POOL_ID id )
{
    MEM_POOL_CTRL * pool = ( MEM_POOL_CTRL * ) id ;
    MEM_BLOCK  *blk ;
    
    if( pool == NULL )
        return ;
    
    pthread_mutex_lock    ( &pool->mutex ) ;
    
    //重新初始化第1个内存块，为后续锁定的任务设定条件
    blk  = ( MEM_BLOCK * ) pool->buffer ;
    blk->valid  = POOL_VALID ;
    blk->length = pool->total - sizeof(MEM_BLOCK)  ;
    blk->addr   = pool->buffer + sizeof(MEM_BLOCK) ;
    blk->next   = NULL       ;
    pool->header= blk        ;    
        
    pthread_cond_broadcast( &pool->cond  ) ;    //唤醒所有的任务
    pthread_mutex_unlock  ( &pool->mutex ) ;

    usleep( 1000 * 5 ) ;                         //等待5 ms

    pthread_cond_destroy ( &pool->cond  ) ;
    pthread_mutex_destroy( &pool->mutex ) ;
        
    free( pool->buffer ) ;
    free( pool ) ;    
}


/////////////////////////////////////////////////////////
//     Function : 
//  Description : 
//        Input :
//       Output :
void  *      core_pool_alloc  ( MEM_POOL_ID id , int length , int timeout ) 
{
    MEM_POOL_CTRL * pool = ( MEM_POOL_CTRL * ) id ;
    MEM_BLOCK  *ptr , *mem ;
    
    
    if( pool == NULL )
        return NULL ;    
        
    length = ( length + 3 )/4*4 ;

    pthread_mutex_lock  ( &pool->mutex ) ;
        
    ptr = pool_find_blk( pool , length ) ;
    if( ptr == NULL )
    {
        if( timeout == NO_WAIT )
        {
            pthread_mutex_unlock( &pool->mutex ) ;
            return NULL ;
        }
        
        pool->wanted = length ;
        if( timeout == WAIT_FOREVER )
        {
            do{
                pthread_cond_wait( &pool->cond , &pool->mutex ) ;
                ptr = pool_find_blk( pool , length ) ;
            }while( ptr == NULL ) ;
                           
        }else{
            struct timeval   now;
            struct timespec  out;
            long   usec ;

            usec =  timeout * 10 * 1000 ;         //Convert to microsecond
            gettimeofday( &now, (struct timezone *)NULL );
            usec =  usec + now.tv_usec ;
            if( usec > 1000000 )                  //大于1秒了
            {
                now.tv_sec = now.tv_sec + usec / 1000000 ;
                usec       = usec % 1000000 ;
            }
            out.tv_sec  = now.tv_sec ;
            out.tv_nsec = usec * 1000;

            if( pthread_cond_timedwait( &pool->cond , &pool->mutex , &out ) != VX_OK )
            {
                //time out now
                pool->wanted = 0 ;
                pthread_mutex_unlock( &pool->mutex ) ;
                return NULL ;
            }
            
            ptr = pool_find_blk( pool , length ) ;
            if( ptr == NULL )
            {    
                //！！！！不应该进入这个分支！！！！
                pool->wanted = 0 ;
                pthread_mutex_unlock( &pool->mutex ) ;                                
                CORE_CHECK( ptr != NULL , NULL ) ;                
            }
        }  
        
        pool->wanted = 0 ;      
    }
    
    
    
    //step 1. 把blk拆分
    mem = ptr ;

    if( ptr->length  >= ( length + sizeof(MEM_BLOCK) ) )
    {
        //内存可以拆成两块 
        ptr          =  ( MEM_BLOCK  * )(mem->addr + length ) ;
        ptr->valid   =  POOL_VALID ;
        ptr->length  =  mem->length - length - sizeof(MEM_BLOCK) ;
        ptr->addr    =  (U8*)(ptr + 1 ) ;
        ptr->next    =  mem->next  ;
        
        mem->length  =  length ;
        mem->next    =  ptr ;
    }
    
    //step 2. 把mem移出队列
    if( pool->header == mem )
    {
        pool->header = mem->next ;
    }else{        
        ptr = pool->header ;
        while( ptr->next != mem )    //mem一定在队列中
            ptr = ptr->next ;
        ptr->next = mem->next ;
    }
    
    mem->next = NULL ;                
    pthread_mutex_unlock( &pool->mutex ) ;       
    
    return ( void * )mem->addr ;
}

/////////////////////////////////////////////////////////
//     Function : core_pool_free
//  Description : 
//        Input :
//       Output :
void         core_pool_free   ( MEM_POOL_ID id , void *mem  )
{
    MEM_POOL_CTRL * pool = ( MEM_POOL_CTRL * ) id ;
    U8 * dat = ( U8 * ) mem ;
    MEM_BLOCK  *ptr  ;
    
    if( dat == NULL || pool == NULL )
        return ;
  
    ptr = ( MEM_BLOCK * )( dat - sizeof(MEM_BLOCK) ) ;
    CORE_ASSERT( ptr->valid == POOL_VALID ) ;
    
    pthread_mutex_lock  ( &pool->mutex ) ;
    
    pool_attach( pool , ptr ) ;  //添加到可用队列中去
    
    if( pool->wanted )
    {
        if( pool_find_blk( pool , pool->wanted ) )
            pthread_cond_signal( &pool->cond ) ;    //唤醒所有的任务
    }    
    pthread_mutex_unlock( &pool->mutex ) ;   
    
    
}

/////////////////////////////////////////////////////////
//     Function : core_pool_init
//  Description : 空函数，用以把模块连接进系统
//        Input :
//       Output :
int  core_pool_init   ( void )
{
    return 1 ;
}


/////////////////////////////////////////////////////////
//     Function : core_pool_print
//  Description : 
//        Input :
//       Output :
int         core_pool_print  ( MEM_POOL_ID id , char *buf  )
{
    MEM_POOL_CTRL * pool = ( MEM_POOL_CTRL * ) id ;
    MEM_BLOCK  *ptr ;    
    int ret ;

    if( pool == NULL )
        return 0 ;
                
    ret    = sprintf( buf , "POOL[%p] : Data=%p Len=%d\n" , pool , pool->buffer , pool->total );
    ptr    = pool->header ;
    while( ptr != NULL )
    {        
        ret   += sprintf( buf + ret , "  Block:%p Len:%d Data:%p Next:%p \n" , ptr , ptr->length , ptr->addr , ptr->next ) ;
        ptr    = ptr->next ;
    }

    return ret + 1 ;          
}

