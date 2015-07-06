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

#include <ntp/ntp.h>
#include <string.h>
#include <console.h>

//  Stack area for the NTP Scheduler thread.
THD_WORKING_AREA( wa_ntp_scheduler, NTP_SCHEDULER_THREAD_STACK_SIZE );

//  Array of server addresses
char * ntpSrvAddr[] = { NTP_SERVER_LIST, NULL };

//  Variables for debugging and statistics
struct ntp_stru ntps;


void SetTimeUnixSec( time_t ut )
{
  RTCDateTime timespec;
  struct tm * ptim;

  ptim = gmtime( & ut );
  rtcConvertStructTmToDateTime( ptim, 0, & timespec );
  rtcSetTime( & RTCD1, & timespec );
}


uint32_t ntpClient( const char * serverAddr )
{
  struct netconn * conn = NULL;
  struct netbuf * buf = NULL;
  char     buffer[ NTP_PACKET_SIZE ];
  uint32_t secsSince1900 = 0;
  char   * pbuf;
  uint16_t buflen;
  uint8_t  mode, stratum;

  ntps.fase = 1;
  ntps.err = 0;

  buf = netbuf_new();
  conn = netconn_new( NETCONN_UDP );
  if( buf == NULL || conn == NULL )
    goto ntpend;

  ntps.fase = 2;
  if( netconn_gethostbyname( serverAddr , & ntps.addr ) != ERR_OK )
    goto ntpend;

  // Initialize message for NTP request
  memset( buffer, 0, NTP_PACKET_SIZE );
  buffer[ 0 ]  = 0b11100011; // LI(clock unsynchronized), Version 4, Mode client
  buffer[ 1 ]  = 0;          // Stratum, or type of clock

  ntps.fase = 3;
  ntps.err = netbuf_ref( buf, buffer, NTP_PACKET_SIZE );
  if( ntps.err != ERR_OK )
    goto ntpend;

  ntps.fase = 4;
  ntps.err = netconn_connect( conn, & ntps.addr, NTP_PORT );
  if( ntps.err != ERR_OK )
    goto ntpend;

  ntps.fase = 5;
  ntps.err = netconn_send( conn, buf );
  if( ntps.err != ERR_OK )
    goto ntpend;

  ntps.fase = 6;
  // netconn_recv create a new buffer so it is necessary to delete the previous buffer
  netbuf_delete( buf );
  // Set receive time out to 3 seconds
  netconn_set_recvtimeout( conn, MS2ST( 3000 ));
  ntps.err = netconn_recv( conn, & buf );
  if( ntps.err != ERR_OK )
    goto ntpend;

  ntps.fase = 7;
  ntps.err = netbuf_data( buf, (void **) & pbuf, & buflen );
  if( ntps.err != ERR_OK )
    goto ntpend;

  // Check length of response
  ntps.fase = 8;
  if( buflen < NTP_PACKET_SIZE )
  {
    ntps.err = buflen;
    goto ntpend;
  }

  // Check Mode (must be Server or Broadcast)
  ntps.fase = 9;
  mode = pbuf[ 0 ] & 0b00000111;
  if( mode != 4 && mode != 5 )
  {
    ntps.err = mode;
    goto ntpend;
  }

  // Check stratum != 0 (kiss-of-death response)
  ntps.fase = 10;
  stratum = pbuf[ 1 ];
  if( stratum == 0 )
    goto ntpend;

  // Combine four high bytes of Transmit Timestamp to get NTP time
  //  (seconds since Jan 1 1900):
  ntps.fase = 0;
  secsSince1900 = pbuf[ 40 ] << 24 | pbuf[ 41 ] << 16 |
                  pbuf[ 42 ] <<  8 | pbuf[ 43 ];

  ntpend:
  if( conn != NULL )
    netconn_delete( conn );
  if( buf != NULL )
    netbuf_delete( buf );
  if( secsSince1900 == 0L )
  {
    DEBUG_PRINT( "NTP error: %i %i\r\n", ntps.fase, ntps.err );
    return 0L;
  }
  return secsSince1900;
}

bool ntpRequest( void )
{
  uint8_t     i = 0;
  uint32_t    secsSince1900 = 0;
  RTCDateTime timespec;
  struct tm   timp;

  // Loop on NTP servers
  while( ntpSrvAddr[ i ] != NULL )
    if(( secsSince1900 = ntpClient( ntpSrvAddr[ i ++ ] )) > 0 )
      break;

  // Exit if no valid response
  if( secsSince1900 == 0 )
    return false;

  // Save current local time
  rtcGetTime( & RTCD1, & timespec );
  rtcConvertDateTimeToStructTm( & timespec, & timp, NULL );
  ntps.unixLocalTime = mktime( & timp );

  // Update RTC
  ntps.lastUnixTime = ntps.unixTime;
  ntps.unixTime = secsSince1900 - NTP_SEVENTY_YEARS + NTP_LOCAL_DIFFERERENCE;
  rtcConvertStructTmToDateTime( gmtime( & ntps.unixTime ), 0, & timespec );
  rtcSetTime( & RTCD1, & timespec );

  return true;
}

// Print statistic

void ntpPrint( void )
{
  uint32_t    elapsedTime;
  uint32_t    lag;
  uint32_t    drift = 0;

  CONSOLE_PRINT( "\n\rRTC updated from NTP server %s\n\r", ipaddr_ntoa( & ntps.addr ));
  CONSOLE_PRINT( "Local time:   %s\r", asctime( gmtime( & ntps.unixLocalTime )));
  CONSOLE_PRINT( "Updated time: %s\r", asctime( gmtime( & ntps.unixTime )));
  if( ntps.lastUnixTime > 0 )
  {
    elapsedTime = ntps.unixTime - ntps.lastUnixTime;
    lag = ntps.unixLocalTime - ntps.unixTime;
    if( lag != 0 )
      drift = 10000 * lag / elapsedTime;
    CONSOLE_PRINT( "Time since last update: %i s\r\n", elapsedTime);
    CONSOLE_PRINT( "Time lag:     %D s\r\n", lag );
    CONSOLE_PRINT( "Drift:        %d.%02u %%\r\n", drift / 100, drift % 100 );
  }
}

/*******************************************************************************
**                                                                            **
**                           NTP Scheduler thread                             **
**                                                                            **
*******************************************************************************/

THD_FUNCTION( ntp_scheduler, p )
{
  (void ) p;

  chRegSetThreadName( "ntp_scheduler" );
  ntps.lastUnixTime = 0;
  chThdSleepSeconds( 5 );

  while( true )
  {
    if( ntpRequest())
    {
      ntpPrint();
      chThdSleepSeconds( NTP_DELAY_SYNCHRO * 60 );
    }
    else
      chThdSleepSeconds( NTP_DELAY_FAILURE * 60 );
  }
}
