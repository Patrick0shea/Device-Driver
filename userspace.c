#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define ROULETTE_MAGIC 'R'
#define IOCTL_GET_WINNING_LED _IOR(ROULETTE_MAGIC, 1, int *)
#define DEVICE "/dev/devicedriver"

int main() {
    int fd;
    char buffer[64];

    printf("\nStarting Roulette Spin...\n");

    // Open the device for writing to trigger the spin
    fd = open(DEVICE, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device for writing");
        return 1;
    }

    printf("Device opened successfully.\n");

    if (write(fd, "1", 1) < 0) {
        perror("Failed to write to device");
        close(fd);
        return 1;
    }

    printf("Spin command sent successfully.\n");

    close(fd); // Close after writing to allow reading operation to function.

    // Delay to allow spin to complete (ensure kernel finishes LED animation)
    sleep(2);

    // Open the device for retrieving the winning LED using ioctl
    fd = open(DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device for ioctl");
        return 1;
    }

    int winning_led;
    if (ioctl(fd, IOCTL_GET_WINNING_LED, &winning_led) < 0) {
        perror("Failed to get winning LED via ioctl");
        close(fd);
        return 1;
    }

    printf("Winning LED: GPIO pin %d\n", winning_led);

    close(fd); // Close after ioctl operation

    return 0;
}
