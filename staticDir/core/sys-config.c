/****************************************************************
    Copyright (C), 2002, Pallas Digital Tech. Ltd.

    FileName: config.c
Description : 通过共享内存访问配置文件，以便给多个进程使用

*****************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <fcntl.h>

//#include "sys-core.h"
#include "sys-config.h"

//打開此宏定義時使用共享內存，
//否則使用進程內部空間，此時只能有單個進程使用
#define CONFIG_USE_SHARE_MEMORY

//item_line或者section_line值，大于等于0时为在对应文件的行数
#define   ITEM_NEW_ADD     (-1)
#define   ITEM_DELETE      (-2)

//////////////////////////////////////////////////////////////////////
//     Function : get_line_size
//  Description : get length of ptr before a new line, include '\n'
//        Input :
//       Output :
int get_line_size( char *ptr )
{
    char *p = ptr;

    while ( *p != 0 && *p != '\n' )
        p++;

    return *p == 0 ? p - ptr : p - ptr + 1;
}

//////////////////////////////////////////////////////////////////////
//     Function : get_line_string
//  Description : get string of src before a new line, include '\n'
//        Input :
//       Output :
int get_line_string( char *dest, char *src )
{
    char *p = src;

    while( *p )
    {
        *dest++ = *p;
        if( *p++ == '\n')
            break;
    }
    *dest = '\0';
    return p - src;
}

//////////////////////////////////////////////////////////////////////
//     Function : lock_file
//  Description :
//        Input :
//       Output : on error -1
int lock_file( int fd )
{
    int i = 0 ,ret = -1;
    struct flock lock;

    /* Initialize the flock structure. */
    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;

    /* Place a write lock on the file. */
    ret = fcntl( fd, F_SETLK, &lock );
    while( ret == -1 && i++ < 10 )
    {
        usleep(100*1000); /* 最多等待10*100ms */
        ret = fcntl( fd, F_SETLK, &lock );
    }
    return ret;
}

//////////////////////////////////////////////////////////////////////
//     Function : unlock_file
//  Description :
//        Input :
//       Output :
void unlock_file( int fd )
{
    struct flock lock;

    /* Initialize the flock structure. */
    memset (&lock, 0, sizeof(lock));
    /* Release the lock. */
    lock.l_type = F_UNLCK;
    fcntl (fd, F_SETLK, &lock);
}


//////////////////////////////////////////////////////////////////////
//     Function : config_char_special
//  Description : check if char is special char
//        Input :
//       Output : 1 yes , 0 no
int  config_char_special( char dat )
{
    char * pattern = "#;\n\r" ;
    char * ptr ;

    ptr = pattern ;
    while( *ptr != 0 )
    {
        if( *ptr == dat )
            return 1 ;
        ptr++ ;
    }
    return ( dat == 0 ? 1:0 ) ;
}

//////////////////////////////////////////////////////////////////////
//     Function : config_get_field_data
//  Description : get field string value according to seperate
//        Input :
//       Output : 0 error
int  config_get_field_data( char * src , char * dst , char seperate , char skip , int max )
{
    int  count , check ;

    count = check = 0 ;
    while( count < max - 1 )
    {
        if( config_char_special( *src ) )
            break ;

        if( *src == skip )//|| *src < ' ' )
        {
            src++ ;
            check++ ;
            continue ;
        }

        if( seperate == *src  )
        {
            //发现了匹配字符
            *dst =  0    ;
            return ( count == 0 ? 0 : check + 1 ) ;
        }

        //还没有遇到匹配字符
        *dst++ = *src++ ;
        count++ ;
        check++ ;
    }

    *dst = 0 ;
    if( seperate == 0 )  //normal
        return check  ;

    return 0 ;
}


