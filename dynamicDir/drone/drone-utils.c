#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/mount.h>
#include <dirent.h>

#include "sys-core.h"
#include "sys-service.h"
#include "sys-config.h"
#include "sys-hal.h"
#include "sys-gui.h"
#include "app-codec.h"
#include "app-vinput.h"

#include "recorder.h"
#include "drone.h"
#include "internal.h"






///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//SDCARD �ļ�����ӿ�
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
////
int  INTF_parse_file_info( H264_JPG_FILE  * info , char * dir , char * name )
{
    int  fid , ch , find ;
    struct stat  statbuf ;
    char buffer[64] ;
    
    //��ȡͨ����
    ch   = VENC_IR_CHANNEL ;
    if( strstr( name , "KJ_" ) != NULL ) 
        ch = VENC_VI_CHANNEL ;        
    
    find = 0 ;
    if( strstr( name , ".264" ) != NULL )
    {
        info->video = 1 ;  
        find        = 1 ;                
        sscanf( name + 3 , "%d.264" , &fid ) ;
              
    }else if( strstr( name , ".jpg" ) != NULL ) {        
        info->video = 0 ;   
        find        = 1 ;    
        sscanf( name + 3 , "%d.jpg" , &fid ) ; //��recorderģ��һ��               
    }
    
    if( !find )
        return 0 ;

    if(dir == NULL)
        sprintf( buffer , "%s" ,  name ) ;
    else
        sprintf( buffer , "%s/%s" , dir , name ) ;
    
    if( stat( buffer , &statbuf ) != 0 )
        return 0 ;
      
      
    info->fid   = fid ;
    info->channel     = ch ;  
    info->length      = statbuf.st_size ;
    info->create_time = statbuf.st_ctim.tv_sec ;
    info->access_time = statbuf.st_atim.tv_sec ;
             
    printf(" PARSE ==>%d %08d %d %d\n" , info->video , info->fid , info->channel , info->length ) ; 
    return 1 ;                           
}

//  Description ������SD����������ļ����������һ��SDCARDĿ¼
//  Return : >  0 �ļ�����
//               0 SDCARDû�й��ػ���û�и�ʽ��
int  INTF_load_sdcard_files    ( void * data  , int * max_fid )
{
    struct dirent * entry ;
    DIR * dir ;

    H264_JPG_FILE  * ptr = ( H264_JPG_FILE * )data ;
    int  count  , max_data ;
    

    *max_fid = 0 ;
    count    = 0 ;   
    if( ptr == NULL )
        return count ;
           
    dir = opendir( STORAGE_DATA_DIR ) ;
    if( dir == NULL ) //Ŀ¼������      
        return count ;
    
    max_data = 0 ;    
    entry    = readdir( dir ) ;
    while( entry != NULL )
    {
        if( entry->d_type != DT_REG )
        {
            entry = readdir( dir ) ;
            continue ;
        }
                                                   
        if( INTF_parse_file_info( ptr , STORAGE_DATA_DIR , entry->d_name ) )            
        {    
            
            if( max_data < ptr->fid ) 
                max_data = ptr->fid ;
                
            count++ ;
            ptr++   ; 
            
            //����Ѿ������㹻�����ݣ�ֱ�ӷ��أ������Ѿ�����
            if( count >= MAX_H264_JPG_FILES )
                break ;               
        }
                    
        entry = readdir( dir ) ;        
    }
    
    closedir( dir ) ;   
    
    printf("FILES : count = %d , max = %d\n" , count , max_data );  
    
    *max_fid = max_data + 1 ; 
    return count ;
}

//  Description ������һ��û����չ�����ļ���
//  Return :  
void INTF_make_filename( int video , int fid  , int ch , char * no_ext_name )
{
    if( ch == VENC_IR_CHANNEL )
        sprintf( no_ext_name , "HW_%d"  , fid ) ;
    else
        sprintf( no_ext_name , "KJ_%d"  , fid ) ;
}


//  Description ��ɾ��ָ���ļ�
//  Return : 
void INTF_delete_file ( int video , int fid , int ch )
{
    char buffer[64] ;
    
    if( ch == VENC_IR_CHANNEL )
        sprintf( buffer , "%s/HW_%d" , STORAGE_DATA_DIR , fid ) ;
    else
        sprintf( buffer , "%s/KJ_%d" , STORAGE_DATA_DIR , fid ) ;
    
    if( video )
        strcat( buffer , ".264" ) ;
    else
        strcat( buffer , ".jpg" ) ;
    remove ( buffer ) ;
    
    LOG_PRINTF("Remove %s" , buffer ) ;
}


void  INTF_delete_file_by_name( char * name )
{
    char buffer[64] ;
    
    sprintf( buffer , "%s/%s" , STORAGE_DATA_DIR , name ) ;
    remove ( buffer ) ;
    
    LOG_PRINTF("Remove %s" , buffer ) ;
}




///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//ý��ӿڣ�¼������
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
//  Description ������¼��
//  Return : 
void INTF_start_record   ( int fid , int ch )
{    
    RECORDER_REC_MSG rec ;
    
    core_fill_message( &rec.header , APP_DRONE , APP_RECORDER , RECORDER_START_REC , sizeof( rec )  ) ;
    rec.rec_type      = RECORDER_TYPE_AVSTREAM ;
    rec.event_type    = RECORDER_EVENT_TYPE_REMOTE ;
    rec.secs          = -1 ; //Keep recording
    rec.channel       = ch ;
    
    INTF_make_filename( 1 , fid , ch , rec.name ) ;
    
    core_send_message ( (KERNEL_MESSAGE*)&rec ) ;
}

//  Description ��ֹͣ¼��
//  Return : 
void INTF_stop_record    ( int ch  )
{
    RECORDER_REC_MSG rec ;
        
    core_fill_message( &rec.header , APP_DRONE , APP_RECORDER , RECORDER_STOP_REC , sizeof( rec )  ) ;
    rec.rec_type      = RECORDER_TYPE_AVSTREAM ;
    rec.event_type    = RECORDER_EVENT_TYPE_REMOTE ;
    rec.secs          = -1 ; 
    rec.channel       = ch ;
    
    core_send_message ( (KERNEL_MESSAGE*)&rec ) ;
}

//  Description ��ץ��
//  Return :
void INTF_snap_picture   ( int fid , int ch , int count )
{
    RECORDER_REC_MSG rec ;
    

    core_fill_message( &rec.header , APP_DRONE , APP_RECORDER , RECORDER_START_SNAP , sizeof( rec ) ) ;
    rec.rec_type      = RECORDER_TYPE_JPEG ;
    rec.event_type    = RECORDER_EVENT_TYPE_REMOTE ;
    rec.secs          = count  ;  // 
    rec.channel       = ch ;
    
    INTF_make_filename( 0 , fid , ch , rec.name ) ;
    
    core_send_message ( (KERNEL_MESSAGE*)&rec ) ;
}

