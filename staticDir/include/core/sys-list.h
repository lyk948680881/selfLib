#ifndef __SYS_LIST_H__
#define __SYS_LIST_H__


typedef struct sys_list_head
{
     struct sys_list_head *pre, *next ; 
     void *dat ;
}SYS_LIST_HEAD, SYS_LIST_NODE ;


void sys_init_list ( struct sys_list_head * head );
void sys_list_del ( struct sys_list_head * node );
void sys_list_add_tail ( struct sys_list_head * head, struct sys_list_head * node );
struct sys_list_head * sys_list_get_first ( struct sys_list_head * head );

#endif 
