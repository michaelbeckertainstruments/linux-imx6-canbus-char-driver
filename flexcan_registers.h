/****************************************************************************
 *  flexcan_registers.h
 *
 *  This is the hardware implementation (API) of the i.MX6 Flexcan module, 
 *  written using the data sheet as reference.
 *
 ***************************************************************************/
#ifndef FLEXCAN_REGISTERS_H__
#define FLEXCAN_REGISTERS_H__

#define FLEXCAN1_PHYSICAL_ADDRESS   0x02090000UL
#define FLEXCAN2_PHYSICAL_ADDRESS   0x02094000UL
#define FLEXCAN_NUM_MESSAGE_BUFFERS 64
#define TX_ERRATA_MB                0
#define TX_MB                       1
#define FIRST_RX_MB                 2



/****************************************************************************
 *
 *  Message Buffer Structure
 *
 ***************************************************************************/

/*  
 *  Message Buffer Address: Base + 0x0080-0x047C
 */
typedef struct MESSAGE_BUFFER_ {
    
    volatile unsigned int code_and_status;
    volatile unsigned int id;
    volatile unsigned int data_0_3;
    volatile unsigned int Data4_7;

}MESSAGE_BUFFER, *PMESSAGE_BUFFER;


#define MB_CS_RSVD31_28         0xF0000000
#define MB_CODE_MASK            0x0F000000
#define MB_CS_RSVD23            0x00800000
#define MB_SRR                  0x00400000
#define MB_IDE                  0x00200000
#define MB_RTR                  0x00100000
#define MB_DLC_MASK             0x000F0000
#define MB_TIMESTAMP_MASK       0x0000FFFF
#define SET_DLC(length_)        ((length_ << 16) & MB_DLC_MASK)
#define GET_DLC(c_s_reg_)       ((c_s_reg_ & MB_DLC_MASK) >> 16)


#define MB_PRIO_MASK            0xE0000000
#define MB_ID_STANDARD_MASK     0x1FFC0000
#define MB_ID_EXTENDED_MASK     0x0003FFFF

#define MB_DATA0_MASK           0xFF000000
#define MB_DATA1_MASK           0x00FF0000
#define MB_DATA2_MASK           0x0000FF00
#define MB_DATA3_MASK           0x000000FF

#define MB_DATA4_MASK           0xFF000000
#define MB_DATA5_MASK           0x00FF0000
#define MB_DATA6_MASK           0x0000FF00
#define MB_DATA7_MASK           0x000000FF


#define MB_CODE_SHIFT           24


#define MB_RX_CODE_INACTIVE     (0x0 << MB_CODE_SHIFT)
#define MB_RX_CODE_EMPTY        (0x4 << MB_CODE_SHIFT)
#define MB_RX_CODE_FULL         (0x2 << MB_CODE_SHIFT)
#define MB_RX_CODE_OVERRUN      (0x6 << MB_CODE_SHIFT)
#define MB_RX_CODE_RANSWER      (0xA << MB_CODE_SHIFT)
#define MB_RX_CODE_BUSY         (0x1 << MB_CODE_SHIFT)

#define MB_TX_CODE_INACTIVE     (0x8 << MB_CODE_SHIFT)
#define MB_TX_CODE_ABORT        (0x9 << MB_CODE_SHIFT)
#define MB_TX_CODE_DATA         (0xC << MB_CODE_SHIFT)
#define MB_TX_CODE_REMOTE       (0xC << MB_CODE_SHIFT)
#define MB_TX_CODE_TANSWER      (0xE << MB_CODE_SHIFT)


/*
(In hex)
Start    Length     Name    
-----------------------------------------------
 4C          4      RXFIR
 50         30      Rsvd3[12]
 80        400      MB[FLEXCAN_NUM_MESSAGE_BUFFERS]
480        400      Rsvd4[256]
880        100      RXIMR0[64]
980         60      Rsvd5[24]
9E0          4      GFWR
*/

#pragma pack(1)

struct FLEXCAN_HW_REGISTERS {
    