//////////////////////////////////////////////////////////////////////
//     Function : config_read_file
//  Description : load resource file to config struct
//        Input :
//       Output : 1 = ok
int config_read_file ( char * name , CONFIG_FILE * cfg )
{
    CONFIG_SECTION  * current ;
    CONFIG_ITEM     * item    ;
    char   buffer[MAX_LINE_LEN + 1] , *ptr ;
    FILE * fp  ;
    int len,line;
    int ret ;

    fp = fopen( name , "r" ) ;
    if( fp == NULL )
    {
        fp = fopen( name , "a+" ) ;
        if( fp == NULL )
            return 0 ;
    }
    buffer[MAX_LINE_LEN] = 0 ;
    current     = NULL ;
    item        = NULL ;
    line        = -1;

    while( !feof( fp ) )
    {
        if( fgets( buffer , MAX_LINE_LEN , fp ) == NULL )
            break ;

        line++;
        ptr = buffer ;
        //去掉开始的空格
        while( *ptr != 0  && *ptr <= ' ' )
            ptr++ ;

        if( config_char_special(*ptr) )   //是注释的行
            continue ;

        if( *ptr == '[' )   //SECTION开始
        {
            ptr++ ;
            current = &cfg->section[cfg->count++] ;

            current->line = line;
            current->count = 0  ;
            if( config_get_field_data( ptr , current->name , ']' , ' ' ,  MAX_NAME_LEN ) == 0 )
            {
                //printf("syntax error : %s \n" , buffer ) ;
                fclose( fp ) ;
                return 0 ;
            }
            continue ;
        }

        if( current == NULL )
            continue ;

        //item line
        item = &current->item[current->count++] ;
        item->item_line   = line;
        item->changed     = 0;

        //step1: get item name
        ret = config_get_field_data( ptr , item->name , '=' , ' ' ,  MAX_NAME_LEN ) ;
        if( ret == 0 )
        {
            //printf("syntax error : %s \n" , buffer ) ;
            fclose( fp ) ;
            return 0 ;
        }
        ptr += ret ;
        item->item_size = ptr - buffer;

        //step2: get item value
        ret = config_get_field_data( ptr , item->value , 0 , ' ' ,  MAX_DATA_LEN ) ;
        item->item_size += ret;

    }

    fseek( fp, 0, SEEK_END );
    len = ftell( fp );
    if( len > MAX_CONFIG_FILE_SIZE )
    {
        cfg->buffer[0] = 0;
        return 1;
    }

    rewind( fp ) ;
    fread( cfg->buffer, 1, len ,fp);
    cfg->buffer[len] = 0;
    fclose( fp ) ;
    return 1 ;
}


//////////////////////////////////////////////////////////////////////
//     Function : config_get_section
//  Description : get section
//        Input :
//       Output : NULL if not find
CONFIG_SECTION * config_get_section ( CONFIG_FILE *res, char *sec )
{
    int i ;

    for ( i=0; i<res->count ; i++ )
    {
        if ( strcmp( res->section[i].name, sec ) == 0 )
        {
            return &res->section[i] ;
        }
    }
    return NULL ;
}


//////////////////////////////////////////////////////////////////////
//     Function : config_get_item
//  Description : get string value
//        Input :
//       Output : NULL if not find
CONFIG_ITEM *  config_get_item ( CONFIG_FILE * res , char * sec , char * item )
{
    CONFIG_SECTION * cfg = NULL  ;
    int i ;

    //find sections
    for( i = 0 ; i < res->count ; i++ )
    {
        if( strcmp( res->section[i].name , sec ) == 0 )
        {
            cfg = &res->section[i] ;
            break ;
        }
    }

    if( cfg == NULL  )
        return NULL ;         //not find it

    for( i = 0 ; i < cfg->count ; i++ )
    {
        if( strcmp( cfg->item[i].name , item ) == 0 )
            return &cfg->item[i] ;
    }

    return NULL ;
}

//////////////////////////////////////////////////////////////////////
//     Function : config_get_str_def
//  Description : get string value
//        Input :
//       Output : def_value if not find
char * config_get_str_def( CONFIG_FILE * res , char * sec , char * item ,char * def_value )
{
    CONFIG_SECTION * cfg ;
    int i ;

    //find sections
    cfg = NULL  ;
    for( i = 0 ; i < res->count ; i++ )
    {
        if( strcmp( res->section[i].name , sec ) == 0 )
        {
            cfg = &res->section[i] ;
            break ;
        }
    }

    if( cfg == NULL  )
        return def_value ;         //not find it

    for( i = 0 ; i < cfg->count ; i++ )
    {
        if( strcmp( cfg->item[i].name , item ) == 0 )
            return cfg->item[i].value ;
    }

    return def_value ;
}

