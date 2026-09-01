# Laboratorio 3: procesos y comunicación entre procesos en Linux

Esta carpeta contiene una solución completa y explicada para los ejercicios que aparecen en la guía. Los programas deben compilarse y ejecutarse en **Linux** (Ubuntu, Debian, Linux Mint, una máquina virtual o WSL), porque las capturas solicitadas deben mostrar ese entorno.

## 1. Preparación

Instale las herramientas necesarias:

```bash
sudo apt update
sudo apt install build-essential psmisc
```

`build-essential` instala `gcc` y `make`; `psmisc` instala `pstree`.

Para compilar los programas usando el `Makefile` de la raíz del proyecto:

- **Desde la carpeta `lab3`:**
  ```bash
  make -C ..
  ```
- **Desde la raíz del repositorio:**
  ```bash
  make
  ```

También puede compilar cada archivo por separado, por ejemplo:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic ejercicio1.c -o ejercicio1
```

## 2. Ejercicio 1: dos hijos, `exec` y `wait`

Ejecute:

```bash
./ejercicio1
```

Cómo funciona:

1. El padre llama a `fork()` y crea el primer hijo.
2. En el hijo, `fork()` devuelve `0`; por eso esa rama imprime `getpid()` y `getppid()` y termina con `_exit()`.
3. Solo el padre continúa hasta el segundo `fork()`. Así se crea exactamente un segundo hijo y no hijos adicionales por accidente.
4. El segundo hijo llama a `execlp("ls", "ls", "-l", NULL)`. Si tiene éxito, su código es sustituido por el programa `ls`; por eso no regresa a la siguiente línea.
5. El padre llama dos veces a `wait()` y recoge el estado de los dos hijos. El orden de terminación puede cambiar en cada ejecución: eso es normal en procesos concurrentes.

Captura requerida: incluya en una sola imagen el comando de compilación, `./ejercicio1`, los PID, la salida de `ls -l` y el mensaje final del padre.

## 3. Ejercicio 3: Padre → Hijo → Nieto

En la terminal 1 ejecute:

```bash
./ejercicio3
```

El programa imprimirá un comando parecido a:

```text
Durante 15 segundos ejecute en otra terminal: pstree -p 12345
```

Antes de que pasen los 15 segundos, abra la terminal 2 y use el PID real mostrado:

```bash
pstree -p 12345
```

Alternativa si no tiene `pstree`:

```bash
ps -f --forest
```

Cómo funciona: el Padre crea al Hijo; únicamente el Hijo ejecuta el segundo `fork()` y crea al Nieto. Cada uno imprime su PID y su PPID. Los tres duermen 15 segundos, manteniendo viva la jerarquía para verla. Después, Hijo y Padre usan `waitpid()` para recoger a sus descendientes y evitar procesos zombis.

Capturas requeridas:

1. Terminal 1 con los mensajes y PID de Padre, Hijo y Nieto.
2. Terminal 2 con `pstree -p PID_DEL_PADRE` mostrando los tres niveles.

## 4. Propuesta 1: `servidor.c` ↔ `cliente.c`

El par realiza 20 rondas: el servidor envía 20 mensajes y el cliente responde con otros 20 (40 transferencias en total). Los mensajes pueden contener espacios.

Terminal 1:

```bash
./servidor
```

Terminal 2:

```bash
./cliente
```

Escriba una conversación en orden: primero habla el servidor, después responde el cliente. Tras la ronda 20, el servidor elimina la cola. Para terminar antes, escriba exactamente `salir`.

Detalles importantes:

- Ambos procesos usan la misma llave `34856`, que identifica la cola.
- Los mensajes de tipo `1` viajan hacia el cliente y los de tipo `2` hacia el servidor. Separar los tipos evita que un proceso reciba su propio mensaje.
- `msgsnd()` envía y `msgrcv()` recibe. Sin `IPC_NOWAIT`, `msgrcv()` queda esperando hasta que llegue el mensaje correcto.
- El servidor crea la cola con `IPC_CREAT | IPC_EXCL`; el cliente solo abre la cola existente.
- El servidor es el dueño de la limpieza y usa `msgctl(..., IPC_RMID, ...)` al finalizar.

Si una ejecución anterior fue interrumpida y aparece “Ya existe la cola”, limpie esa llave y vuelva a iniciar:

```bash
ipcrm -Q 34856
```

Tome suficientes capturas para que se vean las rondas iniciales, intermedias y finales en las dos terminales. Procure que los mismos números de ronda sean visibles a ambos lados.

## 5. Propuesta 2: `emisor.c` ↔ `receptor.c`

Esta pareja también es bidireccional y realiza 20 rondas. Usa una llave distinta (`34857`) para no mezclarse con la propuesta 1.

Terminal 1:

```bash
./emisor
```

Terminal 2:

```bash
./receptor
```

Primero escribe el Emisor; el Receptor recibe y contesta. Los tipos `1` y `2` separan las dos direcciones. Para cancelar una cola abandonada:

```bash
ipcrm -Q 34857
```

Las capturas deben demostrar que el Emisor envía, el Receptor recibe, el Receptor contesta y el Emisor recibe la respuesta. Incluya también rondas cercanas a la número 20.

## 6. Qué conviene explicar en el informe

- `fork()` duplica un proceso; el valor devuelto indica si se está en el padre o en el hijo.
- `getpid()` obtiene el identificador propio y `getppid()` el del padre.
- `exec` no crea otro proceso: sustituye el programa que ejecuta el proceso actual.
- `wait()`/`waitpid()` sincronizan al padre y recogen procesos terminados.
- Una cola System V permite comunicar procesos independientes mediante una llave y un identificador de cola.
- El primer campo de la estructura debe ser `long`; es el tipo que `msgrcv()` usa para seleccionar la dirección correcta.
- A `msgsnd()` se le pasa el tamaño del texto, no el tamaño del campo `long tipo`.
- `IPC_RMID` elimina la cola para que no quede almacenada en el sistema después de la práctica.

## 7. Lista de entrega

- Código fuente: `ejercicio1.c`, `ejercicio3.c`, `servidor.c`, `cliente.c`, `emisor.c` y `receptor.c`.
- Captura de compilación sin errores (`make` o comandos `gcc`).
- Captura completa del ejercicio 1.
- Dos capturas del ejercicio 3: ejecución y árbol de procesos.
- Capturas de ambas terminales de la propuesta 1, mostrando 20 rondas.
- Capturas de ambas terminales de la propuesta 2, mostrando 20 rondas.
- Breve explicación de las funciones principales y de por qué hay dos tipos de mensaje.

> Nota: la guía salta de “Ejercicio 1” a “Ejercicio 3”; este material conserva esa numeración y no inventa un Ejercicio 2.
