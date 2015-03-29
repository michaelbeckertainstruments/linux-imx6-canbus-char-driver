/****************************************************************************
 *  can_init.c
 *
 *  Handles the init stuff, device tree interface, etc.  Borrows heavily 
 *  from flexcan.c in the actual kernel tree, with the exception that this 
 *  driver provides a char driver API, not a network API.
 *
 ***************************************************************************/
#include "can_private.h"


struct file_operations canbus_fops = {
    .owner =            THIS_MODULE,
    .read =             can_read,
    .write =            can_write,
    .unlocked_ioctl =   can_ioctl,
    .llseek =           no_llseek,
    .open =             can_open,
    .release =          can_release,    /* when the file struct is freed - TODO - fork / dup? */
    /*  .flush =    can_flush - If we need called on "every" close, implement this. */
};



static const struct of_device_id flexcan_of_match[] = {
    { .compatible = "fsl,imx6q-flexcan", },
    { /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, flexcan_of_match);



static const struct platform_device_id flexcan_id_table[] = {
    { .name = "flexcan", },
    { /* sentinel */ },
};
MODULE_DEVICE_TABLE(platform, flexcan_id_table);



static int flexcan_probe(struct platform_device *pdev)
{
    int err;
    int wakeup = 1;
    struct pinctrl *pinctrl;
    struct canbus_device_t *dev;

    printk(KERN_INFO PRINTK_DEV_NAME "flexcan_probe()\n");

    dev = kmalloc(sizeof(struct canbus_device_t), GFP_KERNEL);
    if (!dev){
        printk( KERN_ERR PRINTK_DEV_NAME 
                "Failed allocating memory in flexcan_probe()\n");
        err = -ENOMEM;
        goto FAILED_KMALLOC;
    }

    memset(dev, 0, sizeof(struct canbus_device_t));
    dev->signature = CANBUS_DEVICE_SIGNATURE;

    err = alloc_chrdev_region(&dev->devno, 0, 1, DEVICE_NAME);
    if (err < 0){
        printk( KERN_ERR PRINTK_DEV_NAME 
                "Failed alloc_chrdev_region() = %d\n", err);
        goto FAILED_ALLOC_CHRDEV_REGION;
    }

    dev->major_dev_number = MAJOR(dev->devno);

    printk( KERN_INFO PRINTK_DEV_NAME "devno=%d major_dev_num=%d\n",
            dev->devno, dev->major_dev_number);

    spin_lock_init(&dev->register_lock);

    INIT_LIST_HEAD(&dev->transmit_queue);
    INIT_LIST_HEAD(&dev->reader_list);

    err = init_kcanbus_message_pool(10000);

    if(err){
        printk(KERN_ERR PRINTK_DEV_NAME "Failed allocating canbus message pool!\n");
        err = -ENOMEM;
        goto FAILED_KMEM_CACHE_CREATE;
    }

    /*
     *  Enable the correct TX / RX pins for CANbus 
     *  as defined by the dev tree.
     */
    pinctrl = devm_pinctrl_get_select_default(&pdev->dev);
    if (IS_ERR(pinctrl)){
        err = PTR_ERR(pinctrl);
        printk( KERN_ERR PRINTK_DEV_NAME 
                "Failed setting up Flexcan TX/RX pins!! %d\n", err);
        goto FAILED_DEVM_PINCTRL_GET_SELECT_DEFAULT;
    }

    if (pdev->dev.of_node){
        of_property_read_u32(   pdev->dev.of_node,
                                "clock-frequency", 
                                &dev->clock_freq);
        printk( KERN_INFO PRINTK_DEV_NAME 
                "clock-frequency set manually in dev tree =%d\n", 
                dev->clock_freq);
    }
    if (!dev->clock_freq) {

        dev->clk_ipg = devm_clk_get(&pdev->dev, "ipg");
        if (IS_ERR(dev->clk_ipg)) {
            dev_err(&pdev->dev, "no ipg clock defined\n");
            err = PTR_ERR(dev->clk_ipg);
            goto FAILED_CLOCK;
        }

        dev->clk_per = devm_clk_get(&pdev->dev, "per");
        if (IS_ERR(dev->clk_per)) {
            dev_err(&pdev->dev, "no per clock defined\n");
            err = PTR_ERR(dev->clk_per);
            goto FAILED_CLOCK;
        }

        dev->clock_freq = clk_get_rate(dev->clk_per);
        printk( KERN_INFO PRINTK_DEV_NAME "clock_freq from PER=%d\n", dev->clock_freq);
    }

    dev->mem_resource = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!dev->mem_resource) {
        printk(KERN_ERR PRINTK_DEV_NAME "Failed platform_get_resource IORESOURCE_MEM\n");
        err = -ENODEV;
        goto FAILED_GET_MEM_RESOURCE;
    }
    printk( KERN_INFO PRINTK_DEV_NAME "mem_resource %s [%d, %d, %lx]\n", 
            dev->mem_resource->name, 
            dev->mem_resource->start, 
            dev->mem_resource->end, 
            dev->mem_resource->flags);

    dev->mem_size = resource_size(dev->mem_resource);
    if (!request_mem_region(dev->mem_resource->start, dev->mem_size, pdev->name)) {
        printk(KERN_ERR PRINTK_DEV_NAME "Failed request_mem_region\n");
        err = -EBUSY;
        goto FAILED_GET_MEM_RESOURCE_SIZE;
    }
    printk( KERN_INFO PRINTK_DEV_NAME "mem_resource: mem_size=%d\n", dev->mem_size);

    dev->registers = ioremap_nocache(dev->mem_resource->start, dev->mem_size);
    if (!dev->registers) {
        printk(KERN_ERR PRINTK_DEV_NAME "Failed ioremap\n");
        err = -ENOMEM;
        goto FAILED_IOREMAP;
    }
    printk( KERN_INFO PRINTK_DEV_NAME "registers: %p\n", dev->registers);

    dev->irq = platform_get_irq(pdev, 0);
    if (dev->irq <= 0) {
        printk(KERN_ERR PRINTK_DEV_NAME "Failed getting irq %d\n", dev->irq);
        err = -ENODEV;
        goto FAILED_GET_IRQ;
    }
    printk( KERN_INFO PRINTK_DEV_NAME "irq: %d\n", dev->irq);

    dev->stby_gpio = of_get_named_gpio_flags(   pdev->dev.of_node,
                                                "trx-stby-gpio", 
                                                0,
                                                &dev->stby_gpio_flags);
    if (gpio_is_valid(dev->stby_gpio)){
        printk( KERN_INFO PRINTK_DEV_NAME "stby_gpio valid: %d\n", dev->stby_gpio);
        gpio_request_one(dev->stby_gpio, GPIOF_DIR_OUT, "flexcan-trx-stby");
        gpio_direction_output(dev->stby_gpio, 1);
    }
    else{
        printk( KERN_INFO PRINTK_DEV_NAME "stby_gpio invalid: %d\n", dev->stby_gpio);
    }

    platform_set_drvdata(pdev, dev);
    dev->pdev = pdev;

	clk_prepare_enable(dev->clk_ipg);
	clk_prepare_enable(dev->clk_per);

    dev->gpr = syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "gpr");
    if (IS_ERR(dev->gpr)) {
        wakeup = 0;
        dev_dbg(&pdev->dev, "can not get grp\n");
    }

    dev->id = of_alias_get_id(pdev->dev.of_node, "flexcan");
    if (dev->id < 0) {
        wakeup = 0;
        dev_dbg(&pdev->dev, "can not get alias id\n");
    }

    device_set_wakeup_capable(&pdev->dev, wakeup);

    /*
     *  Set up the isr - a real top half handler now.
     */
    err = request_irq(  dev->irq, 
                        can_irq_fn,
                        IRQF_SHARED, /* needed? */
                        /*IRQF_SHARED | IRQF_ONESHOT, needed? */
                        /*IRQF_ONESHOT,  needed? */
                        DEVICE_NAME,
                        dev);
    if (err){
        printk( KERN_ERR PRINTK_DEV_NAME 
                "request_threaded_irq() FAILED  err=%d\n", err);
        goto FAILED_REQUEST_THREADED_IRQ;
    }

    /*
     *  Set up the Flexcan module itself.
     */
    hw_initialize_hardware(dev);

    /*  
     *  Enable the transcever
     */
    if (gpio_is_valid(dev->stby_gpio)) {
	    if (dev->stby_gpio_flags & OF_GPIO_ACTIVE_LOW){
   		    gpio_set_value(dev->stby_gpio, 0);
        }
        else{
   		    gpio_set_value(dev->stby_gpio, 1);
        }
	}

    /*
     *  Do this last!  After cdev_add(), we are live!
     */
    cdev_init(&dev->cdev, &canbus_fops);
    dev->cdev.owner = THIS_MODULE;

    err = cdev_add(&dev->cdev, dev->devno, 1);
    if (err){
        printk(KERN_ERR PRINTK_DEV_NAME "Failed adding Error %d\n", err);
        goto FAILED_CDEV_ADD;
    }

    add_can_proc_files(dev);

    dev_info(&pdev->dev, "device registered (reg_base=%p, irq=%d)\n",
         dev->registers, dev->irq);

    return 0;