//////////////////////////////////////////////////////////////////////
//     Function : config_get_string
//  Description : get string value
//        Input :
//       Output : NULL if not find
char * config_get_string ( CONFIG_FILE * res , char * sec , char * item )
{
    CONFIG_SECTION * cfg ;
    int i ;

    //find sections
    cfg = NULL  ;
    for( i = 0 ; i < res->count ; i++ )
    {
        if( strcmp( res->section[i].name , sec ) == 0 )
        {
            cfg = &res->section[i] ;
            break ;
        }
    }

    if( cfg == NULL  )
        return NULL ;         //not find it

    for( i = 0 ; i < cfg->count ; i++ )
    {
        if( strcmp( cfg->item[i].name , item ) == 0 )
            return cfg->item[i].value ;
    }

    return NULL ;
}


//////////////////////////////////////////////////////////////////////
//     Function : config_get_integer
//  Description : get integer value
//        Input :
//       Output : default value or config value
int  config_get_integer( CONFIG_FILE * res , char * sec , char * item , int def_value )
{
    char * ptr ;
    int  value ;

    ptr = config_get_string( res , sec , item ) ;
    if( ptr == NULL )
        return def_value ;

    if( ptr[0] == 0 )
        return def_value ;

    sscanf( ptr , "%i" , &value ) ;

    return value ;
}


//////////////////////////////////////////////////////////////////////
//     Function : config_write_file
//  Description : write config to file
//        Input :
//       Output : error 0
int config_write_file ( char * name , CONFIG_FILE * cfg )
{
    CONFIG_SECTION  * sect ;
    CONFIG_ITEM     * item    ;
    FILE * fp  ;
    int i ,j ,len ,fd;
    int line,line_size;
    char *ptr;
    char  buffer[MAX_LINE_LEN + 1];


    fp = fopen( name , "a" ) ;
    if( fp == NULL )
        return 0 ;

    if ( lock_file( fileno(fp) ) == -1 )
    {        
        /* 文件已經被鎖，加鎖失敗 */
        printf("lock fail \r\n");
        fclose(fp);
        return 0;
    }

    fp = freopen( name , "r+" ,fp );
    if( fp == NULL )
    {
        printf("freopen error\r\n");
        return 0 ;
    }
    ptr  = cfg->buffer;
    buffer[MAX_LINE_LEN] = 0;
    line = 0;
    for ( i = 0; i < cfg->count ;i++ )
    {
        sect = &cfg->section[i];
        /* 输出section之前的内容 */        
        while ( line++ < sect->line )
        {
            ptr += get_line_string( buffer, ptr );
            fputs( buffer, fp );            
        }

        if( sect->line == ITEM_DELETE ) /* 删除的section */
        {
            int line_temp = line ;
            for (j = 0; j < sect->count ;j++ )
            {
                if( line <= sect->item[j].item_line )
                    line = sect->item[j].item_line+1 ;
            }
            //跳过段中所有行不输出
            while ( line_temp++ <= line )
                ptr += get_line_string( buffer, ptr );
            continue ;
        }
        else if( sect->line == ITEM_NEW_ADD )  /* 输出新加的section */
        {
            snprintf( buffer, MAX_LINE_LEN, "\n[%s]\n", sect->name );
            line--;
        }
        else
        {
            ptr += get_line_string( buffer, ptr );
        }
        fputs( buffer, fp );        

        for (j = 0; j < sect->count ;j++ )
        {
            item = &sect->item[j];
            /* 输出当前item之前的内容 */
            while ( line++ < item->item_line )
            {
                ptr += get_line_string( buffer, ptr );
                fputs( buffer, fp );                
            }

            if( item->item_line == ITEM_NEW_ADD ) //新加的item
            {
                snprintf( buffer, MAX_LINE_LEN, "    %-16s = %-s\n", item->name, item->value );
                line--;
            }
            else if( item->changed )  //修改的item
            {
                line_size = get_line_size( ptr );
                snprintf( buffer, MAX_LINE_LEN, "    %-16s = %-16s%.*s", item->name, item->value,
                   line_size - item->item_size , ptr + item->item_size );
                ptr += line_size;
            }
            else //其余原样输出
            {
                ptr += get_line_string( buffer, ptr );
            }
            fputs( buffer, fp );            
        }
    }

    fputs( ptr, fp ); /* cfg->buffer里未读取的内容原样输出 */

    len = ftell(fp);
    fd = fileno(fp);
    ftruncate(fd, len);
    
    unlock_file( fileno(fp) );
    fclose( fp ) ;
    system("sync");
    return 1 ;

}


