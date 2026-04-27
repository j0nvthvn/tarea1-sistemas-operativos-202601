# Toadd

## Descripción

El sistema consiste en dos programas en C: `toadd` y `toad-cli`.

`toadd` es un proceso daemon que actúa como systemd y corre en background, independiente del terminal desde el que fue iniciado. Su responsabilidad es ejecutar, monitorear y terminar otros procesos, manteniendo registro de sus estado durante toda su vida.

`toad-cli` es la herramienta de línea de comandos con la que el usuario interactúa con `toad`. 

Ambos programas se comunican entre sí mediante FIFOs (pipes con nombre).


## Compilación
```c
gcc -Wall -Wextra -std=c17 toadd.c -o toadd
gcc -Wall -Wextra -std=c17 toad-cli.c -o toad-cli
```

## Uso

### Iniciar el daemon
```c
./toad
```
### Comandos disponibles (`toad-cli`)

#### Ejecutar un binario
```c
./toad-cli start <bin_path>
```
#### Detener un proceso con SIGTERM
```c
./toad-cli stop <iid>
```
#### Listar los procesos administrados por `toadd`
```c
./toad-cli ps
```
#### Ver información detallada de un proceso
```c
./toad-cli status <iid>
```
#### Terminar un proceso y todos sus descendientes (SIGKILL)
```c
./toad-cli kill <iid>
```
#### Listar los procesos administrados por `toadd` en estado "ZOMBIE"
```c
./toad-cli zombie
```
