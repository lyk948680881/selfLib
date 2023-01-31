
#include "sys-list.h"


void sys_init_list ( struct sys_list_head * head )
{
    head->pre  = head ;
    head->next = head ;
    head->dat  = 0 ;
}

void sys_list_del ( struct sys_list_head * node )
{
    struct sys_list_head * pre = node->pre ;
    struct sys_list_head * next = node->next ;
    
    pre->next  = node->next ;
    next->pre  = pre ;
}

int sys_list_empty ( struct sys_list_head * head )
{
    if ( (head->pre == head) &&  (head->next = head) )
        return 1 ;
    
    return 0 ;
}

void sys_list_add_tail ( struct sys_list_head * head, struct sys_list_head * node )
{
    struct sys_list_head *tail = head->pre ;
    
    node->pre = tail ;
    tail->next = node ;
    head->pre = node ;
    node->next = head ;
}

struct sys_list_head * sys_list_get_first ( struct sys_list_head * head )
{
    if ( sys_list_empty( head ) == 1 )
        return 0 ;
       
       
    struct sys_list_head *first = head->next ;
    struct sys_list_head *next = first->next ;
    head->next = first->next ;
    next->pre  = first->pre ; 
    
    return first ;
}