//////////////////////////////////////////////////////////////////////
//     Function : config_remove_section
//  Description : remove section
//        Input :
//       Output : 
void config_remove_section ( CONFIG_FILE * res , char * sec )
{
    int i ;
   
    for( i=0; i< res->count; i++ )
    {
        if ( strncmp( res->section[i].name, sec, MAX_NAME_LEN) == 0 )
        {
            res->section[i].line = ITEM_DELETE ;
            res->changed = 1;
            break ;
        }
    }
}

//////////////////////////////////////////////////////////////////////
//     Function : config_set_string
//  Description : set config string ,if not exist ,add new
//        Input :
//       Output : if item value changed return non zero;else 0
int config_set_string ( CONFIG_FILE * res , char * sec , char * name ,char * value )
{
    CONFIG_SECTION * section ;
    CONFIG_ITEM    * item;
    int i ;

    //find sections
    section = NULL  ;
    for( i = 0 ; i < res->count ; i++ )
    {
        if( strncmp( res->section[i].name , sec, MAX_NAME_LEN ) == 0 )
        {
            section = &res->section[i] ;
            break ;
        }
    }

    //add new section
    if( section == NULL  )
    {
        if( res->count >= MAX_CFG_SECTION )
        {
            return 0 ;
        }
        section = &res->section[res->count];
        res->count++;
        strncpy( section->name, sec, MAX_NAME_LEN - 1);
        section->count = 0;
        section->line =  ITEM_NEW_ADD;
    }

    for( i = 0 ; i < section->count ; i++ )
    {
        item = &section->item[i];
        if( strncmp( item->name , name, MAX_NAME_LEN ) == 0 )
        {
            if ( strncmp( item->value , value, MAX_DATA_LEN ) == 0 )
            {
                return 0;
            }
            strncpy( item->value ,value, MAX_DATA_LEN - 1);
            item->changed = 1;
            res->changed++;
            return 1;
        }
    }

    //add new item
    if( section->count >= MAX_CFG_ITEM - 1)
    {
        return 0 ;
    }
    item = &section->item[section->count];
    strncpy ( item->name , name, MAX_NAME_LEN - 1);
    strncpy ( item->value , value, MAX_DATA_LEN - 1);
    item->item_line = ITEM_NEW_ADD ;
    section->count++;
    res->changed++;
    return 2 ;

}

#ifdef CONFIG_USE_SHARE_MEMORY //使用共享内存
    #define  READ_LOCK(shmcfg)    (__shmcfg_read_lock(shmcfg->semid))
    #define  READ_UNLOCK(shmcfg)  (__shmcfg_read_unlock(shmcfg->semid))
    #define  WRITE_LOCK(shmcfg)   (__shmcfg_write_lock(shmcfg->semid))
    #define  WRITE_UNLOCK(shmcfg) (__shmcfg_write_unlock(shmcfg->semid))
#else //不使用共享内存的操作
    #define  READ_LOCK(shmcfg)    (pthread_rwlock_rdlock( &shmcfg->config->rwlock ))
    #define  READ_UNLOCK(shmcfg)  (pthread_rwlock_unlock( &shmcfg->config->rwlock ))
    #define  WRITE_LOCK(shmcfg)   (pthread_rwlock_wrlock( &shmcfg->config->rwlock ))
    #define  WRITE_UNLOCK(shmcfg) (pthread_rwlock_unlock( &shmcfg->config->rwlock ))
#endif

