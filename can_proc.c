/****************************************************************************
 *  can_proc.c
 *
 *  Create a  /proc entry for statistics and debug.
 *
 *  This is based from:
 *  -------------------------
 *  https://www.linux.com/learn/linux-training/37985-the-kernel-newbie-corner-kernel-debugging-using-proc-qsequenceq-files-part-1
 *  https://www.linux.com/learn/linux-career-center/39972-kernel-debugging-with-proc-qsequenceq-files-part-2-of-3
 *  https://www.linux.com/learn/linux-career-center/44184-the-kernel-newbie-corner-kernel-debugging-with-proc-qsequenceq-files-part-3
 *
 ***************************************************************************/
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "can_private.h"


/*
 *  There should be a way to pass a user defined pointer back and forth
 *  between proc_create / proc_create_data, but I can't find it, and 
 *  this is only for debugging for now.
 */
static struct canbus_device_t *canbus_dev = NULL;


static int ta_canbus_proc_show(struct seq_file *m, void *v)
{
    struct list_head *element;
    struct canbus_file_t *file;

    /*
     *  We are deliberately doing this without the needed locks, so we 
     *  don't interfere with timing.  We accept occassional corrupt 
     *  data from our /proc file so we don't completely whack the 
     *  real timing needed for CANbus, and also make this info 
     *  useless.  Note that this is also not thread safe, if we 
     *  are clever (or evil), we maybe could get an oops here.
     */    
    if (canbus_dev->signature != CANBUS_DEVICE_SIGNATURE){
        printk( KERN_ERR PRINTK_DEV_NAME 
                "Failed signature check! %s %d\n", __FILE__, __LINE__);
        return 0;
    }

    seq_printf(m, "Isr %llu\n", canbus_dev->stats.isr_count);
    seq_printf(m, "TxWarn %u\n", canbus_dev->stats.tx_warn_count);
    seq_printf(m, "RxWarn %u\n", canbus_dev->stats.rx_warn_count);
    seq_printf(m, "BusOff %u\n", canbus_dev->stats.bus_off_count);
    seq_printf(m, "Bit1 %u\n", canbus_dev->stats.bit1_error_count);
    seq_printf(m, "Bit0 %u\n", canbus_dev->stats.bit0_error_count);
    seq_printf(m, "Ack %u\n", canbus_dev->stats.ack_error_count);
    seq_printf(m, "Crc %u\n", canbus_dev->stats.crc_error_count);
    seq_printf(m, "Form %u\n", canbus_dev->stats.form_error_count);
    seq_printf(m, "Stuff %u\n", canbus_dev->stats.bitstuff_error_count);

    seq_printf( m, "TotalIsrTime %ld sec %ld nsec\n", 
                canbus_dev->stats.total_isr_time.tv_sec, 
                canbus_dev->stats.total_isr_time.tv_nsec);
    seq_printf( m, "CurIsrTime %ld sec %ld nsec\n", 
                canbus_dev->stats.cur_isr_time.tv_sec, 
                canbus_dev->stats.cur_isr_time.tv_nsec);
    seq_printf( m, "MaxIsrTime %ld sec %ld nsec\n", 
                canbus_dev->stats.max_isr_time.tv_sec, 
                canbus_dev->stats.max_isr_time.tv_nsec);

    seq_printf(m, "TotalMbUsed %llu\n", canbus_dev->stats.total_mb_used);
    seq_printf(m, "CurMbUsed %u\n", canbus_dev->stats.cur_mb_used);
    seq_printf(m, "MaxMbUsed %u\n", canbus_dev->stats.max_mb_used);

    seq_printf(m, "CurTxQueueCount %u\n", canbus_dev->stats.cur_tx_queue_count);
    seq_printf(m, "MaxTxQueueCount %u\n", canbus_dev->stats.max_tx_queue_count);

    list_for_each(element, &canbus_dev->reader_list){

        file = list_entry(element, struct canbus_file_t, reader_list_entry);
        
        if (file->signature != CANBUS_FILE_SIGNATURE){
            printk(KERN_ERR "File Signature check Failed! %s %d\n", __FILE__, __LINE__);
            return 0;
        }

        if (!file->accept_messages){
            seq_printf(m, "\nfile not accepting messages yet\n");
            continue;
        }

        seq_printf(m, "\nReads %llu\n", file->stats.read_count);
        seq_printf(m, "Writes %llu\n", file->stats.write_count);
        seq_printf(m, "TxsQueued %llu\n", file->stats.write_transmits_queued);
        seq_printf(m, "TxsDirect %llu\n", file->stats.write_transmits_directly_sent);
        seq_printf(m, "CurReadsQueued %u\n", file->stats.cur_rx_queue_count);
        seq_printf(m, "MaxReadsQueued %u\n", file->stats.max_rx_queue_count);
    }

    return 0;
}


static int ta_canbus_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, ta_canbus_proc_show, NULL);
}


static const struct file_operations ta_canbus_proc_fops = {
    .owner      = THIS_MODULE,
    .open       = ta_canbus_proc_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = single_release,
};


int add_can_proc_files(struct canbus_device_t *dev)
{
    canbus_dev = dev;
    proc_create("ta_canbus", 0, NULL, &ta_canbus_proc_fops);
    return 0;
}


int remove_can_proc_files(struct canbus_device_t *dev)
{
    canbus_dev = NULL;
    remove_proc_entry("ta_canbus", NULL);
    return 0;
}


