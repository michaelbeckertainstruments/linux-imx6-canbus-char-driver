/****************************************************************************
 *  can_private.c
 *
 *  Rather than include a huge list of headers and common API structures
 *  in multiple places, we include them here and include this file only.
 *  The driver is inherently coupled just about everywhere, so this is 
 *  only simplifying code clutter.
 *
 ***************************************************************************/
#ifndef CAN_PRIVATE_H__
#define CAN_PRIVATE_H__

#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regmap.h>
#include <linux/clk.h>
#include <linux/mfd/syscon.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/sched/rt.h>

#include "flexcan_registers.h"
#include "flexcan_bitrate.h"


#include "../inc/TaCanbusApi.h"



#define DEVICE_NAME "ta_canbus"
#define PRINTK_DEV_NAME DEVICE_NAME ": "

#define MINOR_DEV_NUMBER 0


/* Kcan */
#define KCANBUS_SIGNATURE   0x6E61634B

struct kcanbus_message {
    
    unsigned int signature;
    struct list_head entry;
    CANBUS_MESSAGE user_message;
};


/* CanD */
#define CANBUS_DEVICE_SIGNATURE 0x446e6143

struct canbus_device_t {

    unsigned int signature;         
    struct cdev cdev;                               /* Char device structure */
    spinlock_t register_lock;                       /* HW Lock, also for transmit_queue */
    struct FLEXCAN_HW_REGISTERS __iomem *registers; /* Access to the real Flexcan HW. */
    struct list_head transmit_queue;                /* Queue of messages to TX */
    struct list_head reader_list;                   /* List of open readers */
    int transmit_in_progress;                       /* Are we transmitting now? */
    int major_dev_number;                           /* Our major device number */
    dev_t devno;                                    /* Our devno */
    u32 clock_freq;                                 /* PER clock */
    struct clk *clk_ipg;                            /* Linux clock structs for this core */
    struct clk *clk_per;                            /* Linux clock structs for this core */
    struct resource *mem_resource;                  /* Memory resource from the dev tree*/
    resource_size_t mem_size;                       /* Size of the memory resource */
    int irq;                                        /* CANbus IRQ number */

    int stby_gpio;
    enum of_gpio_flags stby_gpio_flags;

    struct platform_device *pdev;

    struct regmap *gpr;
    int id;

    struct can_device_stats_t stats;
};


/* CanF */
#define CANBUS_FILE_SIGNATURE 0x466e6143

struct canbus_file_t {

    unsigned int signature;         /* Used to sanity check typecasted pointers. */
    struct canbus_device_t *dev;    /* Back pointer to the canbus_device_t structure. */
    int accept_messages;            /* We need to explicitly turn on getting messages. */

    struct list_head reader_list_entry; /* entry into dev->reader_list */

    struct list_head receive_queue;
    wait_queue_head_t receive_wq;

    struct can_file_stats_t stats;   
};


/**
 *  This is designed to be run as a threaded ISR.
 *  UPDATE - converted to a real top half isr.
 */
irqreturn_t can_irq_fn(int irq, void *dev_id);


/*
 *  File ops we implement here.
 */
int can_open (struct inode *, struct file *);
int can_release (struct inode *, struct file *);
ssize_t can_read (struct file *, char __user *, size_t, loff_t *);
ssize_t can_write (struct file *, const char __user *, size_t, loff_t *);
long can_ioctl (struct file *, unsigned int, unsigned long);    /* unlocked_ioctl */


/*
 *  /proc file registrations
 */
int add_can_proc_files(struct canbus_device_t *dev);
int remove_can_proc_files(struct canbus_device_t *dev);


/*
 *  Our own memory allocation routines.
 */
int init_kcanbus_message_pool(int max_msg_count);
void destroy_kcanbus_message_pool(void);
void free_kcanbus_message(struct kcanbus_message *msg);
struct kcanbus_message * alloc_kcanbus_message(void);


/****************************************************************************
 *  Hardware Accessors
 *
 *  You need to hold the register lock before accessing these,
 *  in general that is.
 */
void
hw_initialize_hardware(struct canbus_device_t *dev);

void
hw_transmit_message(struct canbus_device_t *dev,
                    CANBUS_MESSAGE *message);

/**
 *  Pull a message out of the MB.
 */
unsigned int
hw_receive_message( struct canbus_device_t *dev,
                    CANBUS_MESSAGE *message,
                    int message_buffer_index);

void 
hw_enable_message_buffer_interrupt( struct canbus_device_t *dev,
                                    int message_buffer_index);

void 
hw_disable_message_buffer_interrupt(struct canbus_device_t *dev,
                                    int message_buffer_index);

void 
hw_clear_message_buffer_interrupt(  struct canbus_device_t *dev,
                                    int message_buffer_index);

int
hw_is_message_buffer_interrupting(  struct canbus_device_t *dev,
                                    int message_buffer_index);

void
hw_enable_loopback_mode(struct canbus_device_t *dev);

void
hw_disable_loopback_mode(struct canbus_device_t *dev);

void
hw_enable_self_reception(struct canbus_device_t *dev);

void
hw_disable_self_reception(struct canbus_device_t *dev);

void
hw_abort_transmit(struct canbus_device_t *dev);

void
get_iflags(  struct canbus_device_t *dev,
            unsigned int *iflag1, 
            unsigned int *iflag2);

/***************************************************************************/


#endif