#ifdef CONFIG_USE_SHARE_MEMORY //使用共享内存
    /* The user should define a union like the following to use it for arguments
       for `semctl'.*/
    union semun
    {
     int val;                   //<= value for SETVAL
     struct semid_ds *buf;      //<= buffer for IPC_STAT & IPC_SET
     unsigned short int *array; //<= array for GETALL & SETALL
     struct seminfo *__buf;     //<= buffer for IPC_INFO
    };
    /* Previous versions of this file used to define this union but this is
       incorrect.  One can test the macro _SEM_SEMUN_UNDEFINED to see whether
       one must define the union or not.  */
    #define _SEM_SEMUN_UNDEFINED    1

    #define FLAG_CREATE_NEW  (IPC_CREAT | IPC_EXCL | 0666)
    #define FLAG_GET_EXIST   (IPC_CREAT | 0666)
    #define sem_lock_rwmutux   { 0, -1, 0 }
    #define sem_unlock_rwmutux { 0,  1, IPC_NOWAIT }
    #define sem_up_rdcount     { 1,  1, IPC_NOWAIT }
    #define sem_down_rdcount   { 1, -1, IPC_NOWAIT }
    #define sem_test_rdcount   { 1,  0, 0 }

    int __shmcfg_read_lock( int semid )
    {
        //读的时候要保证没有人写，后续可以继续让其他人读
        struct sembuf buf[2] = { sem_lock_rwmutux, sem_up_rdcount };
        struct sembuf buf2   = sem_unlock_rwmutux ;

        if( semop( semid, buf, 2 ) )
            return -1;
        semop( semid, &buf2, 1 );
        return 0;
    }

    int __shmcfg_read_unlock( int semid )
    {
        struct sembuf buf = sem_down_rdcount;
        return semop( semid, &buf, 1 );
    }

    int __shmcfg_write_lock( int semid )
    {
        //写的时候要保证没有其他人写或者读
        //写优先
        struct sembuf buf1  = sem_lock_rwmutux ;
        struct sembuf buf2  = sem_test_rdcount ;

        if( semop( semid, &buf1, 1 ) )
            return -1;
        return semop( semid, &buf2, 1 );
    }

    int __shmcfg_write_unlock( int semid )
    {
        struct sembuf buf = sem_unlock_rwmutux;
        return semop( semid, &buf, 1 );
    }

    /***********************************************************
    Fuction : shmcfg_create_sem
    Description:
        创建systemV信号集，包括2个信号量
        第1个为读和写的互斥锁，值大于0时可读或写
        第2个是读的信号量，值为0时可写
    Input:
    Output:
    Return :
        -1 on error, semid on success
    ************************************************************/
    int shmcfg_create_sem( key_t shmkey )
    {
        int  semid = -1;
        int  i, init_ok = 0;
        unsigned short sem_init_val[] = { 1, 0 };
        struct semid_ds sem_info;
        union semun sem_arg;
        semid = semget( shmkey, 2, FLAG_CREATE_NEW );
        if( semid < 0 )
        {
    		semid = semget( shmkey, 2, FLAG_GET_EXIST );
    		sem_arg.buf = &sem_info;
    		for( i = 0; i < 100; i++ )
    		{
    			if( semctl(semid, 0, IPC_STAT, sem_arg) == -1 )
    			    return -1;
    			if( sem_arg.buf->sem_otime != 0 )
                {
                    init_ok = 1;
                    break;
                }
                usleep( 10000 );
    		}
    		if( !init_ok )
    		{
    			sem_arg.array = sem_init_val;
    			if( semctl( semid, 0, SETALL, sem_arg ) == -1 )
                    return -1;
    		}
        }
        else //semid>=0; do some initializing
        {
    		sem_arg.array = sem_init_val;
    		if( semctl( semid, 0, SETALL, sem_arg ) == -1 )
                return -1;
        }
        return semid;
    }
#endif




