#define _GNU_SOURCE
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
#include <sys/syscall.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <sched.h>
 
#include "sys-core.h"
#include "sys-service.h"


#define  MAX_FILENAME_LEN   64
//CORE模块结构定义
typedef struct _tagCORE_MODULE
{
    char        name[MAX_FILENAME_LEN]    ;
    void *      handle      ;    //记录so/dll的句柄
    
    MODULE_INIT     module_init    ;
    MODULE_EXIT     module_exit    ;
        
    struct _tagCORE_MODULE * next  ;
}CORE_MODULE ;

//CORE THREAD 结构定义
typedef struct _tagCORE_THREAD
{
    char    name[MAX_FILENAME_LEN] ;
    
    int                 pid        ;
    pthread_t           thread_id  ;
    CORE_THREAD_FUNC    func       ;
    void          *     para       ;   
    
    struct _tagCORE_THREAD * next  ; 
}CORE_THREAD ; 

///function defined here
int    get_module_name( char * name , char * out ) ;
char * get_thread_name( int pid , int tid ) ;
void * core_thread_entry( void * para ) ;

static CORE_THREAD   *   core_thread = NULL ;
static CORE_MODULE   *   core_module = NULL ;        //系统内部维护的链表
static pthread_mutex_t   core_mutex    ;


char * signal_str[] = {
    "Hangup (POSIX)"   ,
    "Interrupt (ANSI)" , 
    "Quit (POSIX)" ,
    "Illegal instruction (ANSI)",
    "Trace trap (POSIX)",
    "Abort (ANSI)",
    "IOT trap (4.2 BSD)" ,
    "BUS error (4.2 BSD)" ,
    "Floating-point exception (ANSI)" ,
    "Kill, unblockable (POSIX)" ,
    "User-defined signal 1 (POSIX)" ,
    "Segmentation violation (ANSI)" ,
    "User-defined signal 2 (POSIX)" ,
    "Broken pipe (POSIX)" ,
    "Alarm clock (POSIX)" ,
    "Termination (ANSI)" ,
    "Stack fault" ,
    "Same as SIGCHLD (System V)" ,
    "Child status has changed (POSIX)" ,
    "Continue (POSIX)" ,
    "Stop, unblockable (POSIX)" ,
    "Keyboard stop (POSIX)" ,
    "Background read from tty (POSIX)" ,
    "Background write to tty (POSIX)" ,
    "Urgent condition on socket (4.2 BSD)" ,
    "CPU limit exceeded (4.2 BSD)" ,
    "File size limit exceeded (4.2 BSD)" ,
    "Virtual alarm clock (4.2 BSD)" ,
    "Profiling alarm clock (4.2 BSD)" ,
    "Window size change (4.3 BSD, Sun)" ,
    "Pollable event occurred (System V)" ,
    "I/O now possible (4.2 BSD)" ,
    "Power failure restart (System V)" ,
    "Bad system call" ,
};


/////////////////////////////////////////////////////////
//     Function : core_excepiton
//  Description : 
//        Input :
//       Output :
void core_excepiton( int sig )
{
    int    pid = syscall(SYS_gettid) ; //getpid() ;
    char *name = get_thread_name( pid , 0 ) ;
    
    printf("\n\n===================================\n");
    printf("Singal[%d]:\"%s\" \n" , sig , signal_str[sig] ) ;
    printf("Pid = %d Name = %s\n", pid , name ? name : "<Empty>" ) ;  
    printf("==========================\n\n");
    exit(0);      
}




