#ifndef _HIVINTEK_SDK_
#define _HIVINTEK_SDK_

/* �û� gui ��� */
#define MAX_USER_GUI_WIDTH     720
/* �û� gui �߶� */
#define MAX_USER_GUI_HEIGHT    576

//ѹ������֡��ʽ
typedef struct
{    
    int channel    ;   //ͨ����
    int type       ;   //֡����
    unsigned long long int  pts        ;   //ʱ���
    int length     ;   //��С    
    int width      ;   //���
    int height     ;   //�߶�            
    int fps        ;   //֡��
    int sample     ;   //��Ƶ������  ������
    
    int phy_cp     ;   // �����ַ���û�����
    void *ref      ;   // �����  �û�����    
    void * stream  ;   //stream��ַ�����0�Ļ�
    
    
    int phy_addr[3] ;  
    int vir_addr[3] ; 
    int stride[3]   ; 
        
    int rec_stat   ;   //�Ҷ����ݴ�usb�����������rec������pwm rec��״̬
    int snap_count ;   //�Ҷ����ݴ�usb�����������snap����������pwm snap �������ۼ�
    
    //�����������
    int nuc_back ;
    int K_para ;
    int M_para ;
    int C_para ;
    int vtemp ;
    char * mtlib ;
        
}FRAME_INFO ;


///////////////////////////////////////////////////////////////////
//�ص���ȥ��֡���ͣ�ע��ʹ�÷���
typedef struct _tagHIVINTEK_FR
{
    //�ڲ�����
    int            reference ;       
    FRAME_INFO     frame  ;    
    struct _tagHIVINTEK_FR * next;   //������
}HIVINTEK_FRAME ;

//�ص��������Ͷ���
typedef  int ( * FUNC_HIVINTEK_CALLBACK ) ( HIVINTEK_FRAME * fr ,  void * para ) ;

#define MAX_USER_GUI_IDX    5 //��󻭲���

/* ����OSD�����С��Ӱ��������ı���С  */
int hivintek_gui_font_size( int size ) ;

/*  ���õ�ǰ�Ĳ��������������Ļ�����ض����������ڵ�ǰ������ ��ȱʡ����idx=0 */
int hivintek_gui_set_current( int idx ) ;
int hivintek_gui_get_current( void    ) ; //��ȡ��ǰ�Ļ���

/* ����OSD������С ��ȱʡΪ720*576����xxx_gui_init֮ǰ������Ч ��֮����Ч */
int hivintek_gui_size( int w , int h );

/* gui ��ʼ������ �� ʹ�����º���֮ǰһ��Ҫ���øú��� */
int hivintek_gui_init( );

/* ��� gui�ϵ��������� */
int hivintek_gui_clear( );

/*����OSD����λ�� ��ȱʡΪ(0,0)�����Զ�̬����*/
int hivintek_gui_position( int x , int y ) ;


/* ������
   ����
   x �� y : ���ϵ�����
   w �� h : ��Ŀ�Ⱥ͸߶�
   fill   : �����Ƿ���䣬0: ����� 1 : ���
   fcolor : rgb1555���͵���ɫ
*/
int hivintek_gui_draw_rect( int x , int y, int w, int h,  int fill, int fcolor );

/* ���ַ�������
   ����
   text   : Ҫд���ַ�����Ҫ����32��
   x �� y : ���ϵ�����
   fcolor : rgb1555���͵���ɫ
*/
int hivintek_gui_draw_text( char *text , int x, int y , int fcolor);

/* ��Բ����
   ����
   x �� y : Բ������
   r : Բ�İ뾶
   fill   : �����Ƿ���䣬0: ����� 1 : ���
   fcolor : rgb1555���͵���ɫ
*/
int hivintek_gui_draw_circle ( int x, int y, int r, int fill, int fcolor );



/* ��������
   ����
   x0 �� y0 : ֱ���������
   x1 ,   y1 : ֱ���յ�����
   fcolor : rgb1555���͵���ɫ
*/
int hivintek_gui_draw_line ( int x0, int y0, int x1, int y1 , int fcolor );

/* ����
   ����
   x �� y : �������
   fcolor : rgb1555���͵���ɫ
*/
int hivintek_gui_draw_pixel ( int x, int y, int fcolor );

/*
   ��׼���õ�guiͼƬ�ŵ���������ʾ
*/
int hivintek_gui_update( );

