/****************************************************************************
  Copyright (C), 2011, Robert Fan

  Histroy  :
            1)  Created by fanshaohua   2011/08/10

*/

#ifndef __CONFIG_DB__
#define __CONFIG_DB__

#ifdef __cplusplus
    extern "C" {
#endif
#include <sys/types.h>


#define  SERVICE_CFG           "/hivintek/app/camera.cfg"
#define  SERVICE_CFG_DEF       "/hivintek/app/camera.cfg.default"

#define  IPCKEY_ID               0xCC
#define  MAX_CONFIG_FILE_SIZE    128*1024

// internal tools
#define  MAX_NAME_LEN     32
#define  MAX_DATA_LEN     64
#define  MAX_CHAR_LEN     96
#define  MAX_LINE_LEN     255

#define  MAX_CFG_ITEM     64
#define  MAX_CFG_SECTION  64

#define  PARAM_DELIMITER   '&'
#define  VALUE_DELIMITER   '='


typedef struct{
    int   item_line;
    int   item_size;
    int   changed;
    char  name [MAX_NAME_LEN] ;
    char  value[MAX_DATA_LEN] ;
}CONFIG_ITEM ;

typedef struct{
    int  count ;
    int  line;
    char name [ MAX_NAME_LEN ] ;
    CONFIG_ITEM  item[MAX_CFG_ITEM] ;
}CONFIG_SECTION ;

typedef struct{
    //pthread_rwlock_t rwlock;
    int              ready;
    int              changed;
    int              count ;
    CONFIG_SECTION   section[ MAX_CFG_SECTION ] ;
    char             buffer[ MAX_CONFIG_FILE_SIZE ]; //存放配置文件内容
}CONFIG_FILE ;

typedef struct
{
    int           shmid  ;
    int           semid  ;
    key_t         shmkey ;
    CONFIG_FILE * config ;
}SHM_CONFIG;


int shmcfg_open( SHM_CONFIG *shmcfg );
int shmcfg_close( SHM_CONFIG *shmcfg  );
int shmcfg_destory( SHM_CONFIG *shmcfg );
int shmcfg_flush(  SHM_CONFIG *shmcfg  );
int shmcfg_reload( SHM_CONFIG *shmcfg  );
int shmcfg_set_string( SHM_CONFIG *shmcfg , char * sec , char * name ,char * value );
int shmcfg_set_integer( SHM_CONFIG *shmcfg , char * sec , char * name , int value  );
int shmcfg_get_integer( SHM_CONFIG *shmcfg , char * sec , char * item , int def_value );
int shmcfg_set_hex    ( SHM_CONFIG *shmcfg , char * sec , char * name , int value  );
int shmcfg_get_hex    ( SHM_CONFIG *shmcfg , char * sec , char * item , int def_value );
float  shmcfg_get_float ( SHM_CONFIG *shmcfg , char * sec , char * item , float def_value );
int    shmcfg_set_float ( SHM_CONFIG *shmcfg , char * sec , char * name , float value  );
char * shmcfg_get_string( SHM_CONFIG *shmcfg , char * sec , char * item);
char * shmcfg_get_str_def( SHM_CONFIG *shmcfg , char * sec , char * item , char * def_value ) ;
CONFIG_SECTION * shmcfg_get_section( SHM_CONFIG *shmcfg, char *sec );
void config_remove_section ( CONFIG_FILE * res , char * sec );
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
int shmcfg_replace( SHM_CONFIG *shmcfg ,char * sect , char * file ) ;


#ifdef __cplusplus
    }
#endif

#endif
