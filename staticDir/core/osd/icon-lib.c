#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

#include "sys-core.h"
#include "sys-service.h"
#include "sys-hal.h"
#include "sys-gui.h"


//////////////////////////////////////////////////////////////
//图片资源库定义
//////////////////////////////////////////////////////////////

/*
* 资源文件的存储格式 
*       ICON_LIB_HEADER 
*       ICON_LIB_INDEX
*       ICON_LIB_INDEX
*       ICON_LIB_INDEX
*       ...        
*       
*       IMAGE_DATA
*       IMAGE_DATA
*       IMAGE_DATA
*       IMAGE_DATA
*       ...
*/
#define ICON_LIB_HEAD1 0x4E4F4349
#define ICON_LIB_HEAD2 0x2042494C 

//16 bytes
typedef struct
{
    int  head[2] ;
    int  count   ;  //icon's count
    int  offset  ;  //icon's base offset
}ICON_LIB_HEADER ;


//32 bytes
typedef struct
{
    int  address ;
    char id[28]  ;
}ICON_LIB_INDEX  ;

static U8 __internal_icon_data[] = {
    #include "icon-zero.h" 
};

static U8 *__icon_lib_data = __internal_icon_data  ;

//初始化icon lib
int  open_icon_lib ( char * name )
{
    ICON_LIB_HEADER * lib ;
    int len ;
    U8 *data;
    
    FILE  * fp = fopen( name , "rb" ) ;
    if( fp == NULL )
    {
        LOG_PRINTF("Fail to open icon lib %s , using internal" , name ) ;
        return 0 ; 
    }

    /////////使用外部的数据/////////
    fseek( fp , 0 , SEEK_END ) ;
    len  = ftell( fp ) ;
    data = (U8*)malloc( len ) ;
    if( data == NULL )
    {
        LOG_PRINTF("Fail to alloc memory [%d] , using internal" , len ) ;
        fclose( fp ) ;
        return 0 ;
    }

    fseek ( fp   , 0 , SEEK_SET ) ;
    fread ( data , 1 , len , fp ) ;
    fclose( fp ) ;

    lib   = ( ICON_LIB_HEADER * )data ;
    if( lib->head[0] == ICON_LIB_HEAD1 && lib->head[1] == ICON_LIB_HEAD2 )
    {
        LOG_PRINTF("Using external icon lib %s" , name ) ;
        __icon_lib_data = data ;
        return 1 ;
    }
    
    LOG_PRINTF("Icon lib is invalid %s , using internal" , name ) ;
    free( data ) ;
    
    return 0 ;
    
}

//关闭icon lib
void close_icon_lib ( void )
{
    if( __icon_lib_data == __internal_icon_data )
        return ;
        
    free( __icon_lib_data ) ;
    __icon_lib_data = __internal_icon_data  ;
}

//获取系统图标数据
U8 * get_icon_data ( char * id , int len ) 
{
    ICON_LIB_HEADER * lib   = ( ICON_LIB_HEADER * )__icon_lib_data ;
    ICON_LIB_INDEX  * index = ( ICON_LIB_INDEX * )( lib + 1 ) ;
    int i ;

    for( i = 0 ; i < lib->count ; i++ )
    {
        if( strncmp( index->id , id , len ) == 0 )
        {
            return (__icon_lib_data + index->address + lib->offset) ;
        }
        index++ ;
    }
    LOG_PRINTF("ICON res = %s  Not Found" , id ) ;
    return NULL ;
}
