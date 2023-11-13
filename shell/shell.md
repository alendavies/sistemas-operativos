# shell

### Búsqueda en $PATH

#### Diferencias entre `execve(2)` y wrappers `exec(3)`.

En primer lugar, `execve` es la system call nativa del kernel de linux. Recibe por parámetro el pathname del binario a ejecutar,un arreglo con el comando y los argumentos, y un arreglo con las variables de entorno.

Por otro lado, las funciones provistas por la librería estándar, correspondientes a la familia de wrappers `exec` (que incluye varias funciones, como `execv`, `execvp`, etc...), proveen una funcionalidad similar a `execve`, pero proporcionando interfaces diferentes. Algunas reciben el pathname, o el nombre del binario a ejecutar y por dentro utilizan la variable de entorno `$PATH` para determinar su ubicación, y las demás requieren pasarles las variables de entorno. Algunas reciben el comando y argumentos en un array, y otras reciben un argumento por parámetro.

Además, manejan los errores de `execve` de forma más elaborada, por ejemplo, continuando la búsqueda de un binario a ejecutar si `execve` lanzó el error `EACCESS`.

En resumen, las funciones de `exec` difieren de la syscall nativa en que proporcionan una variedad de interfaces y manejo de errores más elaborado.

#### Falla en ejecución de `exec(3)`.

Existen varios motivos por los cuales pueden fallar las funciones `exec(3)`, y la mayoría de los errores corresponden a errores de `execve`. Algunos de los más comunes son: permiso denegado, archivo inexistente y error de entrada/salida. Al encontrarse un error, `exec` retorna `-1` y se setea `errno` con el código del error.

Respecto al comportamiento de la shell en caso de error de `exec`, se imprime por pantalla un string con el mensaje de error, y se procede a ejecutar `exit(-1)`, es decir, terminación abrupta del proceso de forma anormal, volviendo al prompt de la shell.

---

### Procesos en segundo plano

#### Mecanismo de implementación

Para implementar procesos en segundo plano, al detectarse un comando finalizado en "`&`", se realiza la ejecución normal del comando, pero en lugar de utilizar `wait` esperando la finalización del programa para volver al prompt del shell, se llama a `waitpid` con el parámetro `options` seteado a `WNOHANG`, lo cual permite continuar ejecutando procesos en primer plano mientras el programa en segundo no haya finalizado. Es necesario llamar oportunísticamente este `waitpid` para terminar el proceso correctamente una vez que finalice.

---

### Flujo estándar

#### Investigar el significado de 2>&1, explicar cómo funciona su forma general

El operador `>` permite redireccionar la salida estandar de un comando. Por ejemplo si se ejecuta:
`comando >out.txt` se imprimirá en el archivo out.txt el resultado del comando.
Si se agrega un file descriptor antes de `>` por ejemplo
`comando 1>out.txt` se está indicando que se quiere redirigir lo apuntado por el file descriptor ¨1¨ al archivo out.txt (dará el mismo resultado que el comando anterior).
Si se desea redirigir la salida del error estandar a un archivo se escribe `comando 2>out.txt`. Si se quiere redirigir la salida estandar y el error estandar al mismo archivo se ejecuta: `comando >out.txt 2>&1`. De esta manera se redirige primero, lo apuntado por el file descriptor "1" al archivo out.txt y luego la salida de error al file descriptor "1" (que está apuntando a out.txt). Es importante agregar el simbolo`&` para que no se interpreten el ¨2¨ y el ¨1¨ como números sino como file descriptors.

**En nuestra shell implementada:** <br>
En el caso del ejemplo se escribe la salida estandar del comando y la salida del error en el archivo out.txt:

```bash
$ ls -C /home /noexiste >out.txt 2>&1`
$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
demo
```

Al invertir el orden (`ls -C /home /noexiste 2>&1 >out.txt`) se produce el mismo resultado.

**En bash(1)** <br>

```bash
$ ls -C /home /noexiste >out.txt 2>&1`
$ cat out.txt
ls: cannot access '/noexiste': No such file or directory
/home:
demo
```

Se produce el mismo resultado que en nuestra shell: se redirigen tanto la salida estandar como el error al archivo.

Al invertir el orden de las operaciones:

```bash
$ ls -C /home /noexiste 2>&1 >out.txt
ls: cannot access '/noexiste': No such file or directory
$ cat out.txt
/home:
demo
```

Lo que ocurre en este caso es que, primero se escribe el error por consola (ya que el file descriptor "1" todavía apunta a la consola) y en segundo lugar se hace la redireccion de la salida estandar al archivo out.txt, escribiendo asi el resultado del comando en él.

---

### Tuberías múltiples