/***********************************************************
Fuction : shmcfg_open
Description:
    获取存放配置的共享内存，
    若不存在则创建新的共享内存并加载配置文件
Input:
Output:
Return :
    -1:error, 0 :ok
************************************************************/
int shmcfg_open( SHM_CONFIG *shmcfg )
{
#ifdef CONFIG_USE_SHARE_MEMORY //使用共享内存
    int   shmid   = -1;
    int   semid   = -1;
    key_t shmkey;
    CONFIG_FILE *config = NULL;

    memset( shmcfg, 0 ,sizeof(SHM_CONFIG) );
    shmkey = ftok( "/hivintek/app", IPCKEY_ID )  ;
    semid  = shmcfg_create_sem( shmkey );
    if( semid < 0 )
        goto error_handle ;
        
    
    //加上独享锁
    if( __shmcfg_write_lock( semid ))
        goto error_handle ;
    shmid  = shmget( shmkey, sizeof(CONFIG_FILE), FLAG_CREATE_NEW );
    if( shmid < 0 )
    {
        //share memory exist
        shmid  = shmget( shmkey, sizeof(CONFIG_FILE), FLAG_GET_EXIST );
        if( shmid < 0 )
            goto error_handle ;

        config = shmat( shmid, NULL, 0 ) ;
        if( config == (void *)(-1) )
            goto error_handle ;

    }  else  {
        //new created share memory
        config = shmat( shmid, NULL, 0 );
        if( config == (void *)(-1) )
            goto error_handle ;
        config->ready = 0 ;
    }
    printf("%s %d\r\n", __FUNCTION__, __LINE__ );  
    if( ! config->ready )
    {
        if( !config_read_file( SERVICE_CFG, config ) )
            goto error_handle ;

        config->ready = 1;
    }
    printf("%s %d\r\n", __FUNCTION__, __LINE__ );  
    //解除独享锁
    __shmcfg_write_unlock( semid );

    shmcfg->config = config ;
    shmcfg->shmid  = shmid  ;
    shmcfg->semid  = semid  ;
    shmcfg->shmkey = shmkey ;
    return 0;

error_handle:
    if( config )
        shmdt ( config );
    if( shmid != -1 )
        shmctl( shmid, IPC_RMID, NULL );
    if( semid != -1 )
        semctl( semid, 0, IPC_RMID );
    return -1;

#else //不使用共享内存的操作
    CONFIG_FILE *config = NULL;

    memset( shmcfg, 0 ,sizeof(SHM_CONFIG) );

    config = (CONFIG_FILE *)malloc( sizeof(CONFIG_FILE) );
    if( !config )
        return -1 ;

    if( !config_read_file( SERVICE_CFG, config ) )
        goto error_handle;
    config->ready = 1;

    pthread_rwlock_init( &config->rwlock, NULL );
    shmcfg->config = config ;
    return 0;

error_handle:
    if( config )
        free ( config );
    return -1;
#endif
}

/***********************************************************
Fuction : shmcfg_replace
Description:
    将file指定配置文件中的section段取出放入shmcfg结构中，
    如果不存在则新添加进去,如果已经存在则替换
Input:
Output:
Return :
    -1:error, 0 :ok
************************************************************/
int shmcfg_replace( SHM_CONFIG *shmcfg ,char * sect , char * file )
{
    int i ;
    CONFIG_FILE *config = NULL;
    CONFIG_SECTION * src, * dst, * section ;

    if( shmcfg->config == NULL )
        return 0;

    config = (CONFIG_FILE *)malloc( sizeof(CONFIG_FILE) );
    if( !config )
        return -1 ;

    if( !config_read_file( file, config ) )
        goto error_handle;

    src = config_get_section( config, sect );
    if( src == NULL )
        goto error_handle;

   if( WRITE_LOCK(shmcfg ) )
        goto error_handle;

    if( !shmcfg->config->ready )
        goto error_unlock ;

    //删除被替换的段
    dst = config_get_section( shmcfg->config, sect );
    if( dst != NULL )
    {
        dst->line = ITEM_DELETE ;
    }

    //src段作为新段加入到config中
    if( shmcfg->config->count >= MAX_CFG_SECTION )
        goto error_unlock ;
    section = &shmcfg->config->section[shmcfg->config->count] ;
    memcpy( section, src, sizeof(*src) );
    shmcfg->config->count++       ;
    shmcfg->config->changed =  1  ;
    section->line  = ITEM_NEW_ADD ;
    //段中所有item也变成新加项
    for (i = 0; i < section->count ;i++ )
        section->item[i].item_line = ITEM_NEW_ADD ;
    //printf("shmcfg_replace section name[%s]\n ", section->name );

    //保存到文件
    if( !config_write_file( SERVICE_CFG, shmcfg->config ) )
        goto error_unlock ;
    shmcfg->config->count     = 0;
    shmcfg->config->changed   = 0;
    shmcfg->config->buffer[0] = 0;

    //重新加载
    if( config_read_file( SERVICE_CFG, shmcfg->config ) )
        shmcfg->config->ready = 1 ;
    else {
        shmcfg->config->ready = 0 ;
        goto error_unlock ;
    }

    WRITE_UNLOCK(shmcfg );
    free ( config );
    return 1 ;

error_unlock:
    WRITE_UNLOCK(shmcfg );
error_handle:
    if( config )
        free ( config );
    return 0;
}

