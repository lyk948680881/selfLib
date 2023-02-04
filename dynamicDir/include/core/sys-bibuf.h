
#ifndef __SYS_BIBUF__
#define __SYS_BIBUF__

#define   MAX_BIBUF_NUM    3

typedef enum 
{
    SBS_READING  = 0,
    SBS_READDONE = 1 ,
    SBS_WIRTING  = 2 ,
    SBS_WRITTEN  = 3 ,
    SBS_IDLE     = 4 
}SYS_BIBUF_STAT ;


typedef struct 
{

    
    void            *buf  ;
    int              buf_stat ;    
}SYS_BIBUF ;

typedef struct 
{
    pthread_mutex_t mutex ;
    
    SYS_BIBUF   bibufs[MAX_BIBUF_NUM] ;
}SYS_BIBUF_CONTEXT ;




#endif



