/****************************************************************************
 *  can_write.c
 *
 *  The write() API is how we transmit a CANbus message.  Everything between
 *  this driver and user space is done in terms of the CANBUS_MESSAGE data
 *  structure.
 *
 *  Future optimizations could include allowing writing multiples of a 
 *  message.
 ***************************************************************************/
#include "can_private.h"


ssize_t can_write ( struct file *filp, const char __user *buf, 
                    size_t count, loff_t *f_pos)
{
    struct kcanbus_message *message;
    struct canbus_file_t *file;
    struct canbus_device_t *dev;
    unsigned int real_data_size;
    int message_sent;
    unsigned long flags;

    /*
     *  Recover our per file and per device data structures.
     *  Sanity check everything.
     */
    file = filp->private_data;
    if (file->signature != CANBUS_FILE_SIGNATURE){
        printk(KERN_ERR "Failed signature check! %s %d\n", __FILE__, __LINE__);
        return -EBADFD;
    }

    dev = file->dev;
    if (dev->signature != CANBUS_DEVICE_SIGNATURE){
        printk(KERN_ERR "Failed signature check! %s %d\n", __FILE__, __LINE__);
        return -EBADFD;
    }

    file->stats.write_count++;

    if ( (count > sizeof(CANBUS_MESSAGE)) || 
            (count < (sizeof(CANBUS_MESSAGE) - 8))) {
        return -EPROTO;
    }

    message = alloc_kcanbus_message();
    if (!message){
        return -ENOMEM;
    }

    memset(message, 0, sizeof(struct kcanbus_message));
    INIT_LIST_HEAD(&message->entry);
    message->signature = KCANBUS_SIGNATURE;

    if (copy_from_user(&message->user_message, buf, count)){
        printk(KERN_ERR "Bad user write buffer!\n");
        free_kcanbus_message(message);
        return -EFAULT;
    }

    if ((message->user_message.Type != CmtStandard) &&
        (message->user_message.Type != CmtExtended)){
        printk(KERN_ERR "Bad message type!\n");
        free_kcanbus_message(message);
        return -EPROTO;
    }

    if (message->user_message.Id & 0xE0000000){
        printk(KERN_ERR "Invalid message id!\n");
        free_kcanbus_message(message);
        return -EPROTO;
    }

    real_data_size = sizeof(CANBUS_MESSAGE) - 8 + message->user_message.DataLength;
    if (real_data_size > count){
        printk(KERN_ERR "Data length mismatch!\n");
        free_kcanbus_message(message);
        return -EPROTO;
    }

    if(message->user_message.DataLength > 8){
        printk(KERN_ERR "Invalid Data length!\n");
        free_kcanbus_message(message);
        return -EPROTO;
    }

    /*
     *  LOCK --------------------------------------------------------
     */
    spin_lock_irqsave(&dev->register_lock, flags);

    /*
     *  If the HW is busy, add this to the tx queue.  Otherwise 
     *  send it out directly from here.
     */
    if (dev->transmit_in_progress){

        list_add_tail(&message->entry, &dev->transmit_queue);

        file->stats.write_transmits_queued++;
        message_sent = 0;

        dev->stats.cur_tx_queue_count++;
        if (dev->stats.cur_tx_queue_count > dev->stats.max_tx_queue_count){
            dev->stats.max_tx_queue_count = dev->stats.cur_tx_queue_count;
        }
    }
    else{
        file->stats.write_transmits_directly_sent++;
        message_sent = 1;

        dev->transmit_in_progress = 1;

        hw_transmit_message(dev, &message->user_message);
        hw_enable_message_buffer_interrupt(dev, TX_MB);
    }

    /*
     *  UNLOCK --------------------------------------------------------
     */
    spin_unlock_irqrestore(&dev->register_lock, flags);

    /*
     *  If we sent it right from here, the ISR can't free it
     *  because it never gets dequeued from the isr.  We need to 
     *  free it here.
     */
    if(message_sent){
        free_kcanbus_message(message);
    }

    return count;
}