/***********************************************************
Fuction : shmcfg_reload
Description:
    重新将配置文件加载到共享内存
Input:
Output:
Return :
    -1:error, 0 :ok
************************************************************/
int shmcfg_reload( SHM_CONFIG *shmcfg )
{
    int ret = 0;
    if( shmcfg->config == NULL ) return 0;
    if( WRITE_LOCK(shmcfg) )
        return -1;
    shmcfg->config->changed   = 0;
    shmcfg->config->count     = 0;
    shmcfg->config->buffer[0] = 0;
    ret = config_read_file( SERVICE_CFG, shmcfg->config );
    shmcfg->config->ready = (ret == 1) ? 1 : 0;
    WRITE_UNLOCK( shmcfg );
    return !ret ;
}

/***********************************************************
Fuction : shmcfg_flush
Description:
    将共享内存中配置写入配置文件
Input:
Output:
Return :
    -1:error, 0 :ok
************************************************************/
int shmcfg_flush( SHM_CONFIG *shmcfg )
{
    int ret = 0;
    printf("shmcfg = %p\r\n", shmcfg);
    if( shmcfg->config == NULL ) return 0;
    if( READ_LOCK( shmcfg ) )//加读锁就可以了?
        return -1;
    printf("shmcfg changed = %d, ready = %d\r\n", shmcfg->config->changed, shmcfg->config->ready);
    if( shmcfg->config->changed && shmcfg->config->ready )
    {
        shmcfg->config->changed = 0;
        ret = config_write_file( SERVICE_CFG, shmcfg->config );
    }
    READ_UNLOCK( shmcfg );
    return !ret;
}

/***********************************************************
Fuction : shmcfg_close
Description:
    卸载共享内存，但不删除
Input:
Output:
Return :
    -1:error, 0 :ok
************************************************************/
int shmcfg_close(  SHM_CONFIG *shmcfg )
{
    int ret = 0;
    if( shmcfg->config == NULL ) return -1;
    ret = shmcfg_flush( shmcfg );
#ifdef CONFIG_USE_SHARE_MEMORY //使用共享内存
    shmdt( shmcfg->config );
#else //不使用共享内存的操作
    pthread_rwlock_destroy( &shmcfg->config->rwlock );
    free( shmcfg->config );
#endif
    shmcfg->config = NULL;
    return ret;
}

/***********************************************************
Fuction : shmcfg_destory
Description:
    删除共享内存
Input:
Output:
Return :
    -1:error, 0 :ok
************************************************************/
int shmcfg_destory( SHM_CONFIG *shmcfg )
{
#ifdef CONFIG_USE_SHARE_MEMORY //使用共享内存
    if( shmcfg->semid != -1 )
        semctl( shmcfg->semid, 0, IPC_RMID );
    if(  shmcfg->shmid != -1 )
        shmctl( shmcfg->shmid, IPC_RMID, NULL );
    shmcfg->semid = -1;
    shmcfg->shmid = -1;
    return 0;
#else //不使用共享内存的操作
    return 0;
#endif
}


/***********************************************************
Fuction : shmcfg_get_section
Description:
    获取SECTION后如要访问或修改其中内容
    请自行对shmcfg->semid加读写锁，防止冲突
Input:
Output:
Return :
************************************************************/
CONFIG_SECTION * shmcfg_get_section (  SHM_CONFIG *shmcfg , char *sec )
{
    CONFIG_SECTION * ptr;
    if( shmcfg->config == NULL ) return NULL;
    READ_LOCK( shmcfg );
    ptr = config_get_section( shmcfg->config, sec );
    READ_UNLOCK( shmcfg );
    return ptr;
}

char * shmcfg_get_string( SHM_CONFIG *shmcfg , char * sec , char * item)
{
    char * ptr;
    if( shmcfg->config == NULL ) return NULL;
    READ_LOCK( shmcfg );
    ptr = config_get_string( shmcfg->config, sec , item );
    READ_UNLOCK( shmcfg );
    return ptr;
}

