/****************************************************************************
 *  can_ioctl.c
 *
 *  This implements the unlocked_ioctl call.  We handle misc. HW and 
 *  software logic here.  Most of the work for CANbus is done via 
 *  reads and writes.  Much of the ioctl API was for initial debugging 
 *  and bring-up, but there are some required calls here too.
 ***************************************************************************/
#include "can_private.h"


/* 
 * unlocked_ioctl 
 */
long can_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)    
{
    struct canbus_file_t *file;
    struct canbus_device_t *dev;
    unsigned int reg;
    unsigned long flags;

    /*
     *  Recover our file and device data.  Sanity check it.
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

	switch(cmd){

		case CAN_IOCTL_READ_MCR:
			reg = ioread32(&dev->registers->MCR);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;

		case CAN_IOCTL_READ_CTRL1:
			reg = ioread32(&dev->registers->CTRL1);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;

		case CAN_IOCTL_READ_TIMER:
			reg = ioread32(&dev->registers->TIMER);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;

		case CAN_IOCTL_READ_ECR:
			reg = ioread32(&dev->registers->ECR);
			break;

		case CAN_IOCTL_READ_ESR1:
			reg = ioread32(&dev->registers->ESR1);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;

		case CAN_IOCTL_READ_IMASK2:
			reg = ioread32(&dev->registers->IMASK2);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;

		case CAN_IOCTL_READ_IMASK1:
			reg = ioread32(&dev->registers->IMASK1);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;

		case CAN_IOCTL_READ_IFLAG2:
			reg = ioread32(&dev->registers->IFLAG2);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;

		case CAN_IOCTL_READ_IFLAG1:
			reg = ioread32(&dev->registers->IFLAG1);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;

		case CAN_IOCTL_READ_CTRL2:
			reg = ioread32(&dev->registers->CTRL2);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;

		case CAN_IOCTL_READ_ESR2:
			reg = ioread32(&dev->registers->ESR2);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;

		case CAN_IOCTL_READ_GFWR:
			reg = ioread32(&dev->registers->GFWR);
            if (copy_to_user((void *)arg, &reg, sizeof(unsigned int))){
                return -EFAULT;
            }
			break;


		case CAN_IOCTL_ENABLE_LOOPBACK:
            /*
             *  LOCK --------------------------------------------------------
             */
            spin_lock_irqsave(&dev->register_lock, flags);

            hw_enable_loopback_mode(dev);

            /*
             *  UNLOCK --------------------------------------------------------
             */
            spin_unlock_irqrestore(&dev->register_lock, flags);
			break;


        case CAN_IOCTL_DISABLE_LOOPBACK:
            /*
             *  LOCK --------------------------------------------------------
             */
            spin_lock_irqsave(&dev->register_lock, flags);

            hw_disable_loopback_mode(dev);

            /*
             *  UNLOCK --------------------------------------------------------
             */
            spin_unlock_irqrestore(&dev->register_lock, flags);
			break;


		case CAN_IOCTL_ENABLE_SELF_RECEPTION:
            /*
             *  LOCK --------------------------------------------------------
             */
            spin_lock_irqsave(&dev->register_lock, flags);

            hw_enable_self_reception(dev);

            /*
             *  UNLOCK --------------------------------------------------------
             */
            spin_unlock_irqrestore(&dev->register_lock, flags);
			break;


        case CAN_IOCTL_DISABLE_SELF_RECEPTION:
            /*
             *  LOCK --------------------------------------------------------
             */
            spin_lock_irqsave(&dev->register_lock, flags);

            hw_disable_self_reception(dev);

            /*
             *  UNLOCK --------------------------------------------------------
             */
            spin_unlock_irqrestore(&dev->register_lock, flags);
			break;

        /*
         *  Tells us that you are ready to start receiving messages.
         */
        case CAN_IOCTL_ENABLE_MESSAGE_ACCEPT:
            file->accept_messages = 1;
			break;


		default:
            printk(KERN_ERR PRINTK_DEV_NAME "Unknown IOCTL! %x\n", cmd);
			return -EINVAL;
	}

    return 0;
}


