/****************************************************************************
 *  can_open.c
 *
 *  Contains the open and release routines for the driver.
 *
 ***************************************************************************/
#include "can_private.h"


/*
 *  A client is opening the device.
 */
int can_open (struct inode *inode, struct file *filp)
{
    struct canbus_device_t *dev;
    struct canbus_file_t *file;
    unsigned long flags;

    dev = container_of(inode->i_cdev, struct canbus_device_t, cdev);

    if (dev->signature != CANBUS_DEVICE_SIGNATURE){
        printk(KERN_ERR "Failed signature check! %s %d\n", __FILE__, __LINE__);
        return -EBADFD;
    }

    file = kmalloc(sizeof(struct canbus_file_t), GFP_KERNEL);
    if (!file){
        printk(KERN_ERR "Failed allocating memory in can_open()\n");
        return -ENOMEM;
    }

    memset(file, 0, sizeof(struct canbus_file_t));

    file->signature = CANBUS_FILE_SIGNATURE;
    file->dev = dev;

    INIT_LIST_HEAD(&file->receive_queue);
    init_waitqueue_head(&file->receive_wq);

    INIT_LIST_HEAD(&file->reader_list_entry);

    filp->private_data = file;

    /*
     *  LOCK --------------------------------------------------------
     */
    spin_lock_irqsave(&dev->register_lock, flags);

    list_add(&file->reader_list_entry, &dev->reader_list);

    /*
     *  UNLOCK ------------------------------------------------------
     */
    spin_unlock_irqrestore(&dev->register_lock, flags);

    /*
     *  We do not seek, this is a stream.
     */
    nonseekable_open(inode, filp);

    /*
     *  0 = Success
     */
    return 0;
}


/*
 *  Closing the Device.
 */
int can_release (struct inode *inode, struct file *filp)
{
    struct canbus_file_t *file;
    struct canbus_device_t *dev;
    struct kcanbus_message *message;
    struct list_head *element;
    unsigned long flags;

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

    /*
     *  LOCK --------------------------------------------------------
     */
    spin_lock_irqsave(&dev->register_lock, flags);

    list_del(&file->reader_list_entry);

    /*
     *  UNLOCK ------------------------------------------------------
     */
    spin_unlock_irqrestore(&dev->register_lock, flags);

    /*
     *  We are closing and we just unlinked ourselves from the 
     *  reader_list, no locks needed here.
     */
    while ( !list_empty(&file->receive_queue) ){

        element = file->receive_queue.next;
        list_del(element);
        message = list_entry(element, struct kcanbus_message, entry);
        
        if (message->signature != KCANBUS_SIGNATURE){
            /*
             *  "If" we get here, we have some sort of internal memory corruption.
             *  We can't know if this memory area is even ours, so we'll inform someone
             *  that we are broke, and ignore the memory.  This is a memory leak, but 
             *  done on purpose so we don't double free something, or free a pointer 
             *  that wasn't even allocated.
             */
            printk(KERN_ERR "Signature check Failed! %s %d\n", __FILE__, __LINE__);
        }
        else{
            free_kcanbus_message(message);
        }
    }

    kfree(file);

    return 0;
}


