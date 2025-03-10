// using this

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

#define DEVICE_NAME "test2_roulette"
#define CLASS_NAME "test2_roulette_class"

// GPIO pins for LEDs
static int gpio_pins[] = {23, 6, 17, 27, 22, 12, 16, 20, 21};
#define NUM_LEDS (sizeof(gpio_pins) / sizeof(gpio_pins[0]))

static dev_t dev_number;
static struct cdev roulette_cdev;
static struct class *roulette_class;
static struct device *roulette_device;
static wait_queue_head_t wq;
static int spin_in_progress = 0;

// Function prototypes
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);
static long dev_ioctl(struct file *, unsigned int, unsigned long);
static void spin_wheel(void);

static struct file_operations fops = {
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl,
};

// Open device
static int dev_open(struct inode *inodep, struct file *filep) {
    return 0;
}

// Close device
static int dev_release(struct inode *inodep, struct file *filep) {
    return 0;
}

// Read function (blocks until spin stops)
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    wait_event_interruptible(wq, spin_in_progress == 0);
    char result[16];
    int led_index = get_random_u32() % NUM_LEDS;
    snprintf(result, sizeof(result), "LED: %d\n", led_index);
    if (copy_to_user(buffer, result, strlen(result) + 1)){
        return -EFAULT;
}
    return strlen(result) + 1;
}

// Write function (start spin)
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    if (spin_in_progress) {
        return -EBUSY;
    }
    spin_wheel();
    return len;
}

// IOCTL (start/stop)
static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg) {
    if (cmd == 0) {
        spin_wheel();
    }
    return 0;
}

// Spin function
static void spin_wheel(void) {
    spin_in_progress = 1;
    int i, j;
    for (i = 0; i < 10; i++) {  // Simulate spinning effect
        for (j = 0; j < NUM_LEDS; j++) {
            gpio_set_value(gpio_pins[j], 1);
            msleep(100);
            gpio_set_value(gpio_pins[j], 0);
        }
    }
    spin_in_progress = 0;
    wake_up_interruptible(&wq);
    // Module init
static int __init test2_roulette_init(void) {
    int i;
    
    if (alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME) < 0) 
        return -1;

    cdev_init(&roulette_cdev, &fops);
    if (cdev_add(&roulette_cdev, dev_number, 1) < 0) 
        return -1;
    
    roulette_class = class_create(CLASS_NAME);
    if (IS_ERR(roulette_class)) {
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(roulette_class);
}

    roulette_device = device_create(roulette_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(roulette_device)) {
        class_destroy(roulette_class);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(roulette_device);
}

    // Initialize GPIOs
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_free(gpio_pins[i]);

        if (gpio_request(gpio_pins[i], "sysfs")) {
            printk(KERN_ERR "Failed to request GPIO %d\n", gpio_pins[i]);

        while(i > 0) {
                i--;
                gpio_free(gpio_pins[i]);
        }

            return -1;
        }
        printk(KERN_INFO "Successfully requested GPIO %d\n", gpio_pins[i]);
        gpio_direction_output(gpio_pins[i], 0);

    }

    init_waitqueue_head(&wq);
    printk(KERN_INFO "Test2 Roulette driver loaded\n");

    return 0;  // <---- Missing return statement added!
    static void __exit test2_roulette_exit(void) {
    int i;
    
    // Remove device

if (roulette_device) {
    device_destroy(roulette_class, dev_number);
        roulette_device = NULL;
}
    // Remove class (Fix for -EEXIST error)
    if (roulette_class) {
        class_destroy(roulette_class);
        roulette_class = NULL;
    }


    // Unregister char device
    cdev_del(&roulette_cdev);
    unregister_chrdev_region(dev_number, 1);

    // Free GPIOs
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_free(gpio_pins[i]);
    }

    printk(KERN_INFO "Test2 Roulette driver unloaded\n");
}

////////
module_init(test2_roulette_init);
module_exit(test2_roulette_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Patrick");
MODULE_DESCRIPTION("Test2 Roulette Wheel LED Driver for Raspberry Pi");

