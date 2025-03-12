/*
 * Linux Kernel Module for a Roulette Wheel LED Game
 * 
 * This module simulates a spinning roulette wheel using LEDs connected to GPIO pins.
 * A winning LED is selected at random, and the result can be read from user space.
 * It provides a character device driver interface and a /proc file for status.
 */

#include <linux/module.h>   // Required for all kernel modules
#include <linux/fs.h>       // File operations
#include <linux/uaccess.h>  // User-space memory access functions
#include <linux/cdev.h>     // Character device support
#include <linux/gpio.h>     // GPIO control functions
#include <linux/delay.h>    // Delays (e.g., msleep)
#include <linux/random.h>   // Random number generation
#include <linux/mutex.h>    // Mutex locks for synchronization
#include <linux/proc_fs.h>  // /proc file system support
#include <linux/seq_file.h> // Sequential file operations
#include <linux/ioctl.h>    // IOCTL support
#include <linux/wait.h>     // Wait queues for blocking operations

// Device and class names
#define DEVICE_NAME "roulette_driver"
#define CLASS_NAME "roulette_driver"

// IOCTL command definitions
#define ROULETTE_MAGIC 'R'
#define IOCTL_GET_WINNING_LED _IOR(ROULETTE_MAGIC, 1, int *)

// GPIO pins assigned to LEDs
static int gpio_pins[] = {535, 518, 529, 534, 524, 528, 532, 533};
#define NUM_LEDS (sizeof(gpio_pins) / sizeof(gpio_pins[0])) // Number of LEDs

// Device variables
static dev_t dev_number; // Device number (major/minor)
static struct cdev roulette_cdev; // Character device structure
static struct class *roulette_class; // Device class

// Game state variables
static int winning_led = -1; // Holds the index of the winning LED
static int spin_count = 0;   // Tracks number of spins

// Mutex to prevent concurrent access
static DEFINE_MUTEX(roulette_mutex);

// Function prototypes
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);
static int proc_show(struct seq_file *m, void *v);
static int proc_open(struct inode *inode, struct file *file);

// IOCTL function - return the winning LED
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case IOCTL_GET_WINNING_LED:
            if (copy_to_user((int __user *)arg, &winning_led, sizeof(winning_led))) {
                return -EFAULT; // Return error if copy fails
            }
            return 0;
        default:
            return -EINVAL; // Invalid command
    }
}

// File operations structure for character device
static struct file_operations fops = {
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
    .read = dev_read,
    .unlocked_ioctl = dev_ioctl,
};

// Proc file operations structure
static const struct proc_ops proc_fops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

// Open function - called when device file is opened
static int dev_open(struct inode *inodep, struct file *filep) {
    return 0;
}

// Close function - called when device file is closed
static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "device %s released\n", DEVICE_NAME);
    return 0;
}

// Wait queue for blocking readers until spin completes
static DECLARE_WAIT_QUEUE_HEAD(roulette_wait_queue);
static int spinning = 0;  // Indicates if the device is spinning

// Write function - spins the wheel, picks a winner, and blinks the winning LED
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    int i, j;
    int rounds = 40;  // Number of times LEDs will cycle

    mutex_lock(&roulette_mutex); // Lock to prevent concurrent writes
    spinning = 1;

    // Pick a random winning LED
    get_random_bytes(&winning_led, sizeof(winning_led));
    winning_led = (winning_led < 0 ? -winning_led : winning_led) % NUM_LEDS;
    printk(KERN_INFO "dev_write: Spinning to select winning LED...\n");

    spin_count++; // Increment spin count

    // Setup GPIOs
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_free(gpio_pins[i]);
        if (gpio_request(gpio_pins[i], "sysfs")) {
            printk(KERN_WARNING "GPIO %d request failed\n", gpio_pins[i]);
        } else {
            gpio_direction_output(gpio_pins[i], 0);
        }
    }

    // Simulate spinning LEDs
    for (i = 0; i < rounds; i++) {
        int current_led = i % NUM_LEDS;
        for (j = 0; j < NUM_LEDS; j++) {
            gpio_set_value(gpio_pins[j], 0);
        }
        gpio_set_value(gpio_pins[current_led], 1);
        msleep(50 + (i * 2)); // Increasing delay for slowdown effect
    }

    // Flash winning LED
    for (i = 0; i < 5; i++) {
        gpio_set_value(gpio_pins[winning_led], 1);
        msleep(500);
        gpio_set_value(gpio_pins[winning_led], 0);
        msleep(500);
    }

    // Cleanup and signal completion
    spinning = 0;
    wake_up_interruptible(&roulette_wait_queue);
    mutex_unlock(&roulette_mutex);

    return len;
}

// Read function - returns the winning LED index
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    char msg[16];
    int msg_len;

    if (*offset > 0) return 0; // Prevent multiple reads

    wait_event_interruptible(roulette_wait_queue, spinning == 0);
    msg_len = snprintf(msg, sizeof(msg), "%d\n", winning_led);

    if (copy_to_user(buffer, msg, msg_len)) return -EFAULT;
    *offset += msg_len;
    return msg_len;
}

// Proc file show function - displays current status
static int proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "Winning LED: %d\n", winning_led);
    seq_printf(m, "Spin count: %d\n", spin_count);
    return 0;
}

// Proc file open function
static int proc_open(struct inode *inode, struct file *file) {
    return single_open(file, proc_show, NULL);
}

// Module initialization - registers character device
static int __init test2_roulette_init(void) {
    alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    cdev_init(&roulette_cdev, &fops);
    cdev_add(&roulette_cdev, dev_number, 1);
    roulette_class = class_create(CLASS_NAME);
    device_create(roulette_class, NULL, dev_number, NULL, DEVICE_NAME);
    proc_create("roulette_winner", 0, NULL, &proc_fops);
    printk(KERN_INFO "Test2 Roulette driver loaded\n");
    return 0;
}

// Module exit - cleans up resources
static void __exit test2_roulette_exit(void) {
    device_destroy(roulette_class, dev_number);
    class_destroy(roulette_class);
    cdev_del(&roulette_cdev);
    unregister_chrdev_region(dev_number, 1);
    remove_proc_entry("roulette_winner", NULL);
    printk(KERN_INFO "Test2 Roulette driver unloaded\n");
}

module_init(test2_roulette_init);
module_exit(test2_roulette_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Emmett");
MODULE_DESCRIPTION("Test2 Roulette Wheel LED Driver for Raspberry Pi");
