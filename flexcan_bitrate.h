/****************************************************************************
 *  flexcan_bitrate.c
 *
 *  This is not my code, I only ported it.
 *  This is taken from Freescale's example bare metal BSP for the i.MX6.
 *  It assumes a 30MHz clock only.  
 ***************************************************************************/
#ifndef FLEXCAN_BITRATE_H__
#define FLEXCAN_BITRATE_H__


enum can_bitrate {
      Mbps_1,
      Kbps_800,
      Kbps_500,
      Kbps_250,
      Kbps_125,
      Kbps_62_5,  /* 62.5kps */
      Kbps_20,
      Kbps_10
};


/**
 *	Port of the Freescale bit rate calculations.
 */
unsigned int 
can_update_bitrate(	unsigned int can_clock_frequency,	/*	Can protocol engine clock, input from CCM */
					enum can_bitrate bitrate);	    /*	Requested Bitrate */


#endif

