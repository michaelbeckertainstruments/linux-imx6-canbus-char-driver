/****************************************************************************
 *  flexcan_bitrate.c
 *
 *  This is not my code, I only ported it.
 *  This is taken from Freescale's example bare metal BSP for the i.MX6.
 *  It assumes a 30MHz peripheral clock only.  
 *
 ***************************************************************************/
#include "can_private.h"


struct time_segment_t {

    unsigned int propseg;
    unsigned int pseg1;
    unsigned int pseg2;

};


/*
 *  possible time segment settings for propseg, pseg1, pseg2
 */
static const struct time_segment_t time_segments[18] = {
    {1, 2, 1},  /* 0: total 8 timequanta */
    {1, 2, 2},  /* 1: total 9 timequanta */
    {2, 2, 2},  /* 2: total 10 timequanta */
    {2, 2, 3},  /* 3: total 11 timequanta */
    {2, 3, 3},  /* 4: total 12 timequanta */
    {3, 3, 3},  /* 5: total 13 timequanta */
    {3, 3, 4},  /* 6: total 14 timequanta */
    {3, 4, 4},  /* 7: total 15 timequanta */
    {4, 4, 4},  /* 8: total 16 timequanta */
    {4, 4, 5},  /* 9: total 17 timequanta */
    {4, 5, 5},  /* 10: total 18 timequanta */
    {5, 5, 5},  /* 11: total 19 timequanta */
    {5, 5, 6},  /* 12: total 20 timequanta */
    {5, 6, 6},  /* 13: total 21 timequanta */
    {6, 6, 6},  /* 14: total 22 timequanta */
    {6, 6, 7},  /* 15: total 23 timequanta */
    {6, 7, 7},  /* 16: total 24 timequanta */
    {7, 7, 7}   /* 17: total 25 timequanta */
};


/**
 *  We won't touch HW here, only do the calculation / lookup.
 */
unsigned int
can_update_bitrate( unsigned int can_clock_frequency,   /*  Can protocol engine clock, input from CCM   */
                    enum can_bitrate bitrate)           /*  Requested Bitrate */
{
    unsigned int reg;
    unsigned int presdiv;   /* Clock pre-divider */
    struct time_segment_t ts;
    int valid = 1;

    if (can_clock_frequency == 30000000){

        switch (bitrate){

            case Mbps_1: 
                presdiv = 1;            /* sets sclk ftq to 15MHz = PEclk/2 */
                ts = time_segments[7];  /* 15 time quanta (15-8=7 for array ID) */
                break;

            case Kbps_800:
                presdiv = 1;            /* sets sclk ftq to 15MHz = PEclk/2 */
                ts = time_segments[11]; /* 19 time quanta (19-8=11 for array ID) */
                break;

            case Kbps_500:
                presdiv = 2;            /* sets sclk ftq to 10MHz = PEclk/3 */
                ts = time_segments[12]; /* 20 time quanta (20-8=12 for array ID) */
                break;   

            case Kbps_250:
                presdiv = 4;            /* sets sclk ftq to 6MHz = PEclk/5 */
                ts = time_segments[16]; /* 24 time quanta (24-8=16 for array ID) */
                break;

            case Kbps_125:
                presdiv = 9;            /* sets sclk ftq to 3MHz = PEclk/10 */
                ts = time_segments[16]; /* 24 time quanta (24-8=16 for array ID) */
                break;

            case Kbps_62_5:             /*  62.5kps  */
                presdiv = 19;           /* sets sclk ftq to 1.5MHz = PEclk/20 */
                ts = time_segments[16]; /* 24 time quanta (24-8=16 for array ID) */
                break;

            case Kbps_20:
                presdiv = 59;           /* sets sclk ftq to 500KHz = PEclk/60  */
                ts = time_segments[17]; /* 25 time quanta (25-8=17 for array ID) */
                break;

            case Kbps_10:
                presdiv = 119;          /* sets sclk ftq to 500KHz = PEclk/60 */
                ts = time_segments[17]; /* 25 time quanta (25-8=17 for array ID) */
                break;

            default:
                valid = 0;
                presdiv = 0; 
                ts = time_segments[0]; 
                printk(KERN_ERR "CAN bitrate not supported\n");
                break;       
        }
    }
    else { 
        valid = 0;
        presdiv = 0; 
        ts = time_segments[0]; 
        printk(KERN_ERR "CAN PE_CLK input to CAN module speed not supported\n");
    }
    

    if (valid){
        reg = (presdiv << 24) + (ts.pseg1 << 19) + (ts.pseg2 << 16) + (ts.propseg);
        printk( KERN_INFO 
                "CAN bitrate setup presdiv: %d  "
                "pseg1: 0x%x  pseg2: 0x%x  propseg: 0x%x\n",
                presdiv, ts.pseg1, ts.pseg2, ts.propseg);
    }
    else {
        reg = 0;
    }

    return reg;
}


