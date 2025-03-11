#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/random.h>

#define DEVICE_NAME "roulette_driver"
#define CLASS_NAME "roulette_driver"

static int gpio_pins[] = {23, 6, 17, 27, 22, 12 , 16, 20, 21};
#define NUM_LEDS (sizeof(gpio_pins) / sizeof(gpio_pins[0]))

static dev_t dev_number;
static struct cdev roulette_cdev;
static struct class *roulette_class;

static int winning_led = -1;

// Function prototypes
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);

// File operations
static struct file_operations fops = {
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
    .read = dev_read
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

// Write function - spin the wheel and pick winner
// Write function - spin the wheel and pick winner
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    int i, j;
    int rounds = 40; // Total LED cycles
    get_random_bytes(&winning_led, sizeof(winning_led));
    winning_led = (winning_led < 0 ? -winning_led : winning_led) % NUM_LEDS;

    printk(KERN_INFO "dev_write: Spinning to select winning LED...\n");

    // Set all LEDs to off
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_request(gpio_pins[i], "sysfs");
        gpio_direction_output(gpio_pins[i], 0);
    }

    // Spin the LEDs and slowly increase speed
    for (i = 0; i < rounds + winning_led; i++) {
        int current_led;  // Renamed the variable to avoid conflict
        current_led = i % NUM_LEDS;  // Assign the value to current_led
        for (j = 0; j < NUM_LEDS; j++) {
            gpio_set_value(gpio_pins[j], 0);  // Turn off all LEDs
        }
        gpio_set_value(gpio_pins[current_led], 1);  // Turn on the current LED
        msleep(50 + (i * 2)); // Slow down gradually
    }

    // Highlight the winning LED
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_set_value(gpio_pins[i], i == winning_led);  // Set the winning LED
    }

    printk(KERN_INFO "dev_write: Winning LED index is %d (GPIO %d)\n", winning_led, gpio_pins[winning_led]);
    return len;
}

// Read function - return the winning LED
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    char msg[16];
    int msg_len;

    if (*offset > 0)
        return 0;

    msg_len = snprintf(msg, sizeof(msg), "%d\n", winning_led);

    if (len < msg_len)
        return -EINVAL;

    if (copy_to_user(buffer, msg, msg_len))
        return -EFAULT;

    *offset += msg_len;
    return msg_len;
}

// Module init
static int __init test2_roulette_init(void) {
    int ret;

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_INFO "Failed to allocate major\n");
        return ret;
    }
    printk(KERN_INFO "Allocated major number %d, minor %d\n", MAJOR(dev_number), MINOR(dev_number));

    cdev_init(&roulette_cdev, &fops);

    ret = cdev_add(&roulette_cdev, dev_number, 1);
    if (ret < 0) {
        printk(KERN_INFO "Failed to add char device\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    roulette_class = class_create(CLASS_NAME);
    if (IS_ERR(roulette_class)) {
        printk(KERN_INFO "Failed to create class\n");
        cdev_del(&roulette_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(roulette_class);
    }

    if (device_create(roulette_class, NULL, dev_number, NULL, DEVICE_NAME) == NULL) {
        printk(KERN_INFO "Failed to create device\n");
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
    int i;
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_set_value(gpio_pins[i], 0);
        gpio_free(gpio_pins[i]);
    }

    device_destroy(roulette_class, dev_number);
    class_destroy(roulette_class);
    cdev_del(&roulette_cdev);
    unregister_chrdev_region(dev_number, 1);

    printk(KERN_INFO "Test2 Roulette driver unloaded\n");
}

module_init(test2_roulette_init);
module_exit(test2_roulette_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emmett");
MODULE_DESCRIPTION("Test2 Roulette Wheel LED Driver for Raspberry Pi");
