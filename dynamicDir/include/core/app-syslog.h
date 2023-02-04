

#ifndef __SYSLOG_H__
#define __SYSLOG_H__

#ifdef __cplusplus
    extern "C" {
#endif

#define   SYSLOG_FILE_NAME      "/hivintek/app/system.log"


#define   SYSLOG_MSG           ( APP_SYSLOG << 16 )

#define   SYSLOG_WRITE_MSG     ( SYSLOG_MSG + 1 ) 
#define   SYSLOG_SEARCH_MSG    ( SYSLOG_MSG + 2 ) 


/* ������־  */
#define   LT_ALARM                   0x1           /* �������� */
#define   LT_ALARM_START             0x0
#define   LT_ALARM_UNKNOW            0x0           /* δ֪�������� */
#define   LT_ALARM_IN                0x1           /* �ⲿ�������� */
#define   LT_ALARM_OUT               0x2
#define   LT_ALARM_IN_STOP           0x3           /* �ⲿ�澯������� */
#define   LT_ALARM_MOTION_DETECTIVE  0x4           /* �����ƶ����*/
#define   LT_ALARM_MOTION_LOSE       0x5           /* �ƶ������ʧ*/
#define   LT_ALARM_END               0x5
#define   LT_ALARM_GRAY_IN           0x6
#define   LT_ALARM_GRAY_STOP         0x7

#define   LT_EXCEPTION               0x2           /* �쳣�������� */
#define   LT_EXP_START               0x20
#define   LT_EXP_UNKNOW              0x20          /* δ֪�쳣���� */
#define   LT_EXP_VIDEO_LOST          0x21          /* ��Ƶ��ʧ */
#define   LT_EXP_ILLEGAL_ACCESS      0x22          /* �Ƿ����� */
#define   LT_EXP_HDD_FULL            0x23          /* Ӳ���� */
#define   LT_EXP_HDD_ERROR           0x24          /* Ӳ�̳��� */
#define   LT_EXP_SENSOR_RESET        0x25          /* ͼ�񴫸�����λ */
#define   LT_EXP_IP_CONFLICT         0x26          /* IP ��ͻ */
#define   LT_EXP_NET_BROKEN          0x27          /* ����Ͽ� */
#define   LT_EXP_VIDEO_COMEBACK      0x28          /* ��Ƶ�ָ� */
#define   LT_EXP_HDD_INDEX_OVER      0x2A          /* �洢�豸���������� */
#define   LT_EXP_END                 0x2A

#define   LT_OPERATION               0x3           /* ������־���� */
#define   LT_OPT_START               0x40          /* */
#define   LT_OPT_UNKNOW              0x40          /* δ֪�������� */
#define   LT_OPT_SYS_START           0x41          /* ϵͳ���� */
#define   LT_OPT_SYS_STOP            0x42          /* ϵͳֹͣ */
#define   LT_OPT_ILLEGAL_STOP        0x43
#define   LT_OPT_RESTORE_DEFAULT     0x44          /* �ָ�Ĭ������ */
#define   LT_OPT_REMOTE_LOGIN        0x45          /* Զ�̵�½ */
#define   LT_OPT_REMOTE_LOGOUT       0x46          /* Զ�̵ǳ� */
#define   LT_OPT_REMOTE_START_REC    0x47          /* Զ������¼�� */
#define   LT_OPT_REMOTE_STOP_REC     0x48          /* Զ��ֹͣ¼�� */
#define   LT_OPT_REMOTE_REBOOT_SYS   0x49          /* Զ������ϵͳ */
#define   LT_OPT_REMOTE_REBOOT_APP   0x4A          /* Զ������������ */
#define   LT_OPT_REMOTE_GET_PARA     0x4B          /* Զ�̻�ȡ���� */
#define   LT_OPT_REMOTE_SET_PARA     0x4C          /* Զ�����ò��� */
#define   LT_OPT_REMOTE_GET_STATUS   0x4D          /* Զ�̻�ȡ״̬ */
#define   LT_OPT_REMOTE_ALARM        0x4E          /* Զ�̱��� */
#define   LT_OPT_REMOTE_DISABLE_ALM  0x4F          /* Զ��ȡ������ */
#define   LT_OPT_REMOTE_QUERY_LOG    0x50          /* Զ�̲�ѯ��־ */
#define   LT_OPT_REMOTE_DOWNLOAD_LOG 0x51          /* Զ�̲�ѯ��־ */
#define   LT_OPT_REMOTE_TALK_START   0x52          /* Զ�̶Խ���ʼ */
#define   LT_OPT_REMOTE_TALK_STOP    0x53          /* Զ�̶Խ����� */
#define   LT_OPT_REMOTE_UPGRADE      0x54          /* Զ������ */
#define   LT_OPT_REMOTE_QUERY_DATA   0x55          /* Զ�̲�ѯ�ļ� */
#define   LT_OPT_REMOTE_DOWNLAOD     0x56          /* Զ�������ļ� */
#define   LT_OPT_REMOTE_PTZCTRL      0x57          /* Զ����̨���� */
#define   LT_OPT_REMOTE_START_MON    0x58          /* Զ�̿�ʼ���� */
#define   LT_OPT_REMOTE_STOP_MON     0x59          /* Զ��ֹͣ���� */
#define   LT_OPT_REMOTE_ADD_USER     0x5A          /* Զ������û� */
#define   LT_OPT_REMOTE_DEL_USER     0x5B          /* Զ��ɾ���û� */
#define   LT_OPT_REMOTE_QUERY_USER   0x5C          /* Զ�̲�ѯ�û� */
#define   LT_OPT_REMOTE_CHPASS_USER  0x5D          /* Զ���޸����� */
#define   LT_OPT_REMOTE_SET_TIME     0x5E          /* Զ���޸����� */
#define   LT_OPT_NETWORK_UPDATE      0x5F          /* �����ַ���� */
#define   LT_OPT_REMOTE_CTRL_FOCUS   0x60          /* Զ�̾۽� */
#define   LT_OPT_REMOTE_CTRL_FOCUS_SPEED      0x61       /* Զ�̿��ƾ۽��ٶ� */
#define   LT_OPT_REMOTE_CTRL_BRIGHTNESS       0x62       /* Զ�̿������� */
#define   LT_OPT_REMOTE_CTRL_CONTRAST         0x63       /* Զ�̿��ƶԱȶ� */
#define   LT_OPT_REMOTE_CTRL_SATURATION       0x64       /* Զ�̿��Ʊ��Ͷ� */
#define   LT_OPT_REMOTE_CTRL_PALLETE          0x65       /* Զ�̿���ɫ���� */
#define   LT_OPT_REMOTE_CTRL_PALLETEPOLARITY  0x66       /* Զ�̿���ɫ�������� */
#define   LT_OPT_REMOTE_CTRL_UPER_BOUND       0x67       /* Զ�̿���ɫ��������ֵ */
#define   LT_OPT_REMOTE_CTRL_LOWER_BOUND      0x68       /* Զ�̿���ɫ��������ֵ */
#define   LT_OPT_REMOTE_CTRL_ISOTHERM_COLOR   0x69       /* Զ�̿��Ƶ����¶���ɫ */
#define   LT_OPT_REMOTE_CTRL_ISOTHERM_TEMP    0x6a       /* Զ�̿��Ƶ����¶�     */

#define   LT_OPT_END                 0x6a



/* struct define for runtime log  and alarm log */
typedef struct _tagLOG_RECORD  //16 bytes length
{
    U32         log_time   ;
    U8          major_type ;
    U8          minor_type ;
    U16         user_no    ;
    U32         remote_ip  ;
    U32         para       ;
}LOG_RECORD  ;


/*������־*/
typedef struct
{
    U32      begin     ;
    U32      end       ;
    int      type      ;    
    int      max       ;     //�������
    int      start     ;     //���ҿ�ʼλ�ã�-1 ��ʾ��һ�ο�ʼ���ҡ�
                             //�ظ����ã������ҳ����з���Ҫ���
}SYSLOG_SEARCH         ;


CORE_SERVICE * syslog_service   ( void ) ;


#define  write_alarm_log(type, user,para)            write_system_log( LT_ALARM    , type, user, para ,0)
#define  write_exception_log(type, user, para)       write_system_log( LT_EXCEPTION, type, user, para, 0)
#define  write_operation_log(type, user, para, ip)   write_system_log( LT_OPERATION, type, user, para ,ip)

int  write_system_log ( int major , int minor , int user , int para , int ip ) ;
int  search_system_log( SYSLOG_SEARCH * search , LOG_RECORD * log ) ;


#ifdef __cplusplus
    }
#endif

#endif
