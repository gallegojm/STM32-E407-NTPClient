*****************************************************************************
**             NTP client and RTC on STM32-E407 with ChibiOs               **
*****************************************************************************

  My goal is to request the time of a NTP server to set the RTC clock.
  
  I use an Olimex STM32-E407 board.
  It works when compiled with ChibiStudio and chibios 3.0.0p5 but not
with chibios 3.0.0p6

  In file lwipopts.h it is necessary to modify two values:
    #define LWIP_DNS  1
    #define LWIP_SO_RCVTIMEO  1
  
  In file ntp.h, you can configure the list of NTP servers, the delay between
synchronization and the time difference according to your location.
