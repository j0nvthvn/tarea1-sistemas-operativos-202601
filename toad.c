#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <errno.h>

#define FIFO_PATH "temp/mi-fifo"

static void toad();

int main() {
	toad();
	int fd;
	
	fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        syslog(LOG_ERR, "Reader: Open failed");
    }

	syslog(LOG_NOTICE, "toadd iniciado");
	
	while(1) {
		char buffer[100];
		if (read(fd, buffer, sizeof(buffer)) == -1) {
			syslog(LOG_ERR, "Reader: Read failed");
        	close(fd);
    	}

		syslog(LOG_NOTICE, "Reader: Message received");
				
	}

	// TODO: revisar log al finalizar el proceso
	// syslog(LOG_NOTICE, "toadd finalizado");
	closelog();
	return(EXIT_SUCCESS);
}

static void toad() {
	pid_t pid;

	pid = fork();

	if (pid < 0)
		exit(EXIT_FAILURE);

	if (pid > 0)
		exit(EXIT_SUCCESS);

	if (setsid() < 0 )
		exit(EXIT_FAILURE);

	pid = fork();

	if (pid < 0)
		exit(EXIT_FAILURE);

	if (pid > 0)
		exit(EXIT_SUCCESS);

	umask(0);

	chdir("/");

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	openlog("toad", LOG_PID, LOG_DAEMON);
}
