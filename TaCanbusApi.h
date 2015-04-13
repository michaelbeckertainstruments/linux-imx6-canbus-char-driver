/**
 *  This is the public API between the CANbus driver and any application.
 *
 *  Naming conventions are different in this file from the kernel driver code.
 *  This reflects the reality that this entire project was originally a 
 *  Windows Embedded Compact 7 project, and we converetd it to Linux. While 
 *  it made good sense to follow all kernel naming conventions within the 
 *  driver itself, there wasn't time to convert all of the 
 *  user space code, which is heavily dependent on the CANBUS_MESSAGE 
 *  (and friends) structure.
 *  
 *  This header is NOT the same as the Protocol API.  The Protocol API 
 *  builds on this one.
 */
#ifndef TA_CANBUS_API_H__
#define TA_CANBUS_API_H__


#ifdef __cplusplus
extern "C" {
#endif

#include <linux/ioctl.h>


#define CAN_MAGIC_TYPE  0x44


/*
 *  All of the IOCTL functions we support.
 */
#define CAN_IOCTL_READ_MCR                  _IOR(CAN_MAGIC_TYPE, 1, unsigned int)
#define CAN_IOCTL_READ_CTRL1                _IOR(CAN_MAGIC_TYPE, 2, unsigned int)
#define CAN_IOCTL_READ_TIMER                _IOR(CAN_MAGIC_TYPE, 3, unsigned int)
#define CAN_IOCTL_READ_ECR                  _IOR(CAN_MAGIC_TYPE, 4, unsigned int)
#define CAN_IOCTL_READ_ESR1                 _IOR(CAN_MAGIC_TYPE, 5, unsigned int)
#define CAN_IOCTL_READ_IMASK2               _IOR(CAN_MAGIC_TYPE, 6, unsigned int)
#define CAN_IOCTL_READ_IMASK1               _IOR(CAN_MAGIC_TYPE, 7, unsigned int)
#define CAN_IOCTL_READ_IFLAG2               _IOR(CAN_MAGIC_TYPE, 8, unsigned int)
#define CAN_IOCTL_READ_IFLAG1               _IOR(CAN_MAGIC_TYPE, 9, unsigned int)
#define CAN_IOCTL_READ_CTRL2                _IOR(CAN_MAGIC_TYPE, 10, unsigned int)
#define CAN_IOCTL_READ_ESR2                 _IOR(CAN_MAGIC_TYPE, 11, unsigned int)
#define CAN_IOCTL_READ_GFWR                 _IOR(CAN_MAGIC_TYPE, 12, unsigned int)

#define CAN_IOCTL_ENABLE_LOOPBACK           _IO(CAN_MAGIC_TYPE, 13)
#define CAN_IOCTL_DISABLE_LOOPBACK          _IO(CAN_MAGIC_TYPE, 14)
#define CAN_IOCTL_ENABLE_SELF_RECEPTION     _IO(CAN_MAGIC_TYPE, 15)
#define CAN_IOCTL_DISABLE_SELF_RECEPTION    _IO(CAN_MAGIC_TYPE, 16)
#define CAN_IOCTL_ENABLE_MESSAGE_ACCEPT     _IO(CAN_MAGIC_TYPE, 17)

/*
 *  TODO -  If we want an application to get these, it should be via an IOCTL interface.
 *          For now, the /proc API works.
 */
#if 0
#define CAN_IOCTL_GET_FILE_STATS            _IOR(CAN_MAGIC_TYPE, 18)
#define CAN_IOCTL_GET_DEVICE_STATS          _IOR(CAN_MAGIC_TYPE, 19)
#define CAN_IOCTL_RESET_DEVICE_STATS        _IO(CAN_MAGIC_TYPE, 20)
#endif


/*
 *  We only support standard and extended message types, 
 *  no Remote frames, etc.
 */
typedef enum CanbusMessageType_ {

    CmtUndefined,
    CmtStandard,
    CmtExtended

}CanbusMessageType;

/*
 *  This mirrors the elements in the PEAK 
 *  data structure, to help reuse.
 */

#pragma pack(1)

typedef struct CANBUS_MESSAGE_
{
    unsigned int  Id;           /*  11/29-bit message identifier */
    unsigned int  Type;         /*  Type of the message */
    unsigned int  DataLength;   /*  Data Length Code of the message (0..8) */
    unsigned char Data[8];      /*  Data of the message (DATA[0]..DATA[7]) */

} CANBUS_MESSAGE, *PCANBUS_MESSAGE;


/*
 *  MUST be same size as CANBUS_MESSAGE.
 */
typedef struct CANBUS_STATUS_CHANGE_
{
    unsigned int StatusChangeFlag;  /*  Set to CANBUS_STATUS_CHANGE_FLAG for now. */
    unsigned int Status1;           /*  Uses CanStatusChange1 bits */
    unsigned int Status2;           /*  Protocol stats - Driver should not use this. */
    unsigned int Status3;           /* */ 
    unsigned int Status4;           /* */ 

} CANBUS_STATUS_CHANGE, *PCANBUS_STATUS_CHANGE;

#pragma pack()


/*
 *  Use invalid bits to flag it's not a CANBUS_MESSAGE.  
 *  This can be expanded from one flag to 7 if need be.
 */
#define CANBUS_STATUS_CHANGE_FLAG 0xE0000000


enum  CanStatusChange1 {
    Csc1TxWarn      = 0x00000001,   /*  Transmit warning triggered, TX errors > 96 */
    Csc1RxWarn      = 0x00000002,   /*  Receive warning triggered, RX errors > 96 */
    Csc1BusOff      = 0x00000004,   /*  BUS OFF */
    Csc1Bit1Err     = 0x00000008,   /*  Bit 1 Error */
    Csc1Bit0Err     = 0x00000010,   /*  Bit 0 Error */
    Csc1AckErr      = 0x00000020,   /*  Transmit ACK Error */
    Csc1CrcErr      = 0x00000040,   /*  Receive CRC Error */
    Csc1FormErr     = 0x00000080,   /*  Format Error */
    Csc1StuffErr    = 0x00000100,   /*  Bit stuffing error */
};


/*
 *  Running device statistics for a flexcan device.
 */
struct can_device_stats_t {
    
    /*
     *  There is code that expects this to be first in the struct.
     */
    unsigned long long isr_count;

    unsigned int tx_warn_count;         /* Total number of these errors. */
    unsigned int rx_warn_count;         /* Total number of these errors. */
    unsigned int bus_off_count;         /* Total number of these errors. */
    unsigned int bit1_error_count;      /* Total number of these errors. */
    unsigned int bit0_error_count;      /* Total number of these errors. */
    unsigned int ack_error_count;       /* Total number of these errors. */
    unsigned int crc_error_count;       /* Total number of these errors. */
    unsigned int form_error_count;      /* Total number of these errors. */
    unsigned int bitstuff_error_count;  /* Total number of these errors. */

    struct timespec total_isr_time;     /* Total time spent in ISR, for user space averaging. */
    struct timespec cur_isr_time;       /* Time spent in very last ISR. */
    struct timespec max_isr_time;       /* High water mark of time spent all ISRs. */

    unsigned long long total_mb_used;   /* Total MB used ever, for user space averaging. */
    unsigned int cur_mb_used;           /* MBs used in very last ISR. */
    unsigned int max_mb_used;           /* High water mark of MBs used from all ISRs. */

    unsigned int cur_tx_queue_count;    /* Depth of our write (tx) queue "now" */
    unsigned int max_tx_queue_count;    /* High water mark of our write queue */

};


/*
 *  This is tracked "per filehandle".  To reset, just close the file.
 */
struct can_file_stats_t {
    
    unsigned long long read_count;                      /* Total number of reads ever on this file */
    unsigned long long write_count;                     /* Total number of writes ever on this file */
    unsigned long long write_transmits_queued;          /* Total number of writes queued for ISR Tx */
    unsigned long long write_transmits_directly_sent;   /* Total number of writes sent from process context */

    unsigned int cur_rx_queue_count;                    /* Depth of our read (rx) queue "now" */
    unsigned int max_rx_queue_count;                    /* High water mark of our read queue */

};


#ifdef __cplusplus
}
#endif

#endif 

