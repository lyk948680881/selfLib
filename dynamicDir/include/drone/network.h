/****************************************************************************
    Shanghai Pallas Digital Tech. Ltd,  (C) 2002 , All rights are reserved
    Description:
          ÍøÂç¹¤¾ßÄ£¿é
         
****************************************************************************/

#ifndef __NETWORK_TOOL_H__
#define __NETWORK_TOOL_H__



#define   APP_NET_MSG         ( APP_NETWORK << 16 )

//Message for Network
#define   NET_SNTP_UPDATE     ( APP_NET_MSG + 0 ) 
#define   NET_DDNS_START      ( APP_NET_MSG + 1 ) 
#define   NET_DDNS_STOP       ( APP_NET_MSG + 2 ) 
#define   NET_HTTP_START      ( APP_NET_MSG + 3 )  
#define   NET_HTTP_STOP       ( APP_NET_MSG + 4 )  
#define   NET_FTP_SERVER_START    ( APP_NET_MSG + 5 )  
#define   NET_FTP_SERVER_STOP     ( APP_NET_MSG + 6 )  

#define   NET_DHCP_START      ( APP_NET_MSG + 10 ) 
#define   NET_DHCP_STOP       ( APP_NET_MSG + 11 ) 
#define   NET_PPPOE_START     ( APP_NET_MSG + 12 ) 
#define   NET_PPPOE_STOP      ( APP_NET_MSG + 13 ) 
#define   NET_MANUAL_CFG      ( APP_NET_MSG + 18 ) 
#define   NET_NTP_CFG         ( APP_NET_MSG + 19 ) 
#define   NET_DDNS_CFG        ( APP_NET_MSG + 20 ) 

#define   NET_DHCP_IP_CHANGED      ( APP_NET_MSG + 0x20 )
#define   NET_PPPOE_IP_CHANGED     ( APP_NET_MSG + 0x21 )
#define   NET_PING_IP_CHANGED      ( APP_NET_MSG + 0x22 )

#define   NET_GET_INFO          ( APP_NET_MSG + 0x30 )
#define   NET_GET_STATUS        ( APP_NET_MSG + 0x31 )

#define   NET_FORCE_RELOAD      ( APP_NET_MSG + 0x40 ) 
#define   NET_SNTP_TIMER_OUT    ( APP_NET_MSG + 0x80 ) 


typedef struct
{
    MESSAG_HEADER  head ;    
}NETWORK_MESSAGE ;


typedef struct
{
    char mac_addr[32] ;
    U32  ip_addr  ;
    U32  ip_mask  ;
    U32  gateway  ;
    U32  dns[2]   ;         
    int  http_port;    
}NETWORK_INFO ;

typedef struct
{
    int   dhcp     ;      //1:OK 0:OFF  -1:Error
    int   eth_ip   ;
    int   eth_mask ;
    int   eth_gw   ;
    char  eth_mac[32] ;
    
    int   pppoe      ;    //1:OK 0:OFF  -1:Error
    int   ppp_ip     ;
    int   ppp_mask   ;
    int   ppp_gw     ; 
}NETWORK_STATUS ;


#endif
