#ifndef __SYS_FRAMEFLOW_H__
#define __SYS_FRAMEFLOW_H__


typedef void * FRAMEFLOW_HANDLE ;



typedef struct
{
    void *data ;
    void *ref ;
}FRAMEFLOW_ITEM ;

FRAMEFLOW_HANDLE sys_frameflow_create( int item_count , void **bufs );
void sys_frameflow_destroy( FRAMEFLOW_HANDLE handle );
int sys_frameflow_alloc_item ( FRAMEFLOW_HANDLE handle , FRAMEFLOW_ITEM *item );
int sys_frameflow_push_item ( FRAMEFLOW_HANDLE handle, FRAMEFLOW_ITEM *item);
int sys_frameflow_pull_item ( FRAMEFLOW_HANDLE handle , FRAMEFLOW_ITEM *item );
int sys_frameflow_release_item ( FRAMEFLOW_HANDLE handle, FRAMEFLOW_ITEM *item );
int sys_frameflow_free_item ( FRAMEFLOW_HANDLE handle, FRAMEFLOW_ITEM *item );


#endif
