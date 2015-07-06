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

#ifndef _NTP_H_
#define _NTP_H_

#include "ch.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"

#define NTP_VERSION                     "2015-07-04"

#define NTP_PORT                        123

#define NTP_SERVER_LIST "0.south-america.pool.ntp.org", \
                        "1.south-america.pool.ntp.org", \
                        "2.south-america.pool.ntp.org", \
                        "0.north-america.pool.ntp.org"
/*
#define NTP_SERVER_LIST "venezuela.pool.ntp.org", \
                        "0.south-america.pool.ntp.org", \
                        "0.north-america.pool.ntp.org"
*/

// Number of seconds to add to get unix local time
//  ( minus 4 hours and half for Venezuela )
#define NTP_LOCAL_DIFFERERENCE          - 60 * ( 4 * 60 + 30 )

// Delay between two synchronization
#define NTP_DELAY_SYNCHRO               10

// Delay in minutes between retries when failure
#define NTP_DELAY_FAILURE               2

// Number of seconds between January 1900 and January 1970 (Unix time)
#define NTP_SEVENTY_YEARS               2208988800UL

#define NTP_PACKET_SIZE                 48

#define NTP_SCHEDULER_THREAD_STACK_SIZE 768

#define NTP_SCHEDULER_THREAD_PRIORITY   (LOWPRIO + 2)

// define a structure of parameters for debugging and statistics
struct ntp_stru
{
  ip_addr_t addr;
  time_t    unixLocalTime;
  time_t    unixTime;
  uint32_t  lastUnixTime;
  uint8_t   fase;
  uint8_t   err;
};

extern THD_WORKING_AREA( wa_ntp_scheduler, NTP_SCHEDULER_THREAD_STACK_SIZE );

#ifdef __cplusplus
extern "C" {
#endif
  THD_FUNCTION( ntp_scheduler, p );
#ifdef __cplusplus
}
#endif

#endif // _NTP_H_
