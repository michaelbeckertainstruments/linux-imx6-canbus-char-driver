/****************************************************************************
 *  can_read.c
 *
 *  The read() API is how we receive a CANbus message.  Everything between 
 *  this driver and user space is done in terms of the CANBUS_MESSAGE data
 *  structure.
 *
 *  Future optimizations could include allowing reading multiples of a 
 *  message.
 *
 ***************************************************************************/
#include "can_private.h"


ssize_t can_read (struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    struct canbus_file_t *file;
    struct kcanbus_message *message;
    struct list_head *element;
    ssize_t ret;
    unsigned long flags;
    struct canbus_device_t *dev;

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

    file->stats.read_count++;
    
    /*
     *  After opening, a user space app still needs to tell us that it's
     *  ready to receive messages.
     */
    if (!file->accept_messages){
        return -EBUSY;
    }
    
    if (count < sizeof(CANBUS_MESSAGE)){
        return -EPROTO;
    }

    /*
     *  LOCK --------------------------------------------------------
     */
    spin_lock_irqsave(&dev->register_lock, flags);

    while (list_empty(&file->receive_queue)){

        /*
         *  UNLOCK --------------------------------------------------
         */
        spin_unlock_irqrestore(&dev->register_lock, flags);

        if ( wait_event_interruptible(  file->receive_wq, 
                    !list_empty(&file->receive_queue))){
            return -ERESTARTSYS;
        }

        /*
         *  LOCK --------------------------------------------------------
         */
        spin_lock_irqsave(&dev->register_lock, flags);
    }

    element = file->receive_queue.next;
    list_del(element);
    message = list_entry(element, struct kcanbus_message, entry);
    
    file->stats.cur_rx_queue_count--;

    /*
     *  UNLOCK ------------------------------------------------------
     */
    spin_unlock_irqrestore(&dev->register_lock, flags);

    if (copy_to_user(buf, &message->user_message, sizeof(CANBUS_MESSAGE))){
        ret = -EFAULT;
    }
    else{
        ret = sizeof(CANBUS_MESSAGE);
    }
    
    free_kcanbus_message(message);

    return ret;
}

