#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/random.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ioctl.h>
#include <linux/wait.h>

#define DEVICE_NAME "roulette_driver"
#define CLASS_NAME "roulette_driver"
#define ROULETTE_MAGIC 'R'
#define IOCTL_GET_WINNING_LED _IOR(ROULETTE_MAGIC, 1, int *)

static int gpio_pins[] = {535, 518, 529, 534, 524, 528, 532, 533};
#define NUM_LEDS (sizeof(gpio_pins) / sizeof(gpio_pins[0]))

static dev_t dev_number;
static struct cdev roulette_cdev;
static struct class *roulette_class;

static int winning_led = -1;
static int spin_count = 0;

// Mutex to prevent concurrent access
static DEFINE_MUTEX(roulette_mutex);

// Function prototypes
static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char __user *, size_t, loff_t *);
static int proc_show(struct seq_file *m, void *v);
static int proc_open(struct inode *inode, struct file *file);

// IOCTL
static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case IOCTL_GET_WINNING_LED:
            if (copy_to_user((int __user *)arg, &winning_led, sizeof(winning_led))) {
                return -EFAULT;
	    }
            return 0;
        default:
            return -EINVAL;
    }
}

// File operations for device
static struct file_operations fops = {
    .open = dev_open,
    .release = dev_release,
    .write = dev_write,
    .read = dev_read,
    .unlocked_ioctl = dev_ioctl,
};

// Proc file operations
static const struct proc_ops proc_fops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
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

static DECLARE_WAIT_QUEUE_HEAD(roulette_wait_queue);
static int spinning = 0;  // Indicates if the device is spinning

// Write function - spin the wheel and pick winner
static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset) {
    int i, j;
    int rounds = 40;  // Total LED cycles

    mutex_lock(&roulette_mutex);  // Block other writes
    spinning = 1;

    get_random_bytes(&winning_led, sizeof(winning_led));
    winning_led = (winning_led < 0 ? -winning_led : winning_led) % NUM_LEDS;
    printk(KERN_INFO "dev_write: Spinning to select winning LED...\n");

    spin_count++;

    // Ensure GPIOs are freed before requesting
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_free(gpio_pins[i]);
        if (gpio_request(gpio_pins[i], "sysfs")) {
            printk(KERN_WARNING "GPIO %d request failed\n", gpio_pins[i]);
        } else {
            gpio_direction_output(gpio_pins[i], 0);
        }
    }

    for (i = 0; i < rounds; i++) {
        int current_led = i % NUM_LEDS;
        for (j = 0; j < NUM_LEDS; j++) {
            gpio_set_value(gpio_pins[j], 0);
        }
        gpio_set_value(gpio_pins[current_led], 1);
        msleep(50 + (i * 2));
    }

    winning_led = (winning_led + 1) % NUM_LEDS;
    printk(KERN_INFO "dev_write: Winning LED is GPIO pin %d\n", gpio_pins[winning_led]);

    for (i = 0; i < 5; i++) {
        gpio_set_value(gpio_pins[winning_led], 1);
        msleep(500);
        gpio_set_value(gpio_pins[winning_led], 0);
        msleep(500);
    }

    for (i = 0; i < NUM_LEDS; i++) {
        gpio_set_value(gpio_pins[i], 0);
        gpio_direction_output(gpio_pins[i], 0);
        gpio_direction_input(gpio_pins[i]);
    }

    spinning = 0;  // Spin complete
    wake_up_interruptible(&roulette_wait_queue);  // Wake up blocked readers
    mutex_unlock(&roulette_mutex);

    return len;
}

// Read function - return the winning LED
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset) {
    char msg[16];
    int msg_len;

    if (*offset > 0)
        return 0;

    wait_event_interruptible(roulette_wait_queue, spinning == 0);  // Block until spin is done


    while (mutex_is_locked(&roulette_mutex)) {
	msleep(100); // Sleep and retry if spin is ongoing
    }

    msg_len = snprintf(msg, sizeof(msg), "%d\n", winning_led);

    if (len < msg_len)
        return -EINVAL;

    if (copy_to_user(buffer, msg, msg_len))
        return -EFAULT;

    *offset += msg_len;
    return msg_len;
}

// Proc file show function
static int proc_show(struct seq_file *m, void *v) {
    seq_printf(m, "Winning LED: %d\n", winning_led);
    seq_printf(m, "Spin count: %d\n", spin_count);
    return 0;
}

// Proc file open function
static int proc_open(struct inode *inode, struct file *file) {
    return single_open(file, proc_show, NULL);
}

// Module init
static int __init test2_roulette_init(void) {
    int ret;

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_INFO "Failed to allocate major\n");
        return ret;
    }

    cdev_init(&roulette_cdev, &fops);

    ret = cdev_add(&roulette_cdev, dev_number, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    roulette_class = class_create(CLASS_NAME);
    if (IS_ERR(roulette_class)) {
        cdev_del(&roulette_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(roulette_class);
    }

    if (device_create(roulette_class, NULL, dev_number, NULL, DEVICE_NAME) == NULL) {
        class_destroy(roulette_class);
        cdev_del(&roulette_cdev);
        unregister_chrdev_region(dev_number, 1);
        return -1;
    }

    proc_create("roulette_winner", 0, NULL, &proc_fops);

    printk(KERN_INFO "Test2 Roulette driver loaded\n");
    return 0;
}

// Module exit
static void __exit test2_roulette_exit(void) {
    int i;
    for (i = 0; i < NUM_LEDS; i++) {
        gpio_set_value(gpio_pins[i], 0);  // Ensure all LEDs are OFF
        gpio_direction_output(gpio_pins[i], 0);
        gpio_direction_input(gpio_pins[i]);  // Set GPIOs to input
        gpio_free(gpio_pins[i]);  // Properly free GPIO
    }

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
