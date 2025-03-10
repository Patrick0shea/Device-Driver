#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE "/dev/test2_roulette"

int main() {
    int fd;

    printf("Starting Test2 Roulette Spin...\n");

    // Open the device for writing to trigger the spin
    fd = open(DEVICE, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device for writing");
        return 1;
    }

    // Write a dummy value to start the spin
    if (write(fd, "1", 1) < 0) {
        perror("Failed to write to device");
        close(fd);
        return 1;
    }
    close(fd); // Done with writing

    // Open the device for reading the result
    fd = open(DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device for reading");
        return 1;
    }

    char buffer[16];
    ssize_t ret = read(fd, buffer, sizeof(buffer));
    if (ret < 0) {
        perror("Read failed");
        close(fd);
        return 1;
    }

    printf("Winning LED: %s\n", buffer);
    close(fd);

    return 0;
}
