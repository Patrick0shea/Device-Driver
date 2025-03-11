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

#define DEVICE_NAME "roulette_driver"
#define CLASS_NAME "roulette_driver"
#define PROC_FILENAME "roulette_driver"

static dev_t dev_number;
static struct cdev roulette_cdev;
static struct class *roulette_class;

// Proc entry
static struct proc_dir_entry *proc_file;

// Function prototypes
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset);

// File operations
static struct file_operations fops = {
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
    .read = dev_read
};

// Function definitions

// Open device
static int dev_open(struct inode *inodep, struct file *filep) {
    return 0;
}

// Close device
static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "device %s released\n", DEVICE_NAME);
    return 0;
}

// Write function
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    printk(KERN_INFO "dev_write: received %zu bytes from userspace\n", len);
    return len; // simulate successful write
}

// Read function
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    char *msg = "Hello from the kernel!\n";
    size_t msg_len = strlen(msg);

    if (len < msg_len)
        return -EINVAL; // buffer too small

    if (copy_to_user(buffer, msg, msg_len))
        return -EFAULT;

    return msg_len;
}

// Module init
static int __init test2_roulette_init(void) {
    int ret;

    // Dynamically allocate major and minor number
    // based on available major and minor numbers registered in the kernel
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_INFO "Failed to allocate major\n");
        return ret;
    }
    printk(KERN_INFO "Allocated the major number %d, and the minor %d\n", MAJOR(dev_number), MINOR(dev_number));

    // Initialize character device object with file operations 'fops'
    cdev_init(&roulette_cdev, &fops);

    // Add the character device
    ret = cdev_add(&roulette_cdev, dev_number, 1);
    if (ret < 0) {
        printk(KERN_INFO "failed to add character device to the kernel\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    // Create the device class in the kernel
    roulette_class = class_create(CLASS_NAME);
    if (IS_ERR(roulette_class)) {
        printk(KERN_INFO "Failed to create class for the device");
        cdev_del(&roulette_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(roulette_class);
    }

    // Create the device in the kernel
    if (device_create(roulette_class, NULL, dev_number, NULL, DEVICE_NAME) == NULL) {
        printk(KERN_INFO "Failed to create device\n");

        // Clean up already created kernel data
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
MODULE_AUTHOR("Emmett");
MODULE_DESCRIPTION("Test2 Roulette Wheel LED Driver for Raspberry Pi");
