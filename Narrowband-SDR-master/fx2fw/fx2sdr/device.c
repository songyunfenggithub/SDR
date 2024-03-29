/**
 * Copyright (C) 2009 Ubixum, Inc. 
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 **/

#include <fx2regs.h>
#include <fx2macros.h>
#include <fx2ints.h>
#include <autovector.h>
#include <delay.h>
#include <setupdat.h>
#include <eputils.h>
#include <gpif.h>


#ifdef DEBUG_FIRMWARE
#include <stdio.h>
#else
#define printf(...)
#endif

#include "fw.h"
#include "gpif_dat.h"
#include "spi.h"

// mode 0: clk low in idle, data read on rising edge.
#define SPI_MODE ((0 << 1) | (0 << 0))

// IFCLOCK setting for 30MHz uninverted clock
#define IFCLOCK_30M 0xAE
// IFCLOCK setting for 48MHz uninverted clock
#define IFCLOCK_48M 0xEE
BYTE IF_clock_sel = IFCLOCK_30M;


BYTE vendor_command;

volatile WORD ledcounter = 1000;

/*
* These handlers fill out the pre-defs which fx2lib/includes/setupdat.c is looking for:
*
    extern BOOL handle_get_descriptor();
    extern BOOL handle_vendorcommand(BYTE cmd);
    extern BOOL handle_set_configuration(BYTE cfg);
    extern BOOL handle_get_interface(BYTE ifc, BYTE* alt_ifc);
    extern BOOL handle_set_interface(BYTE ifc,BYTE alt_ifc);
    extern BYTE handle_get_configuration();
    extern void handle_reset_ep(BYTE ep); <-- does not appear to ever be used in setupdat.c
*/

BOOL handle_get_descriptor() {
  // by returning FALSE, _handle_get_descriptor in fx2lib/includes/setupdat.c takes over
  return FALSE;
}

//************************** Configuration Handlers *****************************

// change to support as many interfaces as you need
//volatile xdata BYTE interface=0;
//volatile xdata BYTE alt=0; // alt interface

// set *alt_ifc to the current alt interface for ifc
BOOL handle_get_interface(BYTE ifc, BYTE* alt_ifc) {
// *alt_ifc=alt;
 return TRUE;
}
// return TRUE if you set the interface requested
// NOTE this function should reconfigure and reset the endpoints
// according to the interface descriptors you provided.
BOOL handle_set_interface(BYTE ifc,BYTE alt_ifc) {  
	/* We only support interface 0, alternate interface 0. */
	if (ifc != 0 || alt_ifc != 0)
		return FALSE;

	/* Perform procedure from TRM, section 2.3.7: */

	/* (1) TODO. */

	/* (2) Reset data toggles of the EPs in the interface. */
	/* Note: RESETTOGGLE() gets the EP number WITH bit 7 set/cleared. */
	RESETTOGGLE(0x82);

	/* (3) Restore EPs to their default conditions. */
	/* Note: RESETFIFO() gets the EP number WITHOUT bit 7 set/cleared. */
	RESETFIFO(0x02);
	/* TODO */

	/* (4) Clear the HSNAK bit. Not needed, fx2lib does this. */

	return TRUE;
}

// handle getting and setting the configuration
// 1 is the default.  If you support more than one config
// keep track of the config number and return the correct number
// config numbers are set int the dscr file.
//volatile BYTE config=1;
BYTE handle_get_configuration() { 
 return 1;
}

// NOTE changing config requires the device to reset all the endpoints
BOOL handle_set_configuration(BYTE cfg) { 
 printf ( "Set Configuration.\n" );
 //config=cfg;
 return TRUE;
}


//******************* VENDOR COMMAND HANDLERS **************************


BOOL handle_vendorcommand(BYTE cmd) {
  LED2_TOGGLE();

  // check the bRequest byte in the USB setup packet to determine what to do
  if (SETUPDAT[1] == 0xB0) {

    // SETUPDAT[2] is the low byte of wValue, which should contain the channel selection
    SPI_bit_bang_write(SPI_MODE, SETUPDAT[2], EP0BCL, (BYTE*) EP0BUF);
    EP0BCL = 0; // indicate we've read the buffer
    return TRUE;

  } else if (SETUPDAT[1] == 0XB1) {

    // We'll use bRequest == 0xB1 for setting the IFCLOCK for 48 MHz
    // which gives a sample rate of 3 Msps
    IF_clock_sel = IFCLOCK_48M;
    IFCONFIG = IFCLOCK_48M;
    SYNCDELAY;
    EP0BCL = 0; // indicate we've read the buffer/handled the command
    return TRUE;

  } else if (SETUPDAT[1] == 0XB2) {

    // We'll use bRequest == 0xB2 for setting the IFCLOCK for 30 MHz
    // which gives a sample rate of 1.875 Msps
    IF_clock_sel = IFCLOCK_30M;
    IFCONFIG = IFCLOCK_30M;
    SYNCDELAY;
    EP0BCL = 0; // indicate we've read the buffer/handled the command
    return TRUE;

  }

  return FALSE;
}

