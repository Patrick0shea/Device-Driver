#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE "/dev/devicedriver"

int main() {
    int fd;
    char buffer[64];

    printf("Starting Roulette Spin...\n");

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

    // Open the device for reading the result
    fd = open(DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device for reading");
        return 1;
    }

    ssize_t ret = read(fd, buffer, sizeof(buffer) - 1);
    if (ret < 0) {
        perror("Failed to read from device");
        close(fd);
        return 1;
    }

    buffer[ret] = '\0'; // Null-terminate result
    printf("Winning LED: GPIO pin %s\n", buffer);

    close(fd); // Close after reading.

    return 0;
}