/*
 *  Error cases and cleanup.
 */

FAILED_CDEV_ADD:
    free_irq(dev->irq, dev);

FAILED_REQUEST_THREADED_IRQ:

    platform_set_drvdata(pdev, NULL);
	clk_disable_unprepare(dev->clk_per);
	clk_disable_unprepare(dev->clk_ipg);

    if (gpio_is_valid(dev->stby_gpio)){
	    
        if (dev->stby_gpio_flags & OF_GPIO_ACTIVE_LOW){
   		    gpio_set_value(dev->stby_gpio, 0);
        }
        else{
   		    gpio_set_value(dev->stby_gpio, 1);
        }

        gpio_free(dev->stby_gpio);
    }

FAILED_GET_IRQ:
    iounmap(dev->registers);
    
FAILED_IOREMAP:
    release_mem_region( dev->mem_resource->start, dev->mem_size);
    
FAILED_GET_MEM_RESOURCE_SIZE:
FAILED_GET_MEM_RESOURCE:
FAILED_CLOCK:
FAILED_DEVM_PINCTRL_GET_SELECT_DEFAULT:
    destroy_kcanbus_message_pool();

FAILED_KMEM_CACHE_CREATE:
    unregister_chrdev_region(dev->devno, 1);

FAILED_ALLOC_CHRDEV_REGION:
    kfree(dev);

