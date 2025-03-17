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
#define ROULETTE_MAGIC 'R' // R is a magic number which helps distinguish commands belonging to different drivers.
// _IOR is a macro that acts as a shortcut for the compiler and kernel to recognize an IOCTL read request
#define IOCTL_GET_WINNING_LED _IOR(ROULETTE_MAGIC, 1, int *) // 1 = command number, int* = expected data type being read

// GPIO pins assigned to LEDs
static int gpio_pins[] = {535, 518, 529, 534, 524, 528, 532, 533};
#define NUM_LEDS (sizeof(gpio_pins) / sizeof(gpio_pins[0])) // a = total size of the array in bytes b = size of a single elements in bytes / = total number of elements in the array

// Device variables
static dev_t dev_number; // dev_t stores the major and minor numbers (major number identifies the driver associated with the device and the minor number differentiates devices managed by the same driver.)
static struct cdev roulette_cdev; // struct cdev is a structure that represents a character device in the kernel.
static struct class *roulette_class; // struct class * is a pointer to a device class.

// Game state variables
static int winning_led = -1; // Holds the index of the winning LED
static int spin_count = 0;   // Tracks number of spins

// Mutex to prevent concurrent access (macro in kernel source code)
static DEFINE_MUTEX(roulette_mutex);

// Function prototypes
static int dev_open(struct inode *, struct file *); // open function for character device, inode stores metadata about the file ,file represents file structure for the opened device.
static int dev_release(struct inode *, struct file *);// close function for character device 
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *); // read function for character device, struct file * represents the file being read, char __user * pointer to user-space buffer where the driver should copy data, number of bytes the user wants to read, pointer to where the file starts reading from.
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *); // write function for character device
static int proc_show(struct seq_file *m, void *v); // proc file output, seq_file *m helps format and display data when the file is read, void *v can store extra data if needed
static int proc_open(struct inode *inode, struct file *file); // open function for the proc file, file *file represents the proc file being opened.

// IOCTL function - return the winning LED
// struct file *file represents the device file. unsigned int cmd the IOCTL command being sent. unsigned long arg an argument passed from user space (used to transfer data).
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    // switch statement to handle different IOCTL commands.
    switch (cmd) {
        case IOCTL_GET_WINNING_LED:
        // This copies data from kernel space (winning_led) to user space (pointed to by arg).
            if (copy_to_user((int __user *)arg, &winning_led, sizeof(winning_led))) {
                return -EFAULT; // Return error if copy fails
            }
            return 0; // success 
        default:
        // If an unrecognized IOCTL command is received, it returns invalid argument error 
            return -EINVAL;
    }
}

// File operations structure for character device it defines the operations that the character device supports
// it links systems calls to the drivers function, allowing user space programs to interact with the device
// fops = structure that tells the kernel how to handle interactions with your device driver. links system calls to the actual functions in your driver.
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
// meta data about the file is stored in inode, file represents the file being opened
static int dev_open(struct inode *inodep, struct file *filep) {
    return 0;
}

// Close function - called when device file is closed
static int dev_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "device %s released\n", DEVICE_NAME);
    return 0;
}

// creates  a wwait queue for blocking readers until spin completes
static DECLARE_WAIT_QUEUE_HEAD(roulette_wait_queue);
static int spinning = 0;  // Indicates if the device is spinning 0 = not spinning, 1 = spinning




// Write function - spins the wheel, picks a winner, and blinks the winning LED
// *filep = file being written to, *buffer = points to user space (unused here), len = length of data being written, *offset = file offset (unused here)
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    int i, j; // loop counters
    int rounds = 40;  // Number of times LEDs will cycle

    mutex_lock(&roulette_mutex); // Lock to prevent concurrent writes. Only one spin can happen at a time
    spinning = 1; // Marks that the roulette is currently spinning. Other processes trying to read the result will have to wait.


    // Pick a random winning LED
    get_random_bytes(&winning_led, sizeof(winning_led)); // andom integer from the kernel’s random number generator
    winning_led = (winning_led < 0 ? -winning_led : winning_led) % NUM_LEDS; // ensure num is + with get_random_bytes and uses modulo to keep it within the range of the LEDs
    printk(KERN_INFO "dev_write: Spinning to select winning LED...\n"); // print to kernel log (debugging)

    spin_count++; // Increment spin count

    // Setup GPIOs
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_free(gpio_pins[i]); // free the GPIO pins if they werent already 
        if (gpio_request(gpio_pins[i], "sysfs")) {
            printk(KERN_WARNING "GPIO %d request failed\n", gpio_pins[i]);
        } else {
            gpio_direction_output(gpio_pins[i], 0); // Sets each pin as an output and turns off all LEDs initially 
        }
    }

    // Simulate spinning LEDs
    for (i = 0; i < rounds; i++) { // loop = 40 by default 
        int current_led = i % NUM_LEDS; // Calculates which LED should be ON at each step 
        for (j = 0; j < NUM_LEDS; j++) {
            gpio_set_value(gpio_pins[j], 0); // turn off all LEDs
        }
        gpio_set_value(gpio_pins[current_led], 1); // turn on the current LED
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
    spinning = 0; // Marks that the roulette is no longer spinning
    wake_up_interruptible(&roulette_wait_queue); // wake up any processes waiting for the wheel to stop spinning
    mutex_unlock(&roulette_mutex); // Unlocks the mutex, allowing another spin to start.

    return len; // Returns the number of bytes written (returning len tells the user-space program the write was successful)
}

