/****************************************************************************
 *  flexcan_hardware.c
 *
 *  This implements the i.MX6 flexcan hardware API used by this driver
 *  and is based on the needs of the driver as well as the Freescale i.MX6 
 *  data sheet.
 *
 ***************************************************************************/
#include "can_private.h"


/**
 *	We are going to accept everything, since this is effectively 
 *	the master controller.
 */
static inline void
configure_message_buffer_masks(struct canbus_device_t *dev)
{
	unsigned int reg;
    int i;

	for (i=0; i<FLEXCAN_NUM_MESSAGE_BUFFERS; i++){

		iowrite32(0, &dev->registers->RXIMR[i]);
	    reg = ioread32(&dev->registers->RXIMR[i]);

        printk(KERN_DEBUG "Writing RXIMR[%d] = 0x%08x\n", i, reg);
	}
}



static inline void
reset_flexcan_module(struct canbus_device_t *dev)
{
	unsigned int reg;

	reg = ioread32(&dev->registers->MCR);
	printk(KERN_DEBUG "reset_flexcan_module() MCR = 0x%08x\n", reg);

	reg |= MCR_SOFT_RST;
	iowrite32(reg, &dev->registers->MCR);

	do {

	    reg = ioread32(&dev->registers->MCR);
	    printk(KERN_DEBUG "reset_flexcan_module() MCR = 0x%08x\n", reg);

	}while (reg & MCR_SOFT_RST);
}



static inline void
enable_flexcan_module(struct canbus_device_t *dev)
{
	unsigned int reg;

	reg = ioread32(&dev->registers->MCR);
	printk(KERN_DEBUG "enable_flexcan_module() MCR = 0x%08x\n", reg);

	reg &= ~MCR_MDIS;
	iowrite32(reg, &dev->registers->MCR);
    udelay(10);

    reg = ioread32(&dev->registers->MCR);
	printk(KERN_DEBUG "enable_flexcan_module() MCR = 0x%08x\n", reg);
}



static inline void
disable_flexcan_module(struct canbus_device_t *dev)
{
	unsigned int reg;

	reg = ioread32(&dev->registers->MCR);
	printk(KERN_DEBUG "disable_flexcan_module() MCR = 0x%08x\n", reg);

	reg |= MCR_MDIS;
	iowrite32(reg, &dev->registers->MCR);
    udelay(10);

    reg = ioread32(&dev->registers->MCR);
	printk(KERN_DEBUG "disable_flexcan_module() MCR = 0x%08x\n", reg);
}

    
    
static inline void
enter_freeze_mode(struct canbus_device_t *dev)
{
	unsigned int reg;
    const unsigned int freeze_flags = MCR_FRZ | MCR_HALT | MCR_FRZ_ACK;

	reg = ioread32(&dev->registers->MCR);
	printk(KERN_DEBUG "enter_freeze_mode() MCR = 0x%08x\n", reg);

	reg |= (MCR_FRZ | MCR_HALT);
	iowrite32(reg, &dev->registers->MCR);

    do {

    	reg = ioread32(&dev->registers->MCR);
	    printk(KERN_DEBUG "enter_freeze_mode() MCR = 0x%08x\n", reg);

    }while ((reg & freeze_flags) != freeze_flags);
}



static inline void
exit_freeze_mode(struct canbus_device_t *dev)
{
	unsigned int reg;
    const unsigned int freeze_flags = MCR_FRZ | MCR_HALT | MCR_FRZ_ACK;

	reg = ioread32(&dev->registers->MCR);
	printk(KERN_DEBUG "exit_freeze_mode() MCR = 0x%08x\n", reg);

	reg &= ~(MCR_FRZ | MCR_HALT);
	iowrite32(reg, &dev->registers->MCR);

    do {

	    reg = ioread32(&dev->registers->MCR);
	    printk(KERN_DEBUG "exit_freeze_mode() MCR = 0x%08x\n", reg);

    }while (reg & freeze_flags);
}



