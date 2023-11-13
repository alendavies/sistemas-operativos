# sched

Lugar para respuestas en prosa, seguimientos con GDB y documentación del TP.

**PARTE UNO**
***CONTEXT_SWITCH***

Seguimiento mediante gdb del cambio de contexto.

Información de los registros en el kernel space, una linea antes de ejecutar iret:

![Alt text](Im%C3%A1genes_informe/image-1.png)

Como se puede ver, el $esp (stack pointer) tiene una posición alta de memoria.

Además los últimos bits del $cs (code segment) indican que se está en el ring 0.

Luego de ejecutar iret:

![Alt text](Im%C3%A1genes_informe/image.png)

El $esp (stack pointer) tiene una posición baja de memoria.

Los últimos bits del $cs (code segment) indican que se está en el ring 3.

**PARTE DOS**
***ROUND ROBIN***

En este apartado se implementa un scheduler bajo la política Round Robin.
Simplemente se busca el próximo environment RUNNABLE disponible y se ejecuta.
Si no hay ningún environment RUNNABLE y el environment actual está RUNNING, se sigue corriendo el actual.

Luego de la parte uno y dos implementadas, corren todos los tests.

**PARTE TRES**
***SCHEDULER CON PRIORIDADES***

Basándonos en las siguientes reglas básicas:

1. Si la prioridad de A es mayor que la de B, A se ejecuta y B no.
2. Si la prioridad de A es igual a la de B, A y B se ejecutan en Round-Robin.
3. Cuando un proceso entra en el sistema se pone en la de prioridad más alta.
4. Una vez que una tarea usa su asignación de tiempo (slice) de un nivel dado (independientemente de cuantas veces haya renunciado al uso de la CPU) su prioridad se reduce (baja un nivel).
5. Después de cierto periodo de tiempo S, se mueven todos los procesos a la cola con más prioridad.

Se desarrollo un Scheduler con prioridades. En nuestro caso no utilizamos colas, sino números que representan un nivel de prioridad.

La idea es sencilla: 

-Todos los procesos comienzan en la mejor prioridad.
-Se busca el próximo proceso RUNNABLE en la lista de prioridad que tenga la prioridad más baja
En caso de que todos tengan la misma prioridad es igual a la implementación del Round Robin.
-Si un proceso se ejecuto CYCLES veces (usó su asignación de tiempo (slice) una cierta cantidad de veces) su prioridad baja.
Esto es así para que no bajen tan rápido de prioridad y que la prioridad valga realmente.
-Para evitar el starving, cada cierta cantidad de ejecuciones generales (EXECUTIONS_TO_BOOST), se boostean todos los procesos a la mejor prioridad. De esta forma se garantiza que todos los procesos van a ser atendidos en algún momento.

En caso de no encontrar ninguno RUNNABLE, se ejecuta el que estaba RUNNING (si es que estaba RUNNING).