#### Responder: Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.

Para saber cual es el exit code en bash hay que ejecutar el comando `echo $?` que mostrara el exit code del ultimo comando ejecutado.<br>
Si todos los comandos de un pipe se ejcutan sin errores el exit code es 0. Esto ocurre en nuestra shell implementada y en bash.<br>
Si en el pipe alguno de los comandos falla ocurre lo siguiente: <br>
**En Bash:**
**si falla el primer comando:**

```bash
$ ls_falla -l | grep Doc | echo hola
hola
ls_falla: command not found
$ echo $?
0
```

Se ve en ejemplo que devuelve exit code 0 y ademas parece ejecutarse al menos el tercer comando.<br>
**si falla el segundo comando:**

```bash
$ ls -l | grep_falla Doc | echo hola
hola
grep_falla: command not found
$ echo $?
0
```

Nuevamente devuelve exit code 0, y se ejecuta el tecer comando.<br>
**si falla el tercer comando:**

```bash
$ ls -l | grep Doc | echo_falla hola
echo_falla: command not found
$ echo $?
127
```

En este caso el exit code es distinto de 0 (en particular 127) que es el exit code del ultimo comando ejecutado.<br>
Podemos concluir que bash devuelve el exit code del ultimo comando ejecutado en un pipe.<br>
**En nuestra shell:**
**si falla el primer comando:**

```bash
$ ls_falla -l | grep Doc | echo hola
execvp: No such file or directory
hola
$ echo $?
0
```

Muestra el exit code del ultimo ejecutado.<br>
**si falla el segundo comando:**

```bash
$ ls -l | grep_falla Doc | echo hola
execvp: No such file or directory
hola
 $ echo $?
0
```

Muestra el exit code del ultimo ejecutaddo.<br>
**si falla el tercer comando:**

```bash
$ ls -l | grep Doc | echo_falla hola
execvp: No such file or directory
$ echo $?
0
```

En nuestra implementación, cuando el proceso padre espera a los hijos izquierdo y derecho, ejecuta exit(0). Al ser este el último proceso, echo &? siempre devuelve 0.
De esta forma no tenemos manera de saber que exit code tiene el último comando del pipe. Sólo es posible ver "execvp: No such file or directory" ya que es el error printeado en exec_cmd luego de llamar execvp.
---

### Variables de entorno temporarias

#### ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?

Es necesario hacerlo luego de la llamada a fork(2) porque si se hace antes de la llamada a fork(2) se estaría modificando la variable de entorno del proceso padre, cuando lo que se quiere es modificar la variable de entorno del proceso hijo.
Esto hace que la variable sea temporaria y le pertenezca unicamente al proceso hijo, durante su ejecución.

#### Utilizar exec(3) en vez de setenv(3)

**¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.**
El comportamiento resultante no es el mismo ya que al pasarle un arreglo con las variables de entorno a exec, estas especificando que el proceso utilice esas variables en vez de las que se encuentran en la variable environ.
El unico caso en el que el comportamiento es el mismo es cuando se pasa NULL o environ como tercer parametro, ya que en ese caso se utiliza la variable environ. En este caso se podria crear un array que contenga las variables de environ, agregando las nuevas que se quieran utilizar.

**Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.**
Se podría crear un arrya de variables de entorno, que contenga las variables que se encuentran en environ y las nuevas variables que se quieran utilizar. Luego se le pasa este array a la funcion de exec.

---

### Pseudo-variables

#### Investigar al menos otras tres variables mágicas estándar, y describir su propósito.

`$_`: Almacena el último argumento del comando anterior. <br>
`$!`: Contiene el PID del último proceso en segundo plano iniciado. <br>
`$-`: Almacena las opciones de configuración actuales del shell. <br>
`$@ y $*`: Representan todos los argumentos de línea de comandos pasados a un script o función de shell. A diferencia de $\*, $@ conserva la separación entre argumentos. <br>

## **Incluir un ejemplo de su uso en bash (u otra terminal similar).**

```bash
$ cd $HOME
$ echo $_
/home/usuario
```

```bash
$ sleep 10 &
[1] 1234
$ echo $!
1234
```

```bash
$ echo $-
himBHs
```

```bash
$ ./ejecutable uno dos tres
$ echo $@
uno dos tres
```

---

### Comandos built-in
¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)

Primero, fueron probados ambos.

pwd funciona a pesar de no estar implementado el built-in.
cd no funciona sin el built-in.

