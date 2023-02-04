/****************************************************************************
  Copyright (C), 2011, Robert Fan

  Histroy  :
            1)  Created by fanshaohua   2011/08/10

*/


#ifndef  ___CORE___
#define  ___CORE___


#ifdef __cplusplus
    extern "C" {
#endif


/*assert macro*/
#define CORE_DEBUG
#ifdef  CORE_DEBUG

    #define CORE_ASSERT(cond)\
    do{\
        if(!(cond))\
        {\
            printf("Condition:%s File:%s Line:%d  ASSERT FAILED!\n", #cond, __FILE__, __LINE__);\
            pause();\
        }\
    }while(0)

    #define CORE_CHECK(cond, ret)\
    do{\
        if(!(cond))\
        {\
            printf("Condition:%s File:%s Line:%d  CHECK FAILED!\n", #cond , __FILE__, __LINE__ );\
            return (ret);\
        }\
    }while(0)

#else

    #define CORE_ASSERT(cond)
    #define CORE_CHECK(cond, ret)

#endif

typedef  unsigned char   U8  ;
typedef  unsigned short  U16 ;
typedef  unsigned int    U32 ;
typedef  unsigned long long int U64 ;


typedef char            S8;
typedef short           S16;
typedef int             S32;

typedef volatile unsigned char  V8;
typedef volatile unsigned short V16;
typedef volatile unsigned int   V32;

typedef struct
{
    int x;
    int y;
    int w;
    int h;
}SYS_RECT;

#define  PACK_OPTION     __attribute__ ((packed))


//////////////////////////////////////////////////////////////////////////////
//        �ڲ�ģ�鶨��
//        ÿһ��ģ�鶼����������¶���ӿڣ�
//                  xxxx_init  ;
//                  xxxx_exit  ;
//        xxxx = ģ������ �磺ģ��ΪPTZ����PTZ_init/PTZ_exit
//        ÿһ��ģ����Ҫ����Ĳ�ע��һ��ָ����ʽ�Ľṹ�����Ժ���ģ��֮��ͨ�š�
typedef int    ( * MODULE_INIT ) ( void ) ;
typedef int    ( * MODULE_EXIT ) ( void ) ;

int  core_init( void ) ;
int  core_exit( void ) ;
int  core_printf( const char * fmt , ... ) ;    //�������

/////////////////////////////////////////////////////////////
//
//    ���ߺ���
//
//    1. ��ʱ������
//    2. ��̬����غ���
//    3. �̺߳���
//    4. VxWorksģ�⺯��
//    5. �ڴ�POOL������
//    6. �������Ӻ���
/////////////////////////////////////////////////////////////

///1. ��ʱ��
#define  TIMER_NORMAL          0
#define  TIMER_PERIOD          1

typedef void ( * TIMER_CALLBACK ) ( int message , void * pdata ) ;
int  core_timer_init ( void ) ;
//�������ص������Ķ�ʱ�� 1 tick = 10ms
int  core_timer_start  ( int message  , int ticks , int type , void *pdata , TIMER_CALLBACK func ) ;

//������ָ��APPģ�鷢����Ϣ�Ķ�ʱ��
int  core_timer_message( int receiver , int message , int ticks , int type ) ;
int  core_timer_get_ticks( int timer) ;
void core_timer_stop ( int timer ) ;
void core_timer_reset( int timer ) ;
void core_timer_pause( int timer , int pause ) ;
void core_timer_set  ( int timer , int ticks ) ;


//2. ��̬����غ���
int  core_load_module   ( char * name ) ;
int  core_unload_module ( char * name ) ;
int  core_list_module   ( char * buf  ) ;
int  core_list_thread  ( void  );

//3. �̺߳���
//�����̣߳���������������
typedef  void *( * CORE_THREAD_FUNC )( void* )  ;
int  core_create_thread ( char * name , CORE_THREAD_FUNC func  , void * para ) ;
int  core_wait_thread   ( int id  ) ;
int  core_set_thread_cpu( int cpu ) ; 



//4. VxWorksģ�⺯��
///////////////////////////////////////////////////////////////////////////////
#define  NO_WAIT             0
#define  WAIT_FOREVER       -1
#define  VX_ERROR           -1
#define  VX_OK               0

/*     Function defined for MESSAGE                                          */
#define MSG_PRI_NORMAL     0
#define MSG_PRI_URGENT     1
#define MSG_Q_FIFO         0
#define MSG_Q_PRIORITY     1
typedef  void *    MSG_Q_ID ;

#define  send_message_nowait( q , len , buf )   msgQSend( q , (char*)(buf) , len , NO_WAIT , MSG_PRI_NORMAL )
#define  send_message_wait( q , len , buf )     msgQSend( q , (char*)(buf) , len , NO_WAIT , MSG_PRI_NORMAL )
#define  send_message_urgent( q , len , buf )   msgQSend( q , (char*)(buf) , len , NO_WAIT , MSG_PRI_URGENT  )
#define  receive_message( q , len , buf )       msgQReceive( q , (char*)(buf) , len , WAIT_FOREVER  )

MSG_Q_ID msgQCreate( int maxMsgs, int maxMsgLength,  int notUsed ) ;
int msgQSend       ( MSG_Q_ID msgQId ,  char * buffer,  int nBytes, int timeOut, int notUsed ) ;
int msgQReceive    ( MSG_Q_ID msgQId ,  char * buffer,  int maxNBytes,  int timeOut ) ;
int msgQDelete     ( MSG_Q_ID msgQId ) ;
int msgQNumMsgs    ( MSG_Q_ID msgQId ) ;
int msgQShow       ( MSG_Q_ID msgQId ) ;

///////////////////////////////////////////////////////////////////////////////
/*     Function defined for SEMAPHORE                                        */
#define  SEM_FULL            1
#define  SEM_EMPTY           0
#define  SEM_Q_FIFO          0
#define  SEM_Q_PRIORITY      1
typedef  void *    SEM_ID ;

SEM_ID semBCreate ( int  notUsed,  int initialState ) ;
SEM_ID semCCreate ( int  notUsed,  int initialCount ) ;
SEM_ID semMCreate ( int  notUsed ) ;
int    semGive    ( SEM_ID semId  ) ;
int    semTake    ( SEM_ID semId,  int    timeOut ) ;
int    semDelete  ( SEM_ID semId  ) ;


//5. �ڴ�POOL������
//timeout=-1������ģʽ����λ������
typedef  void *    MEM_POOL_ID ;

//����һ����СΪlength�ڴ�أ����bufΪ�����Զ�����һ���ڴ�
int          core_pool_init   ( void ) ;
MEM_POOL_ID  core_pool_create ( int length , char * buf )     ; 
void         core_pool_destroy( MEM_POOL_ID id ) ; //����һ���ڴ��
void  *      core_pool_alloc  ( MEM_POOL_ID pool , int length , int timeout ) ; 
void         core_pool_free   ( MEM_POOL_ID pool , void *mem  ) ; //�ͷ��ڴ�
int          core_pool_print  ( MEM_POOL_ID pool , char *buf  ) ; //dump pool info



//6. �������Ӻ���
int  recv_n_char( int socket , char * buf , int len  ) ;
int  recv_nn_char( int s , char * buf , int len , int sec) ; //timeout
int  recvfrom_with_timeout( int s , char * buf , int len , int sec ) ;
int  recv_with_timeout( int s , char * buf , int len , int sec ) ;
int  connect_with_timeout( int s , U32 ip , int port , int sec ) ;


#ifdef __cplusplus
    }
#endif

#endif