// Read function - returns the winning LED index
// len = maximum number of bytes the user requested to read, *offset = the position in the file
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {

    char msg[16]; // character array (buffer) to store the winning LED number as a string
    int msg_len; // Stores the length of the formatted message.

    if (*offset > 0) return 0; // Prevent multiple reads, if > 0 the data was already read 

    wait_event_interruptible(roulette_wait_queue, spinning == 0);// If the roulette is still spinning, the process will sleep here. It will only continue when spinning == 0

    msg_len = snprintf(msg, sizeof(msg), "%d\n", winning_led);// Stores the winning_led as a string, snprintf ensures we don't overflow msg (max 16 bytes).

    if (copy_to_user(buffer, msg, msg_len)) return -EFAULT; // Copies the message (msg) from kernel space to user space (buffer). copy_to_user is needed because direct access to user-space memory is not allowed in the kernel.
    *offset += msg_len; // Updates the file offset so that a second read() will return 0. Prevents multiple reads from returnung the same result
    return msg_len; // Returns the number of bytes successfully read, The user-space program will receive msg_len bytes of data.
}

// Proc file show function - displays current status
// seq_file *m helps format and display data when the file is read and void *v = can store extra data if needed
static int proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "Winning LED: %d\n", winning_led);
    seq_printf(m, "Spin count: %d\n", spin_count);
    return 0;
}

// Proc file open function
// struct inode * represents the file metadata, struct file * represents the file being opened 
static int proc_open(struct inode *inode, struct file *file) {
    // Sets up the file to call proc_show() when /proc/roulette is read, generating the output once and eliminating the need for a custom open function.
    // file	= The proc file being opened, proc_show = The function that will generate the file's contents, NULL = Unused data
    return single_open(file, proc_show, NULL);
}

// Module initialization - registers character device
// __init;  function only runs during module initialization
static int __init test2_roulette_init(void) {
    // Assigns a device number (dev_number) for the character device.
    alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    // cdev_init links the device (roulette_cdev) to the file operations (fops).
    cdev_init(&roulette_cdev, &fops);
    // Registers the character device with the kernel.
    cdev_add(&roulette_cdev, dev_number, 1);
    // Creates a device class under /sys/class/CLASS_NAME/.
    roulette_class = class_create(CLASS_NAME);
    // Creates the /dev/roulette file, so user programs can access the device. uses dev_number to identify the device
    device_create(roulette_class, NULL, dev_number, NULL, DEVICE_NAME);
    // Creates a /proc/roulette_winner file. proc_fops defines how the file behaves
    proc_create("roulette_winner", 0, NULL, &proc_fops);
    // logs message in kernel for debugging
    printk(KERN_INFO "Test2 Roulette driver loaded\n");
    return 0;
}

// Module exit - cleans up resources
// __exit; function only runs during module exit
static void __exit test2_roulette_exit(void) {
    // Removes /dev/roulette, so user programs can no longer access it.
    device_destroy(roulette_class, dev_number);
    // Destroys the device class (/sys/class/CLASS_NAME/). This cleans up the class created in test2_roulette_init.
    class_destroy(roulette_class);
    // Removes the character device from the kernel. This step is required before freeing the device number.
    cdev_del(&roulette_cdev);
    // Releases the major/minor numbers assigned to the device.
    unregister_chrdev_region(dev_number, 1);
    // Deletes /proc/roulette_winner, preventing further reads
    remove_proc_entry("roulette_winner", NULL);
    // logs message in kernel for debugging
    printk(KERN_INFO "Test2 Roulette driver unloaded\n");
}

module_init(test2_roulette_init); // which function to call when loading the module
module_exit(test2_roulette_exit); // which function to call when unloading the module

MODULE_LICENSE("GPL"); // free software!!!
MODULE_AUTHOR("Emmett and Patrick");
MODULE_DESCRIPTION("Test2 Roulette Wheel LED Driver for Raspberry Pi");