Según el enunciado de este apartado: "Es evidente que si cd no se realizara en el mismo proceso donde la shell se está ejecutando, no tendría el efecto deseado". Esto tiene sentido, ya que si se ejecutara cd desde un proceso hijo, se cambiaria el directorio para este, pero no para la shell (el proceso principal). Un proceso hijo, no puede cambiar el directorio del padre. Además, cd no existe como un comando ejecutable (binario), por lo que no funciona con execvp.

Por otro lado, "pwd" si funciona, porque existe el comando ejecutable (binario) y execvp puede encontrarlo y ejecutarlo.

Es un built-in porque es mucho más rápido y eficiente que llamar a otro proceso y ejecutar el binario. En la mayoría de las shells, se puede encontrar como built-in y como ejecutable.

---

### Segundo plano avanzado

#### Explicar detalladamente el mecanismo completo utilizado.

Como base del mecanismo utilizado, está el uso de la señal SIGCHLD, que emite un proceso cuando termina. Para capturar esta señal, se utiliza la syscall `sigaction`, que permite registrar una acción a realizar al recibir una determinada signal. Esta syscall recibe como parámetros la `signum` correspondiente a la señal que se quiere capturar, en nuestro caso, `SIGCHLD`. También recibe un struct del tipo `sigaction`, que contiene la función `handler` (algo así como una callback), y otras configuraciones. Se construye este dato con la función que utilizaremos para manejar la recepción de la señal, y se lo pasa a `sigaction`.

```c
struct sigaction sa = {
    .sa_handler = sigchld_handler
};

if (sigaction(SIGCHLD, &sa, NULL) == -1) {
	perror("sigaction");
	exit(1);
}
```

En esta función es donde se espera a los procesos en segundo plano para liberarlos. Aquí es donde surge un inconveniente, y es que cualquier proceso hijo que termine emitirá la señal, cuando se quiere manejar únicamente las de los procesos en segundo plano, ya que los procesos sincrónicos tienen su propio `wait`. Para ello, es necesario crear un process groupd ID que agrupe a los procesos en segundo plano.

Para poder liberar solamente los procesos en segundo plano, utilizamos `waitpid` pero en lugar de pasarle -1 para que espere a todos los procesos hijos, le pasamos como argumento un 0. Esto hace que espere solo a los procesos hijos que forman parte del mismo process group al que pertenece el proceso actual.
Para crear un process group que solo contenga a los procesos en segundo plano, se utiliza la función `setpgid` con la cual le seteamos un process group ID a los procesos en segundo plano creados en el fork. El process group ID que utilizamos, es el PID del proceso padre (que se puede obtener con `getppid`), y lo seteamos como process group ID mediante `getpgid`, pasandole el PID del padre como parámetro.
Se hizo de esta manera, ya que este proceso padre es el que se encarga de esperar a todos los procesos en segundo plano. De esta forma, todos los procesos en segundo plano que se creen, pertenecerán al mismo process group ID y podrán ser esperados correctamente solucionando el inconveniente mencionado anteriormente.

```c
setpgid(0, getpgid(getppid()));
```

Por último, otro inconveniente que puede surgir es que el stack asignado al proceso de la shell se agote (stack overflow), lo cual pone en peligro que se manejen correctamente las señales. Es muy importante que el funcionamiento de las señales sea predecible y correcto, por lo que se utiliza un mecanismo para asignarles un stack propio, de esta forma aunque el proceso principal se quede sin memoria, la función `handler` tendrá su propio stack y las signals serán manejadas correctamente. Esto se logra utilizando la syscall `sigaltstack`. Se construye un `alt_stack` del tipo `stack_t`, pasando la dirección y tamaño del stack para las señales, y se lo pasa a `sigalstack`. Así queda se informa al sistema de utilizar el stack alternativo para el manejo de señales.

```c
stack_t alt_stack;
ss.ss_sp = malloc(SIGSTKSZ);

if (ss.ss_sp == NULL) {
    perror("malloc");
    exit(1);
}

ss.ss_size = SIGSTKSZ;
ss.ss_flags = 0;

if (sigaltstack(&ss, NULL) == -1) {
    perror("sigaltstack");
    exit(1);
}
```

#### ¿Por qué es necesario el uso de señales?

Es necesario el uso de señales para manejar interrupciones y eventos sin la necesidad de chequear constantemente que se cumplan cieras condiciones. Además, registrar señales garantiza que se ejecuten ciertas acciones cuando se produzcan ciertos eventos, como por ejemplo, la finalización de un proceso en segundo plano o la interrupcion por parte del usuario de un proceso con `Ctrl+C`.
Si bien es posible evitar el uso de señales mediante otros mecanismos, como por ejemplo, el uso de `waitpid` con el flag `WNOHANG`, el uso de señales es más eficiente y permite un manejo más sencillo de los eventos.

---

### Historial

---