char * shmcfg_get_str_def( SHM_CONFIG *shmcfg , char * sec , char * item , char * def_value )
{
    char * ptr;
    if( shmcfg->config == NULL ) return def_value;
    READ_LOCK( shmcfg );
    ptr = config_get_str_def( shmcfg->config, sec , item , def_value );
    READ_UNLOCK( shmcfg );
    return ptr;
}

int shmcfg_get_integer( SHM_CONFIG *shmcfg , char * sec , char * item , int def_value )
{
    int ret;
    if( shmcfg->config == NULL ) return def_value;
    if( READ_LOCK( shmcfg )) return def_value;
    ret = config_get_integer( shmcfg->config, sec , item , def_value );
    READ_UNLOCK( shmcfg );
    return ret;
}


/***********************************************************
Fuction : shmcfg_set_string
Description:
     修改配置
Input:
Output:
Return :
    if item value changed return non zero;else 0; error 0;
************************************************************/
int shmcfg_set_string( SHM_CONFIG *shmcfg , char * sec , char * name ,char * value )
{
    int ret;
    if( shmcfg->config == NULL )  return 0;
    if( WRITE_LOCK(shmcfg) )      return 0;
    ret = config_set_string( shmcfg->config, sec , name, value );
    WRITE_UNLOCK( shmcfg );
    return ret;
}

/***********************************************************
Fuction : shmcfg_set_integer
Description:
     修改配置
Input:
Output:
Return :
    if item value changed return non zero;else 0; error 0;
************************************************************/
int shmcfg_set_integer( SHM_CONFIG *shmcfg , char * sec , char * name ,int value )
{
    int  ret ;
    char buf[12] ;
    if( shmcfg->config == NULL )  return 0;
    if( WRITE_LOCK(shmcfg) )      return 0;
    sprintf( buf, "%d", value );
    ret = config_set_string( shmcfg->config, sec , name, buf );
    WRITE_UNLOCK( shmcfg );
    return ret;
}

/***********************************************************
Fuction : shmcfg_set_hex
Description:
     修改配置
Input:
Output:
Return :
    if item value changed return non zero;else 0; error 0;
************************************************************/
int shmcfg_set_hex    ( SHM_CONFIG *shmcfg , char * sec , char * name , int value  )
{
    int  ret ;
    char buf[16] ;
    if( shmcfg->config == NULL )  return 0;
    if( WRITE_LOCK(shmcfg) )      return 0;
    sprintf( buf, "0x%x", value );
    ret = config_set_string( shmcfg->config, sec , name, buf );
    WRITE_UNLOCK( shmcfg );
    return ret;    
}

int shmcfg_get_hex    ( SHM_CONFIG *shmcfg , char * sec , char * item , int def_value )
{
    char * ptr;
    int  value;
    
    if( shmcfg->config == NULL ) return def_value;
    READ_LOCK( shmcfg );
    ptr = config_get_string( shmcfg->config , sec , item ) ;
    READ_UNLOCK( shmcfg );
    
    if( ptr == NULL || ptr[0] == 0 )
        return def_value ;
        
    sscanf( ptr , "0x%x" , &value ) ;
    return value ;
}

/***********************************************************
Fuction : shmcfg_set_float
Description:
     修改配置
Input:
Output:
Return :
    if item value changed return non zero;else 0; error 0;
************************************************************/
int    shmcfg_set_float ( SHM_CONFIG *shmcfg , char * sec , char * name , float value  )
{
    int  ret ;
    char buf[12] ;
    if( shmcfg->config == NULL )  return 0;
    if( WRITE_LOCK(shmcfg) )      return 0;
    sprintf( buf, "%f", value );
    ret = config_set_string( shmcfg->config, sec , name, buf );
    WRITE_UNLOCK( shmcfg );
    return ret;
}

float  shmcfg_get_float ( SHM_CONFIG *shmcfg , char * sec , char * item , float def_value )
{
    char * ptr;
    float  value;
    
    if( shmcfg->config == NULL ) return def_value;
    READ_LOCK( shmcfg );
    ptr = config_get_string( shmcfg->config , sec , item ) ;
    READ_UNLOCK( shmcfg );
    
    if( ptr == NULL || ptr[0] == 0 )
        return def_value ;
        
    sscanf( ptr , "%f" , &value ) ;
    return value ;
}


