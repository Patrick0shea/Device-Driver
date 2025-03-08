#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>

#define DEVICE "/dev/test2_roulette"

void *read_result(void *arg) {
    int fd = open(DEVICE, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return NULL;
    }
    
    char buffer[16];
    read(fd, buffer, sizeof(buffer));
    printf("Winning LED: %s\n", buffer);
    
    close(fd);
    return NULL;
}

int main() {
    int fd = open(DEVICE, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    printf("Starting Test2 Roulette Spin...\n");
    write(fd, "1", 1);  // Start spin

    pthread_t thread;
    pthread_create(&thread, NULL, read_result, NULL);
    
    pthread_join(thread, NULL);
    close(fd);
    
    return 0;
}
