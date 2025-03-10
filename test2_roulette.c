#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/random.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define DEVICE_NAME "test-Oisin"
#define CLASS_NAME "test-Oisin"
#define PROC_FILENAME "test-Oisin"

static dev_t dev_number;
static struct cdev roulette_cdev;
static struct class *roulette_class;

// Proc entry
static struct proc_dir_entry *proc_file;

// Function prototypes
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);

// File operations
static struct file_operations fops = {
    .open = dev_open,
    .release = dev_release
};

// Open device
static int dev_open(struct inode *inodep, struct file *filep) {
    return 0;
}

// Close device
static int dev_release(struct inode *inodep, struct file *filep) {
    	printk(KERN_INFO "device %s released\n", DEVICE_NAME);
	return 0;
}

// Module init
static int __init test2_roulette_init(void) {
    int ret;

    // dynamically allocate major and minor number
    // based on available major and minor nums registered in the kernel
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0)
    {
    	printk(KERN_INFO "Failed to allocate major\n");
    }
    printk(KERN_INFO "Allocated the major number %d, and the minor %d\n", MAJOR(dev_number), MINOR(dev_number));

    // initialize character device object with file operations 'fops'
    cdev_init(&roulette_cdev, &fops);

    // add the character device
    ret = cdev_add(&roulette_cdev, dev_number, 1);
    if (ret < 0)
    {
    	printk(KERN_INFO "failed to add character device to the kernel\n");
	unregister_chrdev_region(dev_number, 1);
	return ret;
    }

    // create the device class in the kernel
    roulette_class = class_create(CLASS_NAME);
    if (IS_ERR(roulette_class)) {
	printk(KERN_INFO "Failed to create class for the device");
	cdev_del(&roulette_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(roulette_class);
    }

    // create the device in the kernel
    if (device_create(roulette_class, NULL, dev_number, NULL, DEVICE_NAME) == NULL)
    {
    	printk(KERN_INFO "Failed to create device\n");

	// clean up already created kernel data
	//
	// destroy the device class
	class_destroy(roulette_class);
	cdev_del(&roulette_cdev);
	unregister_chrdev_region(dev_number, 1);
	return -1;
    }

    printk(KERN_INFO "Test2 Roulette driver loaded\n");
    return 0;
}

// Module exit
static void __exit test2_roulette_exit(void) {
    device_destroy(roulette_class, dev_number);
    class_destroy(roulette_class);
    cdev_del(&roulette_cdev);
    unregister_chrdev_region(dev_number, 1);
    proc_remove(proc_file);

    printk(KERN_INFO "Test2 Roulette driver unloaded\n");
}

module_init(test2_roulette_init);
module_exit(test2_roulette_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Patrick");
MODULE_DESCRIPTION("Test2 Roulette Wheel LED Driver for Raspberry Pi");
