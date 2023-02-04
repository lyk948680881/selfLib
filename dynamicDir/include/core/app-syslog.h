

#ifndef __SYSLOG_H__
#define __SYSLOG_H__

#ifdef __cplusplus
    extern "C" {
#endif

#define   SYSLOG_FILE_NAME      "/hivintek/app/system.log"


#define   SYSLOG_MSG           ( APP_SYSLOG << 16 )

#define   SYSLOG_WRITE_MSG     ( SYSLOG_MSG + 1 ) 
#define   SYSLOG_SEARCH_MSG    ( SYSLOG_MSG + 2 ) 


/* 报警日志  */
#define   LT_ALARM                   0x1           /* 报警类型 */
#define   LT_ALARM_START             0x0
#define   LT_ALARM_UNKNOW            0x0           /* 未知报警类型 */
#define   LT_ALARM_IN                0x1           /* 外部报警输入 */
#define   LT_ALARM_OUT               0x2
#define   LT_ALARM_IN_STOP           0x3           /* 外部告警输入结束 */
#define   LT_ALARM_MOTION_DETECTIVE  0x4           /* 发生移动侦测*/
#define   LT_ALARM_MOTION_LOSE       0x5           /* 移动侦测消失*/
#define   LT_ALARM_END               0x5
#define   LT_ALARM_GRAY_IN           0x6
#define   LT_ALARM_GRAY_STOP         0x7

#define   LT_EXCEPTION               0x2           /* 异常报警类型 */
#define   LT_EXP_START               0x20
#define   LT_EXP_UNKNOW              0x20          /* 未知异常类型 */
#define   LT_EXP_VIDEO_LOST          0x21          /* 视频丢失 */
#define   LT_EXP_ILLEGAL_ACCESS      0x22          /* 非法访问 */
#define   LT_EXP_HDD_FULL            0x23          /* 硬盘满 */
#define   LT_EXP_HDD_ERROR           0x24          /* 硬盘出错 */
#define   LT_EXP_SENSOR_RESET        0x25          /* 图像传感器复位 */
#define   LT_EXP_IP_CONFLICT         0x26          /* IP 冲突 */
#define   LT_EXP_NET_BROKEN          0x27          /* 网络断开 */
#define   LT_EXP_VIDEO_COMEBACK      0x28          /* 视频恢复 */
#define   LT_EXP_HDD_INDEX_OVER      0x2A          /* 存储设备索引数超限 */
#define   LT_EXP_END                 0x2A

#define   LT_OPERATION               0x3           /* 操作日志类型 */
#define   LT_OPT_START               0x40          /* */
#define   LT_OPT_UNKNOW              0x40          /* 未知操作类型 */
#define   LT_OPT_SYS_START           0x41          /* 系统启动 */
#define   LT_OPT_SYS_STOP            0x42          /* 系统停止 */
#define   LT_OPT_ILLEGAL_STOP        0x43
#define   LT_OPT_RESTORE_DEFAULT     0x44          /* 恢复默认设置 */
#define   LT_OPT_REMOTE_LOGIN        0x45          /* 远程登陆 */
#define   LT_OPT_REMOTE_LOGOUT       0x46          /* 远程登出 */
#define   LT_OPT_REMOTE_START_REC    0x47          /* 远程启动录像 */
#define   LT_OPT_REMOTE_STOP_REC     0x48          /* 远程停止录像 */
#define   LT_OPT_REMOTE_REBOOT_SYS   0x49          /* 远程重启系统 */
#define   LT_OPT_REMOTE_REBOOT_APP   0x4A          /* 远程重启编码器 */
#define   LT_OPT_REMOTE_GET_PARA     0x4B          /* 远程获取参数 */
#define   LT_OPT_REMOTE_SET_PARA     0x4C          /* 远程设置参数 */
#define   LT_OPT_REMOTE_GET_STATUS   0x4D          /* 远程获取状态 */
#define   LT_OPT_REMOTE_ALARM        0x4E          /* 远程报警 */
#define   LT_OPT_REMOTE_DISABLE_ALM  0x4F          /* 远程取消报警 */
#define   LT_OPT_REMOTE_QUERY_LOG    0x50          /* 远程查询日志 */
#define   LT_OPT_REMOTE_DOWNLOAD_LOG 0x51          /* 远程查询日志 */
#define   LT_OPT_REMOTE_TALK_START   0x52          /* 远程对讲开始 */
#define   LT_OPT_REMOTE_TALK_STOP    0x53          /* 远程对讲结束 */
#define   LT_OPT_REMOTE_UPGRADE      0x54          /* 远程升级 */
#define   LT_OPT_REMOTE_QUERY_DATA   0x55          /* 远程查询文件 */
#define   LT_OPT_REMOTE_DOWNLAOD     0x56          /* 远程下载文件 */
#define   LT_OPT_REMOTE_PTZCTRL      0x57          /* 远程云台控制 */
#define   LT_OPT_REMOTE_START_MON    0x58          /* 远程开始监视 */
#define   LT_OPT_REMOTE_STOP_MON     0x59          /* 远程停止监视 */
#define   LT_OPT_REMOTE_ADD_USER     0x5A          /* 远程添加用户 */
#define   LT_OPT_REMOTE_DEL_USER     0x5B          /* 远程删除用户 */
#define   LT_OPT_REMOTE_QUERY_USER   0x5C          /* 远程查询用户 */
#define   LT_OPT_REMOTE_CHPASS_USER  0x5D          /* 远程修改密码 */
#define   LT_OPT_REMOTE_SET_TIME     0x5E          /* 远程修改日期 */
#define   LT_OPT_NETWORK_UPDATE      0x5F          /* 网络地址更新 */
#define   LT_OPT_REMOTE_CTRL_FOCUS   0x60          /* 远程聚焦 */
#define   LT_OPT_REMOTE_CTRL_FOCUS_SPEED      0x61       /* 远程控制聚焦速度 */
#define   LT_OPT_REMOTE_CTRL_BRIGHTNESS       0x62       /* 远程控制亮度 */
#define   LT_OPT_REMOTE_CTRL_CONTRAST         0x63       /* 远程控制对比度 */
#define   LT_OPT_REMOTE_CTRL_SATURATION       0x64       /* 远程控制饱和度 */
#define   LT_OPT_REMOTE_CTRL_PALLETE          0x65       /* 远程控制色标条 */
#define   LT_OPT_REMOTE_CTRL_PALLETEPOLARITY  0x66       /* 远程控制色标条极性 */
#define   LT_OPT_REMOTE_CTRL_UPER_BOUND       0x67       /* 远程控制色标条上限值 */
#define   LT_OPT_REMOTE_CTRL_LOWER_BOUND      0x68       /* 远程控制色标条下限值 */
#define   LT_OPT_REMOTE_CTRL_ISOTHERM_COLOR   0x69       /* 远程控制等温温度颜色 */
#define   LT_OPT_REMOTE_CTRL_ISOTHERM_TEMP    0x6a       /* 远程控制等温温度     */

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


/*查找日志*/
typedef struct
{
    U32      begin     ;
    U32      end       ;
    int      type      ;    
    int      max       ;     //最大条数
    int      start     ;     //查找开始位置，-1 表示第一次开始查找。
                             //重复调用，可以找出所有符合要求的
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
