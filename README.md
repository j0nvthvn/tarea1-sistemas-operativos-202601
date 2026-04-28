# Toadd

## Descripción

El sistema consiste en dos programas en C: `toadd` y `toad-cli`.

`toadd` es un proceso daemon que actúa como systemd y corre en background, independiente del terminal desde el que fue iniciado. Su responsabilidad es ejecutar, monitorear y terminar otros procesos, manteniendo registro de sus estado durante toda su vida.

`toad-cli` es la herramienta de línea de comandos con la que el usuario interactúa con `toad`. 

Ambos programas se comunican entre sí mediante FIFOs (pipes con nombre).


## Compilación
```bash
gcc -Wall -Wextra -std=c17 toadd.c -o toadd
gcc -Wall -Wextra -std=c17 toad-cli.c -o toad-cli
```

## Uso

### Iniciar el daemon
```bash
./toad
```
### Comandos disponibles (`toad-cli`)

#### Ejecutar un binario
```bash
./toad-cli start <bin_path>
```
#### Detener un proceso con SIGTERM
```bash
./toad-cli stop <iid>
```
#### Listar los procesos administrados por `toadd`
```bash
./toad-cli ps
```
#### Ver información detallada de un proceso
```bash
./toad-cli status <iid>
```
#### Terminar un proceso y todos sus descendientes (SIGKILL)
```bash
./toad-cli kill <iid>
```
#### Listar los procesos administrados por `toadd` en estado "ZOMBIE"
```bash
./toad-cli zombie
```

## Arquitectura

### Comunicación entre procesos
- **FIFO principal** `/tmp/toad.fifo`: `toadd` está constantemente leyendo comandos enviados por `toad-cli`.
- **FIFO de respuesta** `/tmp/toad-cli-{PID}.fifo`: cada instancia de `toad-cli`crea su propio FIFO de respuesta usando su PID como identificador único. Esto permite que múltiples instancias de `toad-cli`corran simultáneamente, cada una recibiendo su propia respuesta.

### Daemonización
`toadd` utiliza el patrón de doble fork para convertirse en daemon. Esto garantiza que el proceso quede completamente independiente del terminal desde el que fue iniciado, sin depender de ningún proceso padre.

### Detección de procesos terminados
 
Cuando un proceso hijo termina, el kernel envía la señal `SIGCHLD` a `toadd`. El handler `handler_sigchld` es minimalista: solo activa una bandera (`sig_atomic_t`) para indicar que hay hijos por revisar. El trabajo real ocurre en `revisar_hijos()`, llamada desde el loop principal. Esto evita comportamientos erráticos dentro del handler de señal.

## Estados de procesos

| Estado | Descripción |
| ------ | ----------- |
| `RUNNING` | El proceso está en ejecución |
| `STOPPED` | El proceso recibió una señal de término (`SIGTERM` o `SIGKILL`) |
| `ZOMBIE` | El proceso terminó pero `toadd` aún no recogió su estado de salida |

> **Nota:** En la implementación actual, cuando `toadd` detecta que un hijo
> terminó inesperadamente, llama `waitpid` para evitar zombis reales en el
> kernel, pero mantiene el estado `ZOMBIE` en su tabla interna para cumplir
> con la interfaz del sistema.

## Logs

`toadd` registra eventos importantes en el sistema de logs del sistema.
Para monitorear los logs en tiempo real:

```bash
journalctl -t toad -f
```
Para ver todos los logs desde que arrancó:

```bash
journalctl -t toad
```