    volatile unsigned int MCR;          /* Module Configuration Register */
    volatile unsigned int CTRL1;        /* Control 1 Register */
    volatile unsigned int TIMER;        /* Free Running Timer Register */
    volatile unsigned int Rsvd1;        /* - Gap in registers -  */
    volatile unsigned int RXMGMASK;     /* Rx Mailboxes Global Mask Register */
    volatile unsigned int RX14MASK;     /* Rx Buffer 14 Mask Register */
    volatile unsigned int RX15MASK;     /* Rx Buffer 15 Mask Register */
    volatile unsigned int ECR;          /* Error Counter Register */
    volatile unsigned int ESR1;         /* Error and Status 1 Register */
    volatile unsigned int IMASK2;       /* Interrupt Masks 2 Register */
    volatile unsigned int IMASK1;       /* Interrupt Masks 1 Register */
    volatile unsigned int IFLAG2;       /* Interrupt Flags 2 Register */
    volatile unsigned int IFLAG1;       /* Interrupt Flags 1 Register */
    volatile unsigned int CTRL2;        /* Control 2 Register */
    volatile unsigned int ESR2;         /* Error and Status 2 Register */
    volatile unsigned int Rsvd2[2];     /* - Gap in registers -  */
    volatile unsigned int CRCR;         /* CRC Register */
    volatile unsigned int RXFGMASK;     /* Rx FIFO Global Mask Register */
    volatile unsigned int RXFIR;        /* Rx FIFO Information Register */
    volatile unsigned int Rsvd3[12];    /* - Gap in registers -  */
    MESSAGE_BUFFER    MB[FLEXCAN_NUM_MESSAGE_BUFFERS];    /* The actual Mailboxes */
    volatile unsigned int Rsvd4[256];   /* - Gap in registers -  */
    volatile unsigned int RXIMR[64];    /* Rx Individual Mask Registers */
    volatile unsigned int Rsvd5[24];    /* - Gap in registers -  */
    volatile unsigned int GFWR;         /* Glitch Filter Width Registers */

};

#pragma pack()



/****************************************************************************
 *
 *  Module Configuration Register (MCR)
 *
 ***************************************************************************/
#define MCR_MDIS        0x80000000    
#define MCR_FRZ         0x40000000
#define MCR_RFEN        0x20000000
#define MCR_HALT        0x10000000
#define MCR_NOT_RDY     0x08000000
#define MCR_WAK_MSK     0x04000000
#define MCR_SOFT_RST    0x02000000
#define MCR_FRZ_ACK     0x01000000
#define MCR_SUPV        0x00800000
#define MCR_SLF_WAK     0x00400000
#define MCR_WRN_EN      0x00200000
#define MCR_LPM_ACK     0x00100000
#define MCR_WAK_SRC     0x00080000
#define MCR_RSVD18      0x00040000
#define MCR_SRX_DIS     0x00020000
#define MCR_IRMQ        0x00010000
#define MCR_RSVD15      0x00008000
#define MCR_RSVD14      0x00004000
#define MCR_LPRIO_EN    0x00002000
#define MCR_AEN         0x00001000
#define MCR_RSVD11      0x00000800
#define MCR_RSVD10      0x00000400
#define MCR_IDAM_MASK   0x00000300
#define MCR_RSVD7       0x00000080
#define MCR_MAXMB_MASK  0x0000007F


/****************************************************************************
 *
 *  Control 1 Register (CTRL1)
 *
 ***************************************************************************/
#define CTRL1_PRESDIV_MASK      0xFF000000
#define CTRL1_RJW_MASK          0x00C00000
#define CTRL1_SET_RJW(rjw_)     ((rjw_ << 22) & CTRL1_RJW_MASK)
#define CTRL1_PSEG1_MASK        0x00380000
#define CTRL1_PSEG2_MASK        0x00070000
#define CTRL1_BOFF_MSK          0x00008000
#define CTRL1_ERR_MSK           0x00004000
#define CTRL1_RSVD13            0x00002000
#define CTRL1_LPB               0x00001000
#define CTRL1_TWRN_MSK          0x00000800
#define CTRL1_RWRN_MSK          0x00000400
#define CTRL1_RSVD9             0x00000200
#define CTRL1_RSVD8             0x00000100
#define CTRL1_SMP               0x00000080
#define CTRL1_BOFF_REC          0x00000040
#define CTRL1_TSYN              0x00000020
#define CTRL1_LBUF              0x00000010
#define CTRL1_LOM               0x00000008
#define CTRL1_PROP_SEG_MASK     0x00000007