/**
 *	This will probably be for MB[2] - MB[63].
 *	Follow the algorithm in IMX6DQRM.pdf - Section 26.6.4
*/
static void
hw_init_receive_message_buffer( struct canbus_device_t *dev,
							int message_buffer_index)
{
	MESSAGE_BUFFER	*mb;
	unsigned int code_and_status;
	unsigned int id;

	mb = &dev->registers->MB[message_buffer_index];
	
	code_and_status = ioread32(&mb->code_and_status);
    code_and_status = (code_and_status & ~MB_CODE_MASK) | MB_RX_CODE_INACTIVE;
	iowrite32(code_and_status, &mb->code_and_status);

    id = 0;
	iowrite32(id, &mb->id);

	code_and_status = ioread32(&mb->code_and_status);
    code_and_status = (code_and_status & ~MB_CODE_MASK) | MB_RX_CODE_EMPTY | MB_IDE;
	iowrite32(code_and_status, &mb->code_and_status);
}



/** 
 *  Set up our Transmit Message Buffer.
 */
static void
hw_init_transmit_message_buffer(struct canbus_device_t *dev)
{
	unsigned int code_and_status;

    code_and_status = ioread32(&dev->registers->MB[TX_MB].code_and_status);
    code_and_status = (code_and_status & ~MB_CODE_MASK) | MB_TX_CODE_INACTIVE;
	iowrite32(code_and_status, &dev->registers->MB[TX_MB].code_and_status);

	code_and_status = ioread32(&dev->registers->MB[TX_ERRATA_MB].code_and_status);
    code_and_status = (code_and_status & ~MB_CODE_MASK) | MB_TX_CODE_INACTIVE;
	iowrite32(code_and_status, &dev->registers->MB[TX_ERRATA_MB].code_and_status);
}



/**
 *	Set up the global registers, etc so we are ready to receive messages.
 */
void
hw_initialize_hardware(struct canbus_device_t *dev)
{
	unsigned int reg;
    int i;

    /*
     *  Make sure the Flexcan module is enabled.
     */
    enable_flexcan_module(dev);

	/*
     *  Reset the Flexcan module
     */
    reset_flexcan_module(dev);

    /*
     *  Enter Freeze Mode...
     */
    enter_freeze_mode(dev);

    /*
     *  Setup the Module Configuration register first...
     */
    reg = ioread32(&dev->registers->MCR);

	reg |= (    MCR_SUPV 
		        |	MCR_WRN_EN 
				//|	MCR_WAK_SRC 
				|	MCR_SRX_DIS
				|   MCR_IRMQ
				|	MCR_AEN     );

    /*
     *  Lets try using all the MB we have right now.
     *  This takes an index, not a count.
     */
    reg &= ~MCR_MAXMB_MASK;
	reg |= (MCR_MAXMB_MASK & (FLEXCAN_NUM_MESSAGE_BUFFERS - 1));

	iowrite32(reg, &dev->registers->MCR);

    /*
     *  No masking, we want everything on the bus...
     */
    configure_message_buffer_masks(dev);

    /*
     *  Set up CTRL2 
     */
    reg = CTRL2_MRP;
	iowrite32(reg, &dev->registers->CTRL2);

    /*
     *  Clear any superfluous interrupts
     */
    for (i = 0; i < FLEXCAN_NUM_MESSAGE_BUFFERS; i++){
        hw_clear_message_buffer_interrupt(dev, i);
    }

    /*
     *  Enable Interrupts on the MBs
     *  Since MB[0] is being reserved for an errata workaround, 
     *  don't bother to turn it on.
     */
    for (i = TX_MB; i < FLEXCAN_NUM_MESSAGE_BUFFERS; i++){
        hw_enable_message_buffer_interrupt(dev, i);
    }

    /*
     *  Calculate the bitrate
     */
    reg = can_update_bitrate(dev->clock_freq, Mbps_1);

    /*
     *  from the Flexcan example code...
     */
    reg |= CTRL1_SET_RJW(2);	

    /*
     *  Set up the rest of the register ...
     */
    reg |= (    CTRL1_BOFF_MSK 
				|	CTRL1_ERR_MSK 
				|	CTRL1_TWRN_MSK 
				|	CTRL1_RWRN_MSK 
				|	CTRL1_SMP 
				|	CTRL1_LBUF);

	iowrite32(reg, &dev->registers->CTRL1);

    /*
     *  Set up the receive message buffers.
     */
    for (i = FIRST_RX_MB; i<FLEXCAN_NUM_MESSAGE_BUFFERS; i++){
        hw_init_receive_message_buffer(dev, i);
    }

    /*
     *  Set up the transmit message buffer.
     */
    hw_init_transmit_message_buffer(dev);

    /*
     *  We are finally set-up, exit Freeze mode...
     */
    exit_freeze_mode(dev);
}



