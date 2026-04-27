#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include "struct.h"

static void comandos(const char *c);

int main(int argc, char *argv[])
{

    if (argc < 2)
        comandos(argv[0]);

    if (strcmp(argv[1], "start") == 0 && argc != 3)
        comandos(argv[0]);

    if ((strcmp(argv[1], "stop") == 0 ||
         strcmp(argv[1], "status") == 0 ||
         strcmp(argv[1], "kill") == 0) &&
        argc != 3)
        comandos(argv[0]);

    if ((strcmp(argv[1], "ps") == 0 ||
         strcmp(argv[1], "zombie") == 0) &&
        argc != 2)
        comandos(argv[0]);

    if (strcmp(argv[1], "start") != 0 &&
        strcmp(argv[1], "stop") != 0 &&
        strcmp(argv[1], "ps") != 0 &&
        strcmp(argv[1], "status") != 0 &&
        strcmp(argv[1], "kill") != 0 &&
        strcmp(argv[1], "zombie") != 0)
        comandos(argv[0]);

    Request req;
    req.pid = getpid();
    snprintf(req.comando, sizeof(req.comando), "%s %s", argv[1], argc > 2 ? argv[2] : "");

    int fd_res;
    char fifo_path[64];
    snprintf(fifo_path, sizeof(fifo_path), "/tmp/toad-cli-%d.fifo", req.pid);
    if (mkfifo(fifo_path, 0666))
    {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    int fd = open("/tmp/toadd.fifo", O_WRONLY);
    write(fd, &req, sizeof(req));
    close(fd);

    fd_res = open(fifo_path, O_RDONLY);
    char response[1024];
    read(fd_res, response, sizeof(response));
    printf("%s", response);

    close(fd_res);
    unlink(fifo_path);

    return EXIT_SUCCESS;
}

static void comandos(const char *c)
{
    fprintf(stderr, "Uso:\n"
                    "%s start <bin_path>\n"
                    "%s stop <iid>\n"
                    "%s ps\n"
                    "%s status <iid>\n"
                    "%s kill <iid>\n"
                    "%s zombie\n",
            c, c, c, c, c, c

    );
    exit(EXIT_FAILURE);
}