#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define FIFO_PATH "temp/mi-fifo"

int main(int argc, char *argv[]) {
    int fd;
    char message[] = "Hola desde toad-cli!";

    fd = open(FIFO_PATH, O_WRONLY);
    if(fd = -1) {
        perror("Writer: open failed");
        exit(EXIT_FAILURE);
    }
    
    if (write(fd, message, strlen(message) + 1) == -1) {
        perror("Writer: write failed");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Writer: Message sent: %s<br>", message);
    close(fd);

    return EXIT_SUCCESS;
}