/**********************************************************
* USB endpoint setup.
**********************************************************
* derived from fx2lafw.c in the Sigrok project
*/
static void setup_endpoints(void)
{
	/* Setup EP2 (IN). */
	EP2CFG = (1u << 7) |		  /* EP is valid/activated */
		 (1u << 6) |		  /* EP direction: IN */
		 (1u << 5) | (0u << 4) |  /* EP Type: bulk */
		 (1u << 3) |		  /* EP buffer size: 1024 */
		 (0u << 2) |		  /* Reserved. */
		 (0u << 1) | (0u << 0);	  /* EP buffering: quad buffering */
	SYNCDELAY;

	/* Disable all other EPs (EP1, EP4, EP6, and EP8). */
	EP1INCFG &= ~bmVALID;
	SYNCDELAY;
	EP1OUTCFG &= ~bmVALID;
	SYNCDELAY;
	EP4CFG &= ~bmVALID;
	SYNCDELAY;
	EP6CFG &= ~bmVALID;
	SYNCDELAY;
	EP8CFG &= ~bmVALID;
	SYNCDELAY;

	/* EP2: Reset the FIFOs. */
	/* Note: RESETFIFO() gets the EP number WITHOUT bit 7 set/cleared. */
	RESETFIFO(0x02);

  // gotta fully disable these otherwise we can't access/control PORTD, and
  // the state of these registers on startup/reset is not consistently 0
	//EP2FIFOCFG = 0;
	//SYNCDELAY;
	EP4FIFOCFG = 0;
	SYNCDELAY;
	EP6FIFOCFG = 0;
	SYNCDELAY;
	EP8FIFOCFG = 0;
	SYNCDELAY;

	/* EP2: Enable AUTOIN mode. Set FIFO width to 8bits. */
	EP2FIFOCFG = bmAUTOIN;
	SYNCDELAY;

	/* EP2: Auto-commit 13*39 = 507 (0x01FB) byte packets (due to AUTOIN = 1). */
	EP2AUTOINLENH = 0x01;
	SYNCDELAY;
	EP2AUTOINLENL = 0xFB;
	SYNCDELAY;

	/* EP2: Set the GPIF flag to 'full'. */
  // we'll set it to assert when GPIF FIFO is full
	EP2GPIFFLGSEL = (1 << 1) | (0 << 1);
	SYNCDELAY;
}

/**********************************************************
* GPIF pre-acquisition preparation
* (additional setup that has to run after gpif_init)
**********************************************************
* derived from gpif-acquisition.c in the Sigrok project
*/
void gpif_acquisition_prepare() {
  /* Ensure GPIF is idle before reconfiguration. */
  while (!(GPIFTRIG & 0x80));

  /* Configure the EP2 FIFO. */
  EP2FIFOCFG = bmAUTOIN;
  SYNCDELAY;

  /* Set IFCONFIG to the correct clock source. */
  //IFCONFIG = 0xEE; // 0xFE for inverted, 0xEE for non-inverted
  // DEBUG: choose 30 MHz clock, inverted
  //IFCONFIG = 0xBE;
  // DEBUG: choose 30 MHz clock, non-inverted
  //IFCONFIG = 0xAE;
  // Use an external IFCLOCK
  //IFCONFIG = (0<<7) | (1<<6) | (0<<5) | (0<<4) | (1<<3) | (1<<2) | (1<<1) | (0<<0); 
  IFCONFIG = IF_clock_sel;
  SYNCDELAY;

  /* Update the status. */
  //gpif_acquiring = PREPARED;

  // stops the GPIF Flag terminating GPIF transfers (there should be enough logic in our waveform to handle this??)
	EP2GPIFPFSTOP = (0 << 0);
  SYNCDELAY;
}


