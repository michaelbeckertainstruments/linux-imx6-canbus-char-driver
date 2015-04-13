/****************************************************************************
 *  isr.c
 *
 *  This is the top half ISR for CANbus for the i.MX6.
 *
 ***************************************************************************/
#include "can_private.h"


/**
 *  Keep the larger data struct off the stack, we have 1 - 2 page limit.
 */
static CANBUS_MESSAGE message_buffers[FLEXCAN_NUM_MESSAGE_BUFFERS - FIRST_RX_MB];
static unsigned int message_timestamps[FLEXCAN_NUM_MESSAGE_BUFFERS - FIRST_RX_MB];
static CANBUS_MESSAGE *msg_ptrs[FLEXCAN_NUM_MESSAGE_BUFFERS - FIRST_RX_MB];


/**
 *  This is designed to be run as a threaded ISR.
 *  UPDATE - converted to a real top half isr.
 */
irqreturn_t can_irq_fn(int irq, void *dev_id)
{
    unsigned long flags;
    struct canbus_device_t *dev = (struct canbus_device_t *)dev_id;
    struct list_head *element;
    struct canbus_file_t *file;
    struct kcanbus_message *message;
    CANBUS_STATUS_CHANGE status_change;
    unsigned int reg;
    unsigned int now;
    unsigned int iflag1, iflag2;
    unsigned int count;
    unsigned int i;
    unsigned int iBit;
    struct timespec tv_start;
    struct timespec tv_end;
    int errors_found = 0;
    int err_bit_found;

    getnstimeofday(&tv_start);

    if (dev->signature != CANBUS_DEVICE_SIGNATURE){
        printk(KERN_ERR "Device Failed signature check! %s %d\n", __FILE__, __LINE__);
        return IRQ_HANDLED;
    }

    /*
     *  LOCK --------------------------------------------------------
     */
    spin_lock_irqsave(&dev->register_lock, flags);

    dev->stats.isr_count++;

    /*
     *  Always prep this in case we need it.
     */
    memset(&status_change, 0, sizeof(CANBUS_STATUS_CHANGE));
    status_change.StatusChangeFlag = CANBUS_STATUS_CHANGE_FLAG;

    /*
     *  Check for errors.  Reading bits 15-10 clears them.  
     *  Bits 9-3 are status.
     */
    reg = ioread32(&dev->registers->ESR1);

    /*
     *  Transmit warning interrupt triggered.
     */
    if (reg & ESR1_TWRN_INT){
        errors_found = 1;
        dev->stats.tx_warn_count++;
        status_change.Status1 |= Csc1TxWarn; 
        iowrite32(ESR1_TWRN_INT, &dev->registers->ESR1);

        if (!dev->prior_errors_found)
            printk(KERN_ERR "ESR1_TWRN_INT\n");
    }

    /*
     *  Receive warning interrupt triggered.
     */
    if (reg & ESR1_RWRN_INT){
        errors_found = 1;
        dev->stats.rx_warn_count++;
        status_change.Status1 |= Csc1RxWarn; 
        iowrite32(ESR1_RWRN_INT, &dev->registers->ESR1);

        if (!dev->prior_errors_found)
            printk(KERN_ERR "ESR1_RWRN_INT\n");
    }

    /*
     *  BUS OFF interrupt triggered.
     */
    if (reg & ESR1_BOFF_INT){
        errors_found = 1;
        dev->stats.bus_off_count++;
        status_change.Status1 |= Csc1BusOff; 
        iowrite32(ESR1_BOFF_INT, &dev->registers->ESR1);

        if (!dev->prior_errors_found)
            printk(KERN_ERR "ESR1_BOFF_INT\n");
    }

    /*
     *  Error interrupt triggered.  Now we check which error...
     */
    if (reg & ESR1_ERR_INT){

        errors_found = 1;
        err_bit_found = 0;

        iowrite32(ESR1_ERR_INT, &dev->registers->ESR1);

        /*
         *  HW error statuses that get cleared when they were read.
         */
        if (reg & ESR1_BIT1_ERR){
            err_bit_found = 1;
            dev->stats.bit1_error_count++;
            status_change.Status1 |= Csc1Bit1Err;

            if (!dev->prior_errors_found)
                printk(KERN_ERR "ESR1_BIT1_ERR\n");
        }

        if (reg & ESR1_BIT0_ERR){
            err_bit_found = 1;
            dev->stats.bit0_error_count++;
            status_change.Status1 |= Csc1Bit0Err;

            if (!dev->prior_errors_found)
                printk(KERN_ERR "ESR1_BIT0_ERR\n");
        }

        /*
         *  We get this when we are disconnected from the bus. 
         *  We must handle this intelligently.
         */
        if (reg & ESR1_ACK_ERR){

            err_bit_found = 1;
            dev->stats.ack_error_count++;
 
            status_change.Status1 |= Csc1AckErr; 

            if (!dev->prior_errors_found)
                printk(KERN_NOTICE "ESR1_ACK_ERR\n");

            /*
             *  Abort the HW transmission, because we have to.
             */
            hw_abort_transmit(dev);

            while ( !list_empty(&dev->transmit_queue) ){

                element = dev->transmit_queue.next;
                list_del(element);
                message = list_entry(element, struct kcanbus_message, entry);
                
                dev->stats.cur_tx_queue_count--;
 
                if (message->signature != KCANBUS_SIGNATURE){
                    printk(KERN_ERR "Message Signature check Failed! %s %d\n", __FILE__, __LINE__);
                    goto EXIT;
                }

                free_kcanbus_message(message);
            }

            /*
             *  And we are effectively not transmitting, so clean up to 
             *  try again some time in the future.
             */
            dev->transmit_in_progress = 0;
            hw_disable_message_buffer_interrupt(dev, TX_MB);
        }

        if (reg & ESR1_CRC_ERR){
            err_bit_found = 1;
            dev->stats.crc_error_count++;
            status_change.Status1 |= Csc1CrcErr;

            if (!dev->prior_errors_found)
                printk(KERN_ERR "ESR1_CRC_ERR\n");
        }

        if (reg & ESR1_FRM_ERR){
            err_bit_found = 1;
            dev->stats.form_error_count++;
            status_change.Status1 |= Csc1FormErr;

            if (!dev->prior_errors_found)
                printk(KERN_ERR "ESR1_FRM_ERR\n");
        }

        if (reg & ESR1_STF_ERR){
            err_bit_found = 1;
            dev->stats.bitstuff_error_count++;
            status_change.Status1 |= Csc1StuffErr;

            if (!dev->prior_errors_found)
                printk(KERN_ERR "ESR1_STF_ERR\n");
        }

        if (!err_bit_found && !dev->prior_errors_found){
            printk(KERN_ERR "Received ESR1_ERR_INT without finding an error bit!\n");
        }
    }

    /*
     *  Save if there were any errors this time.
     */
    dev->prior_errors_found = errors_found;

#if 0
    /*
     *  We ignore wake interrupts 
     */
    if (reg & ESR1_WAK_INT){
        iowrite32(ESR1_WAK_INT, &dev->registers->ESR1);
    }
#endif

    /*
     *  If there are HW errors or state changes, let someone know.
     *  We are piggybacking on the Read path.
     */
    if (status_change.Status1 != 0){

        list_for_each(element, &dev->reader_list){

            file = list_entry(element, struct canbus_file_t, reader_list_entry);
            
            if (file->signature != CANBUS_FILE_SIGNATURE){
                printk(KERN_ERR "File Signature check Failed! %s %d\n", __FILE__, __LINE__);
                goto EXIT;
            }

            if (!file->accept_messages){
                continue;
            }
            
            message = alloc_kcanbus_message();
            if (!message){
                goto EXIT;
            }
            
            memcpy(&message->user_message, &status_change, sizeof(status_change));
            INIT_LIST_HEAD(&message->entry);
            message->signature = KCANBUS_SIGNATURE;

            list_add_tail(&message->entry, &file->receive_queue);
 
            file->stats.cur_rx_queue_count++;
            if (file->stats.cur_rx_queue_count > file->stats.max_rx_queue_count){
                file->stats.max_rx_queue_count = file->stats.cur_rx_queue_count;
            }

            wake_up_interruptible(&file->receive_wq);
        }
    }

#if 0
/*
 *  Not currently used ...
 */    
    /*
     *  Current HW state...
     */
    if (reg & ESR1_TX_WRN){
        /*printk(KERN_ERR "ESR1_TX_WRN\n");*/
    }
    if (reg & ESR1_RX_WRN){
        /*printk(KERN_ERR "ESR1_RX_WRN\n");*/
    }
    if (reg & ESR1_IDLE){
        /*printk(KERN_ERR "ESR1_IDLE\n");*/
    }
    if ((reg & ESR1_FLT_CONF_MASK) == ESR1_FLT_CONF_ERROR_ACTIVE){
        /*printk(KERN_ERR "Error Active\n");*/
    }
    else if ((reg & ESR1_FLT_CONF_MASK) == ESR1_FLT_CONF_ERROR_PASSIVE){
        /*printk(KERN_ERR "Error Passive\n");*/
    }
    else /* BUS OFF */ {
        /*printk(KERN_ERR "Error BUS OFF\n");*/
    }
#endif

    /*
     *  Optimized HW read algorithm to get the data out asap!
     */
    get_iflags(dev, &iflag1, &iflag2);

    /*
     *  Need a time "after" all of the messages that we caught in 
     *  the Iflags.
     */
    now = ioread32(&dev->registers->TIMER);

    count = 0;
    iBit = 0x1 << FIRST_RX_MB;

    for (i = FIRST_RX_MB; i<32; i++){

        if (iBit & iflag1){

            message_timestamps[count] = hw_receive_message(dev, &message_buffers[count], i);
            msg_ptrs[count] = &message_buffers[count];

            /*
             *  Fix up the timestamps, if the message occurred "before" now,
             *  add a higher order bit because we wrapped.  
             *  What we really want to do is subtract from 
             *  messages > Now, but we are using 16 bit unsigned int math, so
             *  adding to the ones < Now effectively does the same thing.
             */
            if (message_timestamps[count] < now){
                message_timestamps[count] += 0x10000;
            }

            count++;
        }

        iBit <<= 1;
    }

    iBit = 0x1;

    for (; i<64; i++){

        if (iBit & iflag2){

            message_timestamps[count] = hw_receive_message(dev, &message_buffers[count], i);
            msg_ptrs[count] = &message_buffers[count];

            /*
             *  Fix up the timestamps, if the message occurred "before" now,
             *  add a higher order bit because we wrapped.  
             *  What we really want to do is subtract from 
             *  messages > Now, but we are using 16 bit unsigned int math, so
             *  adding to the ones < Now effectively does the same thing.
             */
            if (message_timestamps[count] < now){
                message_timestamps[count] += 0x10000;
            }

            count++;
        }

        iBit <<= 1;
    }

    /*
     *  Sort this using an Insertion sort for now.
     *  We sort the Timestamps because they are what's sortable,
     *  and we carry the MsgPtrs along for the ride, because
     *  these are what we "really" need sorted.
     *  http://en.wikipedia.org/wiki/Insertion_sort
     */
    for (i = 1; i<count; i++){

        unsigned int x = message_timestamps[i];
        CANBUS_MESSAGE *y = msg_ptrs[i];
        unsigned int j = i;

        while ((j > 0) && (message_timestamps[j-1] > x)){

            message_timestamps[j] = message_timestamps[j - 1];
            msg_ptrs[j] = msg_ptrs[j - 1];
            j--;
        }

        message_timestamps[j] = x;
        msg_ptrs[j] = y;
    }

    dev->stats.total_mb_used += count;
    dev->stats.cur_mb_used = count;
    if (count > dev->stats.max_mb_used){
        dev->stats.max_mb_used = count;
    }


    /*
     *  Now send it on it's way...
     */
    for (i = 0; i<count; i++){

        /*
         *  How big is the real message that we received?
         *  TODO - do we need this?  Or is it ok we just copy the whole
         *  message size out?
         */
        /* real_data_size = sizeof(CANBUS_MESSAGE) - 8 + msg_ptrs[i]->DataLength; */

        list_for_each(element, &dev->reader_list){

            file = list_entry(element, struct canbus_file_t, reader_list_entry);
            
            if (file->signature != CANBUS_FILE_SIGNATURE){
                printk(KERN_ERR "File Signature check Failed! %s %d\n", __FILE__, __LINE__);
                goto EXIT;
            }

            if (!file->accept_messages){
                continue;
            }
            
            message = alloc_kcanbus_message();
            if (!message){
                goto EXIT;
            }
            
            memcpy(&message->user_message, msg_ptrs[i], sizeof(CANBUS_MESSAGE));
            INIT_LIST_HEAD(&message->entry);
            message->signature = KCANBUS_SIGNATURE;
    
            list_add_tail(&message->entry, &file->receive_queue);

            file->stats.cur_rx_queue_count++;
            if (file->stats.cur_rx_queue_count > file->stats.max_rx_queue_count){
                file->stats.max_rx_queue_count = file->stats.cur_rx_queue_count;
            }

            wake_up_interruptible(&file->receive_wq);
        }
    }

    /*
     *  Check for transmit...
     */
    if (hw_is_message_buffer_interrupting(dev, TX_MB)){
            
        hw_clear_message_buffer_interrupt(dev, TX_MB);

        if (list_empty(&dev->transmit_queue)){

            dev->transmit_in_progress = 0;
            hw_disable_message_buffer_interrupt(dev, TX_MB);
        }
        else{

            element = dev->transmit_queue.next;
            list_del(element);
            message = list_entry(element, struct kcanbus_message, entry);

            dev->stats.cur_tx_queue_count--;
        
            if (message->signature != KCANBUS_SIGNATURE){
                printk(KERN_ERR "Message Signature check Failed! %s %d\n", __FILE__, __LINE__);
                goto EXIT;
            }
        
            hw_transmit_message(dev, &message->user_message);
            
            free_kcanbus_message(message);
        }
    }

EXIT:

    getnstimeofday(&tv_end);

    dev->stats.cur_isr_time = timespec_sub(tv_end, tv_start);
    dev->stats.total_isr_time = timespec_add( dev->stats.cur_isr_time, 
                                            dev->stats.total_isr_time);

    if (dev->stats.cur_isr_time.tv_sec > dev->stats.max_isr_time.tv_sec){
        dev->stats.max_isr_time = dev->stats.cur_isr_time;
    }
    else if ((dev->stats.cur_isr_time.tv_sec == dev->stats.max_isr_time.tv_sec) &&
        (dev->stats.cur_isr_time.tv_nsec > dev->stats.max_isr_time.tv_nsec)){
        dev->stats.max_isr_time = dev->stats.cur_isr_time;
    }

    /*
     *  UNLOCK --------------------------------------------------------
     */
    spin_unlock_irqrestore(&dev->register_lock, flags);

    return IRQ_HANDLED;
}



