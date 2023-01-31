#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "sys-list.h"
#include "sys-frameflow.h"



typedef struct 
{
    pthread_mutex_t mutex ;
    int             item_count ;
    SYS_LIST_NODE   idle ;
    SYS_LIST_NODE   ready ;
    SYS_LIST_NODE   nodes[1] ;
}FRAMEFLOW_CONTEXT ;


FRAMEFLOW_HANDLE sys_frameflow_create( int item_count , void **bufs )
{
    int size = sizeof( FRAMEFLOW_CONTEXT ) + sizeof(SYS_LIST_NODE) * (item_count-1) ;
    FRAMEFLOW_CONTEXT *frameflow  = (FRAMEFLOW_CONTEXT *)malloc(size) ;
    SYS_LIST_NODE *node ;
    
    
    sys_init_list( &frameflow->idle );
    sys_init_list( &frameflow->ready )  ;
    int i = 0 ;
    
    node = &frameflow->nodes[0] ;
    for ( i=0 ; i<item_count ; i++ )
    {
        node->dat  = bufs[i];
        //printf("%s bufs[%d] = %p\r\n", __FUNCTION__, i, bufs[i]);
        sys_list_add_tail( &frameflow->idle , node++ );
    }
    
    frameflow->item_count = item_count ;
    pthread_mutex_init( &frameflow->mutex, NULL );
    return (FRAMEFLOW_HANDLE)frameflow ;
}


void sys_frameflow_destroy( FRAMEFLOW_HANDLE handle )
{
   
    free( handle );
}


int sys_frameflow_alloc_item ( FRAMEFLOW_HANDLE handle , FRAMEFLOW_ITEM *item )
{
   FRAMEFLOW_CONTEXT *frameflow  = ( FRAMEFLOW_CONTEXT *)handle ;
   
   SYS_LIST_NODE *node   ;
   
   pthread_mutex_lock ( &frameflow->mutex ) ;   
   node = sys_list_get_first ( &frameflow->idle );   
   
   pthread_mutex_unlock ( &frameflow->mutex ) ;
   
   if ( node != NULL )
   {
       item->data = node->dat ;
       item->ref  = (void *)node ;
       return 1 ;
   }
   
   return 0 ;
}


int sys_frameflow_push_item ( FRAMEFLOW_HANDLE handle, FRAMEFLOW_ITEM *item)
{
    FRAMEFLOW_CONTEXT *frameflow = (FRAMEFLOW_CONTEXT *)handle ;
    
    SYS_LIST_NODE *node = (SYS_LIST_NODE *)item->ref ;
    if ( node->dat == NULL )
        node->dat          = item->data ;
    
    pthread_mutex_lock ( &frameflow->mutex ) ;
    sys_list_add_tail( &frameflow->ready, node ) ;
    pthread_mutex_unlock ( &frameflow->mutex ) ;
    return 1 ;
}


int sys_frameflow_pull_item ( FRAMEFLOW_HANDLE handle , FRAMEFLOW_ITEM *item )
{
    FRAMEFLOW_CONTEXT *frameflow = (FRAMEFLOW_CONTEXT *)handle ;
    
    SYS_LIST_NODE *node ;
    
    pthread_mutex_lock ( &frameflow->mutex ) ;    
    node = sys_list_get_first( &frameflow->ready );
    if ( node == NULL )
    {
        pthread_mutex_unlock ( &frameflow->mutex ) ;    
        return 0 ;
    }
        
    item->data = node->dat ;
    item->ref  = node ;
    pthread_mutex_unlock ( &frameflow->mutex ) ;    
    return 1 ;
}


int sys_frameflow_free_item ( FRAMEFLOW_HANDLE handle, FRAMEFLOW_ITEM *item )
{
    FRAMEFLOW_CONTEXT *frameflow = (FRAMEFLOW_CONTEXT *)handle ;
    
    SYS_LIST_NODE *node ;
    
    node = (SYS_LIST_NODE *)item->ref ;
    if (item->data == NULL )
        node->dat = NULL ;
    item->data = NULL ;
    
    pthread_mutex_lock ( &frameflow->mutex ) ;    
    sys_list_add_tail ( &frameflow->idle, node );
    pthread_mutex_unlock ( &frameflow->mutex ) ;    
    
    return 1 ;
}