/////////////////////////////////////////////////////////
//     Function : core_init
//  Description : init internal data
//        Input :
//       Output :
int core_init( void )
{
    int i  ;
    
    //安装信号捕获函数    
    for( i = 1 ;  i <= 31 ; i++ )
    {
        if( i == SIGUSR1 || i == SIGUSR2 || i == SIGCHLD )
            continue ;
        
        signal( i , core_excepiton ) ;    
    }    
    
    core_module = NULL ;
    core_thread = NULL ;
    pthread_mutex_init( &core_mutex  , NULL );    
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : core_exit
//  Description : exit internal data , 不会被调用到
//        Input :
//       Output :
int core_exit( void )
{
    CORE_MODULE   * module ; 
    CORE_THREAD   * td ;   
    void *ptemp ;


    //free module list    
    module = core_module ;
    while( module != NULL )
    {
        module->module_exit( ); //call each module's exit function

        if( module->handle )
            dlclose( module->handle ) ;

        ptemp    = module ;
        module   = module->next ;
        free( ptemp ) ;
    }
    
    td = core_thread ;
    while( td != NULL )
    {        
        ptemp  = td ;
        td = td->next ;
        free( ptemp ) ;
    }
    
    
    pthread_mutex_destroy( &core_mutex ) ;
    core_module = NULL  ;
    core_thread = NULL  ;

    return 1 ;
}



/////////////////////////////////////////////////////////
//     Function : core_load_module
//  Description : load one module into system
//        Input :
//       Output :
int core_load_module  ( char * name )
{    
    CORE_MODULE * module , * ptr ;
    char  module_name[32] , func_name[48] ;
        
    if( !get_module_name( name , module_name ) )    
        return 0 ;


    //检查是否已经载入系统了
    ptr = core_module ;
    while( ptr )
    {
        if( strcmp( ptr->name , name ) == 0 )
        {
            LOG_PRINTF("[%s] module %s exist" , __FUNCTION__ , name );
            return 0 ;
        }
        ptr = ptr->next ;
    }


    module = (CORE_MODULE*)malloc(sizeof(CORE_MODULE));
    if( module == NULL )
    {
        LOG_PRINTF("[%s] alloc memory failed" , __FUNCTION__ ) ;
        return 0 ;
    }

    module->next   = NULL ;    
    module->handle = dlopen( name , RTLD_NOW ) ;
    if( module->handle == NULL )
    {
        LOG_PRINTF("[%s] load %s failed ." , __FUNCTION__ , name ) ;
        LOG_PRINTF(" =>%s" , dlerror() ) ;
        free( module ) ;
        return 0 ;
    }

    strcpy( module->name , name ) ;

    //load module function address
    dlerror();
    
    sprintf( func_name , "%s_init" , module_name ) ;
    module->module_init = dlsym( module->handle , func_name ) ;
    sprintf( func_name , "%s_exit" , module_name ) ;
    module->module_exit = dlsym( module->handle , func_name ) ;
    if( dlerror() )
    {
        LOG_PRINTF("[%s] %s is wrong DLL" , __FUNCTION__ , name ) ;
        dlclose( module->handle ) ;
        free( module ) ;
        return 0  ;    
    }
       
    if( !module->module_init()  )
    {
        LOG_PRINTF("[%s] init module failed" , __FUNCTION__ ) ;
        dlclose( module->handle ) ;
        free( module ) ;
        return 0 ;
    }

    //add to system
    pthread_mutex_lock( &core_mutex ) ;
    if( core_module == NULL )
    {
        core_module = module ;
    }else{
        ptr = core_module ;
        while( ptr->next != NULL )
            ptr = ptr->next ;
    
        ptr->next = module ;
    }
    pthread_mutex_unlock(&core_mutex );

    LOG_PRINTF("Module[%s] load ok !" , name ) ;
    return 1 ;
}



/////////////////////////////////////////////////////////
//     Function : core_unload_module
//  Description : unload one module from system
//        Input :
//       Output :
int core_unload_module( char * name )
{
    CORE_MODULE  *ptr , *p ;
    
    if( core_module == NULL )
        return 0 ;

    ptr = core_module ;
    while( ptr )
    {
        if( strcmp( ptr->name , name ) == 0 )
            break ;
        ptr = ptr->next ;
    }

    if( ptr == NULL )
    {
        LOG_PRINTF("[%s] %s is not in the system" , __FUNCTION__ , name ) ;
        return 0 ;
    }

    ptr->module_exit( ) ;

    pthread_mutex_lock( &core_mutex ) ;
    p = core_module ;
    if( core_module == ptr )
    {
        core_module = ptr->next ;
    }else{
        while( p->next != ptr )
            p = p->next ;

        p->next = ptr->next ;
    }

    pthread_mutex_unlock(&core_mutex );

    dlclose( ptr->handle ) ; 
    free( ptr ) ;

    LOG_PRINTF("Module[%s] unload ok !" , name ) ;
  
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : core_thread_entry
//  Description : core thread main entry
//        Input :
//       Output :
void * core_thread_entry( void * para ) 
{
    CORE_THREAD * td , *ptr ;
            
    td = ( CORE_THREAD * ) para ;
    
    //增加对队列中去
    pthread_mutex_lock( &core_mutex ) ;
    td->next    = core_thread ;
    core_thread = td ;
    pthread_mutex_unlock(&core_mutex );
    
    //启动真正的函数    
    td->pid =  syscall(SYS_gettid) ;
    //_getpid();
    //LOG_PRINTF("Thread id = %d, ppid = %d, name = %s \r\n", td->pid, getppid(), td->name );     
    td->func( td->para ) ;
   
    
    //从队列中删除
    pthread_mutex_lock( &core_mutex ) ;
    ptr = core_thread ;
    if( ptr == td )  //头部
    {
        core_thread = td->next ;
    }else{
        while( ptr->next != td )
            ptr = ptr->next ;
        ptr->next = td->next ;        
    }
    pthread_mutex_unlock(&core_mutex );
    
    free( td ) ;
    
    return NULL ;    
}



/////////////////////////////////////////////////////////
//     Function : core_create_thread
//  Description : create a detached thread
//        Input :
//       Output :
int  core_create_thread ( char * name , CORE_THREAD_FUNC func , void * para )
{    
    CORE_THREAD * td ;
    
    td = ( CORE_THREAD * ) malloc( sizeof(CORE_THREAD) ) ;
    if( td == NULL ) 
    {
        LOG_PRINTF("Core : Cant create thread !");
        return 0 ;
    }
    
    td->next = NULL ;
    td->func = func ;
    td->para = para ;
    strcpy( td->name , name ) ;
    
    if( pthread_create( &td->thread_id , NULL , core_thread_entry , td ) == 0 )
    {        
        pthread_setname_np( td->thread_id , td->name ) ;   
        pthread_detach( td->thread_id ) ;
        return (int)td->thread_id ;
    }
    
    free( td ) ;
    
    return 0 ;
}

/////////////////////////////////////////////////////////
//     Function : core_wait_thread
//  Description : wait a detached thread
//        Input :
//       Output :
int core_wait_thread ( int id )
{
    return  pthread_join( id, NULL );
}

/////////////////////////////////////////////////////////
//     Function : core_set_thread_cpu
//  Description : set thread running on the specific cpu
//        Input :
//       Output :
int  core_set_thread_cpu( int cpu )
{
    cpu_set_t cpuset;
    pthread_t tid ;
    int ret ;
    
    tid = pthread_self();

    /* Set affinity mask to include CPUs 0 to 7 */
    CPU_ZERO(&cpuset);
    CPU_SET (cpu , &cpuset);
    ret = pthread_setaffinity_np( tid , sizeof(cpu_set_t), &cpuset);
    if( ret != 0)
        return 0 ;

    /* Check the actual affinity mask assigned to the thread */
    CPU_ZERO(&cpuset);
    ret = pthread_getaffinity_np( tid , sizeof(cpu_set_t), &cpuset);
    if( ret != 0 ) 
        return 0 ;
        
    return CPU_ISSET( cpu , &cpuset) ;
}


/////////////////////////////////////////////////////////
//     Function : get_thread_name
//  Description : get thread name
//        Input :
//       Output :
char * get_thread_name ( int pid , int tid ) 
{
    CORE_THREAD * td ;
    td = core_thread ;
    while( td != NULL )
    {
        if( td->pid == pid || td->thread_id == tid )
            return td->name ;
        
        td = td->next ;    
    }
    
    return NULL ;
}

/////////////////////////////////////////////////////////
//     Function : get_module_name
//  Description : wait a detached thread
//        Input :
//       Output :
int get_module_name( char * name , char * out )
{
    char *ptr ;
    
    if( name[0] == 0 )
        return 0 ;
        
    ptr = strrchr( name , '/' ) ;
    if( ptr == NULL )
        ptr = name ;
    strcpy( out , ptr + 1 ) ;
    
    ptr = strchr( out , '.' ) ;        
    if( ptr )
        *ptr = 0 ;
    
    return 1 ;
}

/////////////////////////////////////////////////////////
//     Function : core_list_module
//  Description : wait a detached thread
//        Input :
//       Output :
int core_list_module   ( char * buf  )
{    
    CORE_MODULE  *ptr ;
    char  m[32] ;
    int ret ;
    
    if( core_module == NULL )
        return 0 ;

    //ret    = sprintf( buf , "Modules : \n");
    printf("Modules : \n");
    ptr    = core_module ;
    while( ptr != NULL )
    {
        get_module_name( ptr->name , m ) ;
        //ret   += sprintf( buf + ret , "  %-16s   %-32s \n" , m , ptr->name  ) ;
        printf("  %-16s   %-32s \n" , m , ptr->name  ) ;
        ptr    = ptr->next ;
    }

    return ret + 1 ;    
}


/////////////////////////////////////////////////////////
//     Function : core_list_thread
//  Description : wait a detached thread
//        Input :
//       Output :
int core_list_thread  ( void  )
{    
    CORE_THREAD  *ptr ;
    int ret ;
    
    if( core_thread == NULL )
        return 0 ;

    //ret    = sprintf( buf , "Modules : \n");
    printf("Threads : \n");
    ptr    = core_thread ;
    while( ptr != NULL )
    {
        //ret   += sprintf( buf + ret , "  %-16s   %-32s \n" , m , ptr->name  ) ;
        printf("\tThread name:%-32s   pid:%-8d \n" , ptr->name, ptr->pid  ) ;
        ptr    = ptr->next ;
    }

    return ret + 1 ;    
}