/****************************************************************************
 *
 *  Free Running Timer Register (TIMER)
 *
 ***************************************************************************/
#define TIMER_RSVD    0xFFFF0000
#define TIMER_MASK    0x0000FFFF


/****************************************************************************
 *
 *  Rx Mailboxes Global Mask Register (RXMGMASK)
 *
 ***************************************************************************/
/*    LEGACY SUPPORT  */


/****************************************************************************
 *
 *  Rx Buffer 14 Mask Register (RX14MASK)
 *
 ***************************************************************************/
/*    LEGACY SUPPORT  */


/****************************************************************************
 *
 *  Rx Buffer 15 Mask Register (RX15MASK)
 *
 ***************************************************************************/
/*    LEGACY SUPPORT  */


/****************************************************************************
 *
 *  Error Counter Register (ECR)
 *
 ***************************************************************************/
#define ECR_RSVD31_16               0xFFFF0000
#define ECR_RX_ERR_COUNTER_MASK     0x0000FF00
#define ECR_TX_ERR_COUNTER_MASK     0x000000FF


/****************************************************************************
 *
 *  Error and Status 1 Register (ESR1)
 *
 ***************************************************************************/
#define ESR1_RSVD31_19          0xFFF80000
#define ESR1_SYNCH              0x00040000
#define ESR1_TWRN_INT           0x00020000
#define ESR1_RWRN_INT           0x00010000
#define ESR1_BIT1_ERR           0x00008000
#define ESR1_BIT0_ERR           0x00004000
#define ESR1_ACK_ERR            0x00002000
#define ESR1_CRC_ERR            0x00001000
#define ESR1_FRM_ERR            0x00000800
#define ESR1_STF_ERR            0x00000400
#define ESR1_TX_WRN             0x00000200
#define ESR1_RX_WRN             0x00000100
#define ESR1_IDLE               0x00000080
#define ESR1_TX                 0x00000040
#define ESR1_FLT_CONF_MASK      0x00000030

#define ESR1_FLT_CONF_ERROR_ACTIVE      0x00000000  /* b 00 */
#define ESR1_FLT_CONF_ERROR_PASSIVE     0x00000010  /* b 01 */
#define ESR1_FLT_CONF_ERROR_BUS_OFF     0x00000020  /* b 1x */

#define ESR1_RX                 0x00000008
#define ESR1_BOFF_INT           0x00000004
#define ESR1_ERR_INT            0x00000002
#define ESR1_WAK_INT            0x00000001


/****************************************************************************
 *
 *  Control 2 Register (CTRL2)
 *
 ***************************************************************************/
#define CTRL2_RSVD_31_MUST_BE_ZERO  0x80000000
#define CTRL2_RSVD_30_29            0x60000000
#define CTRL2_WRMFRZ                0x10000000
#define CTRL2_RFFN_MASK             0x0F000000
#define CTRL2_TASD_MASK             0x00F80000
#define CTRL2_MRP                   0x00040000
#define CTRL2_RRS                   0x00020000
#define CTRL2_EACEN                 0x00010000
#define CTRL2_RSVD15_0              0x0000FFFF


/****************************************************************************
 *
 *  Error and Status 2 Register (ESR2)
 *
 ***************************************************************************/
#define ESR2_RSVD31_23      0xFF800000
#define ESR2_LPTM_MASK      0x007F0000
#define ESR2_RSVD15         0x00008000
#define ESR2_VPS            0x00004000
#define ESR2_IMB            0x00002000
#define ESR2_RSVD12_0       0x00001FFF


/****************************************************************************
 *
 *  CRC Register (CRCR)
 *
 ***************************************************************************/
#define CRCR_RSVD31_23      0xFF800000
#define CRCR_MBCRC_MASK     0x007F0000
#define CRCR_RSVD15         0x00008000
#define CRCR_TXCRC          0x00007FFF


#endif 