/**
 *  API to Enable Loopback mode on the chip.
 */
void
hw_enable_loopback_mode(struct canbus_device_t *dev)
{
    unsigned int reg;

    enter_freeze_mode(dev);

    reg = ioread32(&dev->registers->CTRL1);

    reg |= CTRL1_LPB;

	iowrite32(reg, &dev->registers->CTRL1);

	exit_freeze_mode(dev);
}



/**
 *  API to Disable Loopback mode on the chip.
 */
void
hw_disable_loopback_mode(struct canbus_device_t *dev)
{
    unsigned int reg;

    enter_freeze_mode(dev);

    reg = ioread32(&dev->registers->CTRL1);

    reg &= ~CTRL1_LPB;

	iowrite32(reg, &dev->registers->CTRL1);

	exit_freeze_mode(dev);
}



/**
 *  API to Enable Self reception on the chip.
 */
void
hw_enable_self_reception(struct canbus_device_t *dev)
{
    unsigned int reg;

    enter_freeze_mode(dev);

    reg = ioread32(&dev->registers->MCR);

    reg &= ~MCR_SRX_DIS;

	iowrite32(reg, &dev->registers->MCR);

	exit_freeze_mode(dev);
}



/**
 *  API to Disable self-reception on the chip.
 */
void
hw_disable_self_reception(struct canbus_device_t *dev)
{
    unsigned int reg;

    enter_freeze_mode(dev);

    reg = ioread32(&dev->registers->MCR);

    reg |= MCR_SRX_DIS;

	iowrite32(reg, &dev->registers->MCR);

	exit_freeze_mode(dev);
}



/**
 *  Enable IMASL for a Message Buffer.
 */
void
hw_enable_message_buffer_interrupt( struct canbus_device_t *dev,
                                int message_buffer_index)
{
	unsigned int reg;
	volatile unsigned int *imask;
	unsigned int ibit = 0x1;


	if (message_buffer_index > 31){
		imask = &dev->registers->IMASK2;
		ibit = ibit << (message_buffer_index - 32);
	}
	else{
		imask = &dev->registers->IMASK1;
		ibit = ibit << message_buffer_index;
	}

	reg = ioread32(imask);
	reg |= ibit;
	iowrite32(reg, imask);
}



/**
 *  Disable IMASK for a Message Buffer.
 */
void 
hw_disable_message_buffer_interrupt(    struct canbus_device_t *dev,
								    int message_buffer_index)
{
	unsigned int reg;
	volatile unsigned int *imask;
	unsigned int ibit = 0x1;


	if (message_buffer_index > 31){
		imask = &dev->registers->IMASK2;
		ibit = ibit << (message_buffer_index - 32);
	}
	else{
		imask = &dev->registers->IMASK1;
		ibit = ibit << message_buffer_index;
	}

	reg = ioread32(imask);
	reg &= ~ibit;
	iowrite32(reg, imask);
}



/**
 *  Clear the IFLAG for a specific Message Buffer.
 */
void
hw_clear_message_buffer_interrupt(  struct canbus_device_t *dev,
								int message_buffer_index)
{
	volatile unsigned int *iflag;
	unsigned int ibit = 0x1;


	if (message_buffer_index > 31){
		iflag = &dev->registers->IFLAG2;
		ibit = ibit << (message_buffer_index - 32);
	}
	else{
		iflag = &dev->registers->IFLAG1;
		ibit = ibit << message_buffer_index;
	}

    /*
     *  Write a 1 to clear it.  Writes of 0 have no effect.
     */
    iowrite32(ibit, iflag);
}



