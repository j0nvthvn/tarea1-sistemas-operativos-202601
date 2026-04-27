#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include "struct.h"

#define MAX_PROCESOS 64

Proceso procesos[MAX_PROCESOS];
int proceso_count = 0;

static volatile sig_atomic_t hay_hijos_terminados = 0;

static void toad();
void handler_sigchld(int sig);
static void revisar_hijos(void);

int main()
{
	toad();

	syslog(LOG_NOTICE, "toadd iniciado");

	// Crear FIFO principal
	if (mkfifo("/tmp/toadd.fifo", 0666) == -1 && errno != EEXIST)
	{
		syslog(LOG_ERR, "mkfifo falló: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	int fd = open("/tmp/toadd.fifo", O_RDWR);
	if (fd == -1)
	{
		syslog(LOG_ERR, "open falló: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler_sigchld;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		syslog(LOG_ERR, "sigaction(SIGCHLD) falló: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		// Leer request
		Request req;
		ssize_t n = read(fd, &req, sizeof(req));

		if (n <= 0)
		{
			if (errno == EINTR)
				continue;
			continue;
		}

		// Construir path del FIFO de respuesta
		char fifo_path[64];
		snprintf(fifo_path, sizeof(fifo_path), "/tmp/toad-cli-%d.fifo", req.pid);

		// Parsear comando
		char cmd[16];
		char arg[256];
		arg[0] = '\0';
		sscanf(req.comando, "%s %s", cmd, arg);
		syslog(LOG_NOTICE, "Comando recibido: %s", req.comando);

		if (hay_hijos_terminados)
		{
			hay_hijos_terminados = 0;
			revisar_hijos();
		}

		char respuesta[1024];
		respuesta[0] = '\0';

		if (strcmp(cmd, "start") == 0)
		{
			if (proceso_count >= MAX_PROCESOS)
			{
				snprintf(respuesta, sizeof(respuesta), "Error: limite de procesos alcanzado\n");
			}
			else if (access(arg, X_OK) == -1)
			{
				snprintf(respuesta, sizeof(respuesta), "Error: no se puede ejecutar %s\n", arg);
			}
			else
			{
				pid_t pid = fork();
				if (pid == 0)
				{
					close(fd);
					setpgid(0, 0);
					execl(arg, arg, NULL);
					syslog(LOG_ERR, "execlp falló: %s", strerror(errno));
					_exit(EXIT_FAILURE);
				}
				else if (pid > 0)
				{
					procesos[proceso_count].iid = proceso_count + 2;
					procesos[proceso_count].pid = pid;
					strcpy(procesos[proceso_count].binary, arg);
					strcpy(procesos[proceso_count].state, "RUNNING");
					procesos[proceso_count].start_time = time(NULL);
					proceso_count++;
					revisar_hijos();

					syslog(LOG_NOTICE, "Proceso iniciado: IID:%d PID:%d",
						   procesos[proceso_count - 1].iid,
						   procesos[proceso_count - 1].pid);

					snprintf(respuesta, sizeof(respuesta), "IID: %d\n", proceso_count + 1);
				}
				else
				{
					syslog(LOG_ERR, "fork() falló para %s", arg);
				}
			}
		}

		else if (strcmp(cmd, "stop") == 0)
		{
			int iid = atoi(arg);
			int encontrado = 0;
			for (int i = 0; i < proceso_count; ++i)
			{
				if (procesos[i].iid == iid)
				{
					encontrado = 1;
					syslog(LOG_NOTICE, "Intentando detener IID=%d PID=%d", procesos[i].iid, procesos[i].pid);
					if (kill(procesos[i].pid, SIGTERM) == 0)
					{
						strcpy(procesos[i].state, "STOPPED");
						syslog(LOG_NOTICE, "Señal SIGTERM enviada con éxito.\n");
						snprintf(respuesta, sizeof(respuesta), "\n");
					}
					else
					{
						syslog(LOG_ERR, "Error al enviar la señal: %s\n", strerror(errno));
						snprintf(respuesta, sizeof(respuesta), "Error: no se pudo detener IID %d\n", iid);
					}
					break;
				}
			}
			if (!encontrado)
				snprintf(respuesta, sizeof(respuesta), "Error: IID %d no encontrado.\n", iid);
		}

		else if (strcmp(cmd, "ps") == 0)
		{
			time_t ahora = time(NULL);
			snprintf(respuesta, sizeof(respuesta), "IID\tPID\tSTATE\tUPTIME\tBINARY\n");
			for (int i = 0; i < proceso_count; ++i)
			{
				char linea[512];
				long segundos = ahora - procesos[i].start_time;
				int horas = segundos / 3600;
				int minutos = (segundos % 3600) / 60;
				int segs = segundos % 60;
				snprintf(linea, sizeof(linea), "%d\t%d\t%s\t%02d:%02d:%02d\t%s\n",
						 procesos[i].iid,
						 procesos[i].pid,
						 procesos[i].state,
						 horas, minutos, segs,
						 procesos[i].binary);
				strncat(respuesta, linea, sizeof(respuesta) - strlen(respuesta) - 1);
			}
		}

		else if (strcmp(cmd, "status") == 0)
		{
			int iid = atoi(arg);
			int encontrado = 0;
			time_t ahora = time(NULL);
			for (int i = 0; i < proceso_count; ++i)
			{
				if (procesos[i].iid == iid)
				{
					encontrado = 1;
					char linea[512];
					long segundos = ahora - procesos[i].start_time;
					int horas = segundos / 3600;
					int minutos = (segundos % 3600) / 60;
					int segs = segundos % 60;
					snprintf(linea, sizeof(linea), "IID: %d\nPID: %d\nBINARY: %s\nSTATE: %s\nUPTIME: %02d:%02d:%02d\n",
							 procesos[i].iid,
							 procesos[i].pid,
							 procesos[i].binary,
							 procesos[i].state,
							 horas, minutos, segs);
					strncat(respuesta, linea, sizeof(respuesta) - strlen(respuesta) - 1);
					break;
				}
			}
			if (!encontrado)
				snprintf(respuesta, sizeof(respuesta), "Error: IID %d no encontrado.\n", iid);
		}

		else if (strcmp(cmd, "kill") == 0)
		{
			int iid = atoi(arg);
			int encontrado = 0;
			for (int i = 0; i < proceso_count; ++i)
			{
				if (procesos[i].iid == iid)
				{
					encontrado = 1;
					syslog(LOG_NOTICE, "Intentando detener IID=%d PID=%d", procesos[i].iid, procesos[i].pid);
					if (kill(-procesos[i].pid, SIGKILL) == 0)
					{
						strcpy(procesos[i].state, "STOPPED");
						syslog(LOG_NOTICE, "Señal SIGKILL enviada con éxito.\n");
						snprintf(respuesta, sizeof(respuesta), "\n");
					}
					else
					{
						syslog(LOG_ERR, "Error al enviar la señal: %s\n", strerror(errno));
						snprintf(respuesta, sizeof(respuesta), "Error: no se pudo matar IID %d\n", iid);
					}
					break;
				}
			}
			if (!encontrado)
				snprintf(respuesta, sizeof(respuesta), "Error: IID %d no encontrado.\n", iid);
		}

		else if (strcmp(cmd, "zombie") == 0)
		{
			time_t ahora = time(NULL);
			snprintf(respuesta, sizeof(respuesta), "IID\tPID\tSTATE\tUPTIME\tBINARY\n");
			for (int i = 0; i < proceso_count; ++i)
			{
				if (strcmp(procesos[i].state, "ZOMBIE") == 0)
				{
					{
						char linea[512];
						long segundos = ahora - procesos[i].start_time;
						int horas = segundos / 3600;
						int minutos = (segundos % 3600) / 60;
						int segs = segundos % 60;
						snprintf(linea, sizeof(linea), "%d\t%d\t%s\t%02d:%02d:%02d\t%s\n",
								 procesos[i].iid,
								 procesos[i].pid,
								 procesos[i].state,
								 horas, minutos, segs,
								 procesos[i].binary);
						strncat(respuesta, linea, sizeof(respuesta) - strlen(respuesta) - 1);
					}
				}
			}
		}

		// Responder a toad-cli :P
		int fd_res = open(fifo_path, O_WRONLY);
		write(fd_res, respuesta, strlen(respuesta) + 1);
		close(fd_res);
	}
	closelog();
	return (EXIT_SUCCESS);
}

static void toad()
{
	pid_t pid;

	pid = fork();

	if (pid < 0)
		exit(EXIT_FAILURE);

	if (pid > 0)
		exit(EXIT_SUCCESS);

	if (setsid() < 0)
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

void handler_sigchld(int sig)
{
	(void)sig;
	hay_hijos_terminados = 1;
}

static void revisar_hijos(void)
{
	int status;
	pid_t pid;
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		for (int i = 0; i < proceso_count; ++i)
		{
			if (procesos[i].pid == pid)
			{
				if (strcmp(procesos[i].state, "RUNNING") == 0)
				{
					strcpy(procesos[i].state, "ZOMBIE");
					syslog(LOG_NOTICE, "Proceso IID=%d terminó\n", procesos[i].iid);
				}
				break;
			}
		}
	}
}
