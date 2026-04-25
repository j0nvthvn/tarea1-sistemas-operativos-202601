#ifndef STRUCT_H
#define STRUCT_H

#include <sys/types.h>
#include <time.h>

typedef struct
{
    pid_t pid;
    char comando[256];
} Request;

typedef struct
{
    int iid;
    pid_t pid;
    char binary[256];
    char state[16];
    time_t start_time;
} Proceso;

#endif // STRUCT_H