/**
 *  Check if a Message Buffer is interrupting.
 */
int 
hw_is_message_buffer_interrupting(  struct canbus_device_t *dev,
								int message_buffer_index)
{
	volatile unsigned int *iflag;
    unsigned int reg;
	unsigned int ibit = 0x1;


	if (message_buffer_index > 31){
		iflag = &dev->registers->IFLAG2;
		ibit = ibit << (message_buffer_index - 32);
	}
	else{
		iflag = &dev->registers->IFLAG1;
		ibit = ibit << message_buffer_index;
	}

    /*
     *  1 means it's interrupting, 0 means it's not.
     */
    reg = ioread32(iflag);
	if (reg & ibit){
		return 1;
	}
	else{
		return 0;
	}
}



/**
 *  Code for ISR, optimized way to get IFLAGs.
 */
void
get_iflags(  struct canbus_device_t *dev,
            unsigned int *iflag1, 
            unsigned int *iflag2
            )
{
    *iflag1 = ioread32(&dev->registers->IFLAG1);
    *iflag2 = ioread32(&dev->registers->IFLAG2);
}



/**
 *	Anticipated to be MB[1] only.
 *	MB[0] is reserved as an errata workaround.
 */
void
hw_transmit_message(	struct canbus_device_t *dev,
					CANBUS_MESSAGE *message)
{
	MESSAGE_BUFFER  *mb;
	unsigned int code_and_status;
	unsigned int is_interrupting;
	unsigned int data0_3 = 0;
	unsigned int data4_7 = 0;


	is_interrupting = hw_is_message_buffer_interrupting(dev, TX_MB);
	if (is_interrupting){
		hw_clear_message_buffer_interrupt(dev, TX_MB);
	}

	mb = &dev->registers->MB[TX_MB];
	
	do {
		code_and_status = ioread32(&mb->code_and_status);
	}while ((code_and_status & MB_TX_CODE_DATA) == MB_TX_CODE_DATA);

    /*
     *  Write Id
     */
    iowrite32(message->Id, &mb->id);

	/*
		DLC		Valid DATA BYTEs
		====================
		0		none
		1		DATA BYTE 0
		2		DATA BYTE 0-1
		3		DATA BYTE 0-2
		4		DATA BYTE 0-3
		5		DATA BYTE 0-4
		6		DATA BYTE 0-5
		7		DATA BYTE 0-6
		8		DATA BYTE 0-7
	*/

    /*
     *  Write data bytes - we want to fall through...
     */
    switch(message->DataLength){
		case 8:
			data4_7 |= (unsigned int)message->Data[7];
		case 7:
			data4_7 |= (unsigned int)message->Data[6] << 8;
		case 6:
			data4_7 |= (unsigned int)message->Data[5] << 16;
		case 5:
			data4_7 |= (unsigned int)message->Data[4] << 24;
			iowrite32(data4_7, &mb->Data4_7);
		case 4:
			data0_3 |= (unsigned int)message->Data[3];
		case 3:
			data0_3 |= (unsigned int)message->Data[2] << 8;
		case 2:
			data0_3 |= (unsigned int)message->Data[1] << 16;
		case 1:
			data0_3 |= (unsigned int)message->Data[0] << 24;
			iowrite32(data0_3, &mb->data_0_3);
			break;

		default:
			break;
	}

	if (message->Type == CmtExtended){
		code_and_status = MB_TX_CODE_DATA | MB_SRR | MB_IDE | SET_DLC(message->DataLength);
	}
	else{
		code_and_status = MB_TX_CODE_DATA | SET_DLC(message->DataLength);
	}

	/*
     *  Write DLC, Control and Code fields of C/S
     */
    iowrite32(code_and_status, &mb->code_and_status);

    /*
     *  Errata ERR005829 workaround
     */
    code_and_status = MB_TX_CODE_INACTIVE;
	iowrite32(code_and_status, &dev->registers->MB[TX_ERRATA_MB].code_and_status);
	iowrite32(code_and_status, &dev->registers->MB[TX_ERRATA_MB].code_and_status);
}