//********************  INIT ***********************
/*
* this comes from fx2lafw_init in the Sigrok fx2lafw project
*/
void main_init() {
  EA = 0; /* global interrupt disable */

	/* Set DYN_OUT and ENH_PKT bits, as recommended by the TRM. */
	REVCTL = bmNOAUTOARM | bmSKIPCOMMIT;

	vendor_command = 0;

	/* Renumerate. */
	RENUMERATE_UNCOND();

	SETCPUFREQ(CLK_48M);

	USE_USB_INTS();

	/* TODO: Does the order of the following lines matter? */
	ENABLE_SUDAV();
	ENABLE_EP2IBN();
	ENABLE_HISPEED();
	ENABLE_USBRESET();

  LED_INIT();
  LED1_ON();
  
  /* Init timer2. */
  // Timer counts up. Interrupt generated after incrementing from TL = 0xFFFF.
  // TL is auto-reloaded with value of RCAP at this point.
  RCAP2L = -500 & 0xff;
  RCAP2H = (-500 & 0xff00) >> 8;
  // Config of T2CON is found on Table 14-5 of TRM
  // TF2 = 0,     clears overflow flag (TF2 triggers interrupt)
  // EXF2 = 0,    clears external trigger (T2EX pin) of Timer 2 interrupt
  // RCLK = 0,    don't use Timer 2 for USART RX
  // TCLK = 0,    don't use Timer 2 for USART TX
  // EXEN2 = 0,   ignore events on T2EX pin
  // TR2 = 0,     stops timer running
  // C/T2 = 0,    {CLKOUT/4, CLKOUT/12} depending on the state of T2M (CKCON.5) or override with CLKOUT/2 if in mode 3
  // CP/RL2 = 0,  auto-reload upon overflow
  // 
  // 16-bit timer with auto-reload
  T2CON = 0;
  ENABLE_TIMER2(); // enable timer2 interrupt
  // start timer 2 running
  TR2 = 1;

	/* Global (8051) interrupt enable. */
	EA = 1;

	/* Setup the endpoints. */
	setup_endpoints();

	/* Put the FX2 into GPIF master mode and setup the GPIF. */
  // set the GPIF data pins to inputs/outputs appropriately
  gpif_init(WaveData, InitData); // this data comes from gpif_dat.c
  gpif_setflowstate(FlowStates, 2); // our waveform is in bank 2
  gpif_acquisition_prepare();

  // sets the output lines as outputs, and their initial logic levels, etc.
  SPI_init(SPI_MODE);
}


void main_loop() {
 // do some work
}


void timer2_isr(void) __interrupt TF2_ISR
{
	/* Blink LED during acquisition, keep it on otherwise. */
	//if (gpif_acquiring == RUNNING) {
		if (--ledcounter == 0) {
			LED1_TOGGLE();
			ledcounter = 1000;
		}
	//} else if (gpif_acquiring == STOPPED) {
	//	LED1_ON();
	//}

  // clear TF2 and EXF2 interrupt flags
	CLEAR_TIMER2();
}

/* IN BULK NAK - the host started requesting data. */
void ibn_isr(void) __interrupt IBN_ISR
{
	/*
	 * If the IBN interrupt is not disabled, clearing
	 * does not work. See AN78446, 6.2.
	 */
	BYTE ibnsave = IBNIE;
	IBNIE = 0;
	CLEAR_USBINT();

	/*
	 * If the host sent the START command, start the GPIF
	 * engine. The host will repeat the BULK IN in the next
	 * microframe.
	 */
	if ((IBNIRQ & bmEP2IBN)) { // && (gpif_acquiring == PREPARED)) {
		//ledcounter = 1;
		//LED1_OFF();
		//gpif_acquisition_start();
    if ( GPIFTRIG & 0x80 ) { // GPIF is ready
      // make the LEDs blink very slowly
      RCAP2L = -3000 & 0xff;
      RCAP2H = (-3000 & 0xff00) >> 8;

      /* gpif waveform will keep going (pausing only when FIFO is full) 
      until the transaction count is depleted. After TC is depleted there will be
      a delay in ADC reads until the buffers empty and this interrupt fires again.
      TODO: can we periodically reset TC while waveform is running? */
      gpif_set_tc32(13*(0xFFFFFFFF/13)); // highest multiple of 13 to fit in 32 bits
      gpif_fifo_read(0); // start reading into EP 2 (index 0)
    } 
	}

	/* Clear IBN flags for all EPs. */
  IBNIRQ = 0xff;

	NAKIRQ = bmIBN;
	SYNCDELAY;

	IBNIE = ibnsave;
	SYNCDELAY;
}
