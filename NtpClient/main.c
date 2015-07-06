/*
 *
 *  NTP Client on STM32-E407 with ChibiOs
 *
 *  Copyright (c) 2015 by Jean-Michel Gallego
 *
 *  Please read file ReadMe.txt for instructions
 *
 *  This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>

#include "ch.h"
#include "hal.h"

#include "usbser.h"
#include "console.h"

#include "chprintf.h"

#include "lwipthread.h"

#include "ntp/ntp.h"

/*===========================================================================*/
/* Main and generic code.                                                    */
/*===========================================================================*/

/*
 * Green LED blinker thread, times are in milliseconds.
 */

static THD_WORKING_AREA(waThread1, 128);

static THD_FUNCTION(Thread1, arg)
{
  (void) arg;
  chRegSetThreadName("blinker");
  while (true)
  {
    palTogglePad(GPIOC, GPIOC_LED);
    chThdSleepMilliseconds( 500 );
  }
}

/*
 * Application entry point.
 */

// String to store the actual date/time is declared out of main function
//   so that I can display it in the STM32 ST-LINK Utility

char strTime[ 16 ];

int main( void )
{
  halInit();
  chSysInit();

  rtcSTM32SetPeriodicWakeup( &RTCD1, NULL );

  // Initializes a serial-over-USB CDC driver.
  sduObjectInit(&SDU2);
  sduStart(&SDU2, &serusbcfg);

  // Activates the USB driver and then the USB bus pull-up on D+.
  usbDisconnectBus(serusbcfg.usbp);
  chThdSleepMilliseconds(1500);
  usbStart(serusbcfg.usbp, &usbcfg);
  usbConnectBus(serusbcfg.usbp);

  // Creates the blinker thread.
  chThdCreateStatic( waThread1, sizeof( waThread1 ), NORMALPRIO, Thread1, NULL );

  // Creates the LWIP threads (it changes priority internally).
  chThdCreateStatic( wa_lwip_thread, LWIP_THREAD_STACK_SIZE, NORMALPRIO + 2,
                     lwip_thread, NULL );

  // Create the NTP scheduler thread
  chThdCreateStatic( wa_ntp_scheduler, sizeof( wa_ntp_scheduler ),
                     NTP_SCHEDULER_THREAD_PRIORITY, ntp_scheduler, NULL);

  // Normal main() thread activity

  RTCDateTime timespec;
  struct tm stm;

  while( true )
  {
    chThdSleepMilliseconds( 1000 );

    rtcGetTime( & RTCD1, & timespec );
    rtcConvertDateTimeToStructTm( & timespec, & stm, NULL );
    chsnprintf( strTime, 16, "%02u:%02u:%02u", stm.tm_hour, stm.tm_min, stm.tm_sec );
    //chprintf( (BaseSequentialStream *) & SDU2, "%s\r", strTime );
  }
}
