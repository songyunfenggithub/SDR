#ifndef __USER_TCPCLIENT_H__
#define __USER_TCPCLIENT_H__
#include "ip_addr.h"
#include "espconn.h"

/******
 * ����tcp����tcp���������IP�Ͷ˿ںš���user_interface.c 22
 */
#define SSID "CMCC-bups"
#define PASSWORD "gktmekfu"

//#define SSID "CMCC-bups2"
//#define PASSWORD "song1234567890"

//#define SSID "CMCC-myphone"
//#define PASSWORD "fb4d67721a92"

void user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length);
void user_tcp_sent_cb(void *arg);
void user_tcp_discon_cb(void *arg);
void user_tcp_send_data(char* pbuf, u16 len);
void user_tcp_connect_cb(void *arg);
void user_tcp_recon_cb(void *arg, sint8 err);
void user_dns_found(const char *name, ip_addr_t *ipaddr, void *arg);
void user_dns_check_cb(void *arg);
void user_check_ip(void);
void user_set_station_config(void);
void tcpuser_init(void);


#endif