FAILED_KMALLOC:
    return err;
}


static int flexcan_remove(struct platform_device *pdev)
{
    struct canbus_device_t *dev = platform_get_drvdata(pdev);

    if (dev->signature != CANBUS_DEVICE_SIGNATURE){
        printk( KERN_ERR PRINTK_DEV_NAME 
                "Failed signature check! %s %d\n", __FILE__, __LINE__);
        return -1;
    }

    remove_can_proc_files(dev);

    cdev_del(&dev->cdev);

    free_irq(dev->irq, dev);
    platform_set_drvdata(pdev, NULL);
	clk_disable_unprepare(dev->clk_per);
	clk_disable_unprepare(dev->clk_ipg);

    if (gpio_is_valid(dev->stby_gpio)){
        if (dev->stby_gpio_flags & OF_GPIO_ACTIVE_LOW){
   		    gpio_set_value(dev->stby_gpio, 1);
        }
        else{
   		    gpio_set_value(dev->stby_gpio, 0);
        }
        gpio_free(dev->stby_gpio);
    }

    if (dev->registers){
        iounmap(dev->registers);
    }

    if (dev->mem_resource){
        release_mem_region( dev->mem_resource->start, 
                            dev->mem_size);
    }

    destroy_kcanbus_message_pool();

    unregister_chrdev_region(dev->devno, 1);

    kfree(dev);

    return 0;
}


static struct platform_driver flexcan_driver = {
    .driver = {
        .name = DEVICE_NAME,
        .owner = THIS_MODULE,
        .of_match_table = flexcan_of_match,
    },
    .probe = flexcan_probe,
    .remove = flexcan_remove,
    .id_table = flexcan_id_table,
};
module_platform_driver(flexcan_driver);


MODULE_AUTHOR(  "Michael Becker <mbecker@tainstruments.com>");

MODULE_AUTHOR(  "Sascha Hauer <kernel@pengutronix.de>, "
	            "Marc Kleine-Budde <kernel@pengutronix.de>");

MODULE_LICENSE("GPL v2");

MODULE_DESCRIPTION( "TA Instruments driver for i.MX6 Flexcan. "
                    "Using init code derived from the kernel "
                    "flexcan module.");

/*
 *  The version number is auto-incremented as we build.  So we pull it 
 *  out into another file to make it a bit easier to automatically parse and 
 *  revise from a script.
 */
#include "version.h"

