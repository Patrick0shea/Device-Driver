#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

// Define a magic number for the IOCTL commands related to the roulette device
#define ROULETTE_MAGIC 'R'

// Define an IOCTL command to retrieve the winning LED number
#define IOCTL_GET_WINNING_LED _IOR(ROULETTE_MAGIC, 1, int *)

// Define the device path for the character device driver
#define DEVICE "/dev/devicedriver"

int main() {
    int fd;  // File descriptor for interacting with the device
    char buffer[64];  // Buffer for potential data storage, although not used here

    // Start of the roulette spin operation
    printf("\nStarting Roulette Spin...\n");

    // Open the device file for writing (this triggers the roulette spin)
    fd = open(DEVICE, O_WRONLY);
    if (fd < 0) {
        // Handle error if the device cannot be opened for writing
        perror("Failed to open device for writing");
        return 1;
    }

    printf("Device opened successfully.\n");

    // Send the spin command by writing "1" to the device
    if (write(fd, "1", 1) < 0) {
        // Handle error if the write operation fails
        perror("Failed to write to device");
        close(fd);  // Close the file descriptor before returning
        return 1;
    }

    printf("Spin command sent successfully.\n");

    // Close the device after sending the spin command
    close(fd);

    // Delay to allow the kernel module to finish the spin animation on the LEDs
    sleep(2);

    // Open the device file again, this time for reading the result via ioctl
    fd = open(DEVICE, O_RDONLY);
    if (fd < 0) {
        // Handle error if the device cannot be opened for reading
        perror("Failed to open device for ioctl");
        return 1;
    }

    int winning_led;  // Variable to hold the GPIO pin number of the winning LED

    // Use ioctl to retrieve the GPIO pin number of the winning LED
    if (ioctl(fd, IOCTL_GET_WINNING_LED, &winning_led) < 0) {
        // Handle error if the ioctl operation fails
        perror("Failed to get winning LED via ioctl");
        close(fd);  // Close the file descriptor before returning
        return 1;
    }

    // Display the GPIO pin of the winning LED
    printf("Winning LED: GPIO pin %d\n", winning_led);

    // Close the device file after retrieving the result
    close(fd);

    // End of the roulette spin operation
    return 0;
}