/*
   ע��һ���ص��������ײ��б�������֡ʱ����ô˻ص�����
   ����
   cb  �ص��������ص���������ò�Ҫռ��ʱ����ã�����Ӱ��ȡ��
   para Ҫ�����ص������Ĳ�����
*/
int hivintek_register_venc_callback( FUNC_HIVINTEK_CALLBACK cb, void *para );

/*
   ע��һ���ص��������ײ���jpegͼƬʱ����ô˻ص�����
   ����
   cb  �ص��������ص���������ò�Ҫռ��ʱ����ã�����Ӱ��ȡ��
   para Ҫ�����ص������Ĳ�����
*/
int hivintek_register_jpeg_callback( FUNC_HIVINTEK_CALLBACK cb, void *para );

/*
   ע��һ���ص��������ײ���ԭʼ�Ҷ�����ʱ����ô˻ص�����
   ����
   cb  �ص��������ص���������ò�Ҫռ��ʱ����ã�����Ӱ��ȡ��
   para Ҫ�����ص������Ĳ�����
*/
int hivintek_register_frame_callback( FUNC_HIVINTEK_CALLBACK cb, void *para );


/*
   ע��һ���ص��������ײ��лҶ���Ƶ����ʱ����ô˻ص�����
   ����
   cb  �ص��������ص���������ò�Ҫռ��ʱ����ã�����Ӱ��ȡ��
   para Ҫ�����ص������Ĳ�����
*/
int hivintek_register_video_callback( FUNC_HIVINTEK_CALLBACK cb, void *para );

/*
  ��������֡���Ա㴦�������
*/
int hivintek_lock_frame( HIVINTEK_FRAME *frame );

/*
  ��������֡���Ա㴦�������
*/

int hivintek_unlock_frame( HIVINTEK_FRAME *frame );



#ifndef _USE_INSIDE_
/*ģ�����Ϣ�ṹ����ͷ������*/
typedef struct
{
    int  sender   ;      //����ģ��
    int  receiver ;      //����ģ��
    int  command  ;      //������
    int  length   ;      //���ݳ���
}MESSAG_HEADER2 ;

typedef struct
{
    int  sender   ;      //����ģ��
    int  receiver ;      //����ģ��
    int  command  ;      //������
    int  length   ;      //���ݳ���
    char data[128] ;     //���ݣ���һ��Ϊ128�ֽ�
}KERNEL_MESSAGE2   ;


#endif

//��Ϣ���о��
typedef void *   HIVINTEK_MSG_Q;
//������Ϣ����
HIVINTEK_MSG_Q *hivintek_create_msg_q ( int maxMsgLen );

/**********************************
    ������Ϣ����Ϣ����
    ����
    q : ���о��
    len : Ҫ���͵���Ϣ����
    msg : Ҫ���͵���Ϣ
***********************************/
int             hivintek_send_message( HIVINTEK_MSG_Q *q, int len, char *msg );

/**********************************
    ����Ϣ���н�����Ϣ
    ����
    q : ���о��
    len : Ҫ���ܵ���Ϣ��󳤶�
    msg : ���ܵ�����Ϣ
***********************************/
int             hivintek_receive_message( HIVINTEK_MSG_Q *q, int len, char *msg );

//�̺߳�������
typedef  void *( * HIVINTEK_THREAD_FUNC )( void* )  ;
//����һ���߳�
int hivintek_create_thread ( char *name, HIVINTEK_THREAD_FUNC func, void *para );



//�����ڲ�ģ��ֱ�Ӵ�������
typedef struct _tagHIVINTEK_SERVICE
{
    int ( * service_message ) ( void * priv , int sender , void * msg ) ;
    int ( * service_command ) ( void * priv , int op , int len , void * ibuf , void * obuf ) ;
            
    void * private_data ;       //˽�б���
    
}HIVINTEK_SERVICE ;


//��������˽�����ݹҵ�ϵͳ��
//���� 0 : ʧ�� �� 1 :�ɹ�
int hivintek_register_service ( char *name, int id, HIVINTEK_SERVICE *service );

/**********************************
    ���ͱ�����Ϣ����Ϣ������������÷������ٷ��͸�����
    ����
    0 : ʧ��
    1 : �ɹ�
    ����
    alarm : �������ݣ��û����ж���
    len : Ҫ���͵ı������ݵĳ��ȣ��128�ֽ�
***********************************/
int hivintek_alarm_message ( void *alarm , int len  );



#endif