void
hw_abort_transmit(struct canbus_device_t *dev)
{
	MESSAGE_BUFFER *mb;
	unsigned int code_and_status;
	unsigned int is_interrupting;


	is_interrupting = hw_is_message_buffer_interrupting(dev, TX_MB);
	if (is_interrupting){
		hw_clear_message_buffer_interrupt(dev, TX_MB);
	}

	mb = &dev->registers->MB[TX_MB];

    /*  
     *  Write ABORT
     */
    code_and_status = MB_TX_CODE_ABORT;
    iowrite32(code_and_status, &mb->code_and_status);

    /*
     *  Wait for IFLAG indicating that the frame was either 
     *  transmitted or aborted.
     */
    do {
	    is_interrupting = hw_is_message_buffer_interrupting(dev, TX_MB);
	}while (!is_interrupting);

    /*
     *  Check for transmit or abort. 
     */
    code_and_status = ioread32(&mb->code_and_status);

    /*
     *  Clear the IFLAG so the TX MB can be reconfigured.
     */
    hw_clear_message_buffer_interrupt(dev, TX_MB);
}



/**
 *	Pull a message out of the MB.
 *	Follow the algorithm in IMX6DQRM.pdf - Section 26.6.4
 *  This clears the IFLAG bit.
 */
unsigned int 
hw_receive_message(	struct canbus_device_t *dev,
					CANBUS_MESSAGE	*message,
					int message_buffer_index)	
{
	MESSAGE_BUFFER  *mb;
	unsigned int code_and_status;
	unsigned int data0_3;
	unsigned int data4_7;
	unsigned int timer;


	mb = &dev->registers->MB[message_buffer_index];
	
	/*
     *  First, get the data out as fast as possible, so we can re-enable 
     *  the ISR.
     */
    do {
		code_and_status = ioread32(&mb->code_and_status);
	}while (code_and_status & MB_RX_CODE_BUSY);

	message->Id = ioread32(&mb->id);

	data0_3 = ioread32(&mb->data_0_3);
	data4_7 = ioread32(&mb->Data4_7);

    /*
     *  Re-enable the MB Interrupt 
     */
    hw_clear_message_buffer_interrupt(dev, message_buffer_index);

    /*  
     *  This unlocks the Mailbox.  And we are done touching HW.
     */
    timer = ioread32(&dev->registers->TIMER);

    /*  
     *  Process the registers into the format the app is using.
     */
    message->DataLength = GET_DLC(code_and_status);

    /*
     *  Read data bytes - we want to fall through...
     */
 	switch(message->DataLength){
		case 8:
			message->Data[7] = (unsigned char)(data4_7 & 0xFF);
		case 7:
			message->Data[6] = (unsigned char)((data4_7 & 0xFF00) >> 8);
		case 6:
			message->Data[5] = (unsigned char)((data4_7 & 0xFF0000) >> 16);
		case 5:
			message->Data[4] = (unsigned char)((data4_7 & 0xFF000000) >> 24);
		case 4:
			message->Data[3] = (unsigned char)(data0_3 & 0xFF);
		case 3:
			message->Data[2] = (unsigned char)((data0_3 & 0xFF00) >> 8);
		case 2:
			message->Data[1] = (unsigned char)((data0_3 & 0xFF0000) >> 16);
		case 1:
			message->Data[0] = (unsigned char)((data0_3 & 0xFF000000) >> 24);
			break;
		default:
			break;
	}

	if (code_and_status & MB_IDE){
		message->Type = CmtExtended;
	}
	else{
		message->Type = CmtStandard;
	}

    /*
     *  Return the message timestamp so we can sort them.
     */
    return (MB_TIMESTAMP_MASK & code_and_status);
}





