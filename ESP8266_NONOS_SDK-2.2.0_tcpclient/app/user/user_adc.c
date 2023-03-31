/*
 * user_adc.c
 *
 *  Created on: 2017年7月5日
 *      Author: Administrator
 */


#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"

#include "user_adc.h"
#include "user_tcpclient.h"

//#define ADC_READ_INTERVAL	3000
#define ADC_READ_INTERVAL	5

static os_timer_t adc_timer;

#define ADCBUFLEN	64
u16 adc_buf[ADCBUFLEN];
u16 adc_buf_i = 0;
u32 adc_send_i = 0;

void ICACHE_FLASH_ATTR
adc_timer_cb(void *arg)
{
	//static u32 oldt = 0;
	//u32 a = system_get_time();
	adc_buf[adc_buf_i++] = system_adc_read();
	//u32 b = system_get_time();
	//adc_buf[adc_buf_i++] = b - a;

	//adc_buf[adc_buf_i - 2] = adc_buf_i - 2;
	//adc_buf[adc_buf_i - 1] = adc_buf_i - 1;

	if(adc_buf_i == ADCBUFLEN)
	{
		adc_buf_i = 0;
		user_tcp_send_data((char*)adc_buf, ADCBUFLEN*2);
		//os_printf("%d\n", a - oldt);
		//if(!(adc_send_i & 0xF))os_printf("%d\n", adc_send_i);
		adc_send_i++;
	}
	//oldt  = a;
}

void ICACHE_FLASH_ATTR
user_adc_init(void)
{
    os_timer_disarm(&adc_timer);
    os_timer_setfn(&adc_timer, (os_timer_func_t *) adc_timer_cb, NULL);
    os_timer_arm(&adc_timer, ADC_READ_INTERVAL, 1);
}

void ICACHE_FLASH_ATTR
user_adc_deinit(void)
{
    os_timer_disarm(&adc_timer);
}
