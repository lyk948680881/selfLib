#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>


#include "sys-bibuf.h"



void bibuf_init ( SYS_BIBUF_CONTEXT *context , void *buf1, void *buf2  )
{
    pthread_mutex_init( &context->mutex, NULL );
    context->bibufs[0].buf = buf1 ;
    context->bibufs[1].buf = buf2 ;
    context->bibufs[0].buf_stat = SBS_IDLE ;
    context->bibufs[1].buf_stat = SBS_IDLE ;
}


void* bibuf_open_wrbuf ( SYS_BIBUF_CONTEXT *context )
{
     int index = -1 ;
     int i ;
     SYS_BIBUF *buf = &context->bibufs[0] ;
     pthread_mutex_lock ( &context->mutex ) ;
     for ( i=0 ; i<MAX_BIBUF_NUM ; i++ )
     {
         if (buf->buf_stat == SBS_IDLE || buf->buf_stat == SBS_READDONE )
         {
             index = i ;
             break ;
         }
     }
     if ( index == -1 )
     {
         for ( i=0 ; i<MAX_BIBUF_NUM ; i++ )
         {
             if ( buf->buf_stat == SBS_WRITTEN )
             {
                 index = i ;
                 break ;
             }
         }
     }
     pthread_mutex_unlock ( &context->mutex ) ;
     
     
     return context->bibufs[index].buf ;
     
}


void bibuf_close_wrbuf( SYS_BIBUF_CONTEXT *context , void *data )
{
     int *stat = (int *)(data+1) ;
     pthread_mutex_lock   ( &context->mutex ) ;
     *stat = SBS_WRITTEN ;
     pthread_mutex_unlock ( &context->mutex ) ;
}


void *bibuf_open_rdbuf( SYS_BIBUF_CONTEXT *context )
{
     int index = -1 ;
     int i ;
     SYS_BIBUF *buf = &context->bibufs[0] ;
     pthread_mutex_lock ( &context->mutex ) ;
     for ( i=0 ; i<MAX_BIBUF_NUM ; i++ )
     {
         if (buf->buf_stat == SBS_IDLE || buf->buf_stat == SBS_WRITTEN )
         {
             index = i ;
             break ;
         }
     }
     pthread_mutex_unlock ( &context->mutex ) ;
     
     if ( index == -1 )
         return NULL ;
         
     return context->bibufs[index].buf ;
}


void bibuf_close_rdbuf ( SYS_BIBUF_CONTEXT *context, void *data )
{
     int *stat = (int *)(data+1) ;
     pthread_mutex_lock   ( &context->mutex ) ;
     *stat = SBS_READDONE ;
     pthread_mutex_unlock ( &context->mutex ) ;
}

