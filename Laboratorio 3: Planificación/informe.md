Informe
=
Primera parte del laboratorio. Leer codigo para entender como funciona el planificador

## **Contenido**:
1. [Primera Parte](#id1)
    1. [Politica de planificacion](#id1a)
    - ¿Qué política de planificación utiliza xv6-riscv para elegir el próximo proceso a ejecutarse? 
    - ¿Cuánto dura un quantum en xv6-riscv?
    2. [Los cambios de contexto](#id1b)
    - ¿Cuánto dura un cambio de contexto en xv6-riscv?
    - ¿El cambio de contexto consume tiempo de un quantum?
    3. [Los Procesos](#id1c)
    - ¿Hay alguna forma de que a un proceso se le asigne menos tiempo? 
    - ¿Cúales son los estados en los que un proceso pueden permanecer en xv6-riscv y que los hace cambiar de estado?
2. [Segunda Parte](#id2)<br>
    Analisis por casos de la ejecucion de los programas iobench y cpubench con politica Round Robin <br>
    - [iobench solo](#id2a)
    - [cpubench solo](#id2b)
    - [iobench & cpubench](#id2c)
    - [cpubench &; iobench &](#id2d)
    - [cpubench & cpubench & iobench](#id2e)
3. [Tercera Parte](#id3)<br>
    Analisis por casos de la ejecucion de los programas iobench y cpubench con politica MLFQ con Priority boost
    - [iobench & cpubench](#id3a)
    - [cpubench & cpubench & iobench](#id3b)



### Politica de planificacion <div id='id1a'> <div id='id1'> 

- ¿Qué política de planificación utiliza xv6-riscv para elegir el próximo proceso a ejecutarse? 

xv6 utiliza interrupciones temporizadas para poder intercambiar procesos o hilos por cpu. Estas interrupciones provienen del clock hardware para interrumpir cada cpu cada cierto tiempo (quantum).   
RISC-V requiere que las interrupciones del temporizador se tomen en modo máquina, de tal forma que el sistema operativo maneja las interrupciones fuera del entorno de trap mechanism.
Esta politica se denominada Round Robin. 

- ¿Cuánto dura un quantum en xv6-riscv?

En el archivo start.c se programa parte del CLINT hardware (core-local interruptor).
El siguiente framento de codigo indica la cantidad de ciclos de reloj que habra hasta la proxima interrupcion

`// ask the CLINT for a timer interrupt.`

`int interval = 1000000; // cycles; about 1/10th second in qemu.`

Luego los macros definidos en memlayout.h daran la proxima interrupcion (el id es del hart o core al cual pertenece)

`*(uint64*)CLINT_MTIMECMP(id) = *(uint64*)CLINT_MTIME + interval;`

Los datos se guardaran en un tipo de dato scratch y finalmente, start establece mtvec en timervec y habilita las interrupciones del temporizador.

### Los cambios de contexto <div id='id1b'>

- ¿Cuánto dura un cambio de contexto en xv6-riscv?

El tiempo que tarda un cambio de contexto puede variar dependiendo de varios factores, como la velocidad del procesador, la carga del sistema o la temperatura del Core. Sin embargo, en general, el cambio de contexto en xv6-riscv no debería ser significativo. 
Durante el cambio de contexto se efectuan un numero de instrucciones derivados de las funciones que se ejecutan en el kernel.

El orden de una interrupcion siempre empieza con el hardware, el archivo assembly kernelvec.S llama a las funciones en trap.c `void kerneltrap()` y luego para identificar que tipo de interrupciòn fue la que se efectuo `int devintr()`. 

Si la interrupcion fue una de reloj, se llama a void `yield(void)`

A partir de aqui empieza el cambio de contexto como tal.
Primero, yield llama a `void sched(void)` funcion que mediante la rutina 

`swtch(&p->context, &mycpu()->context);` 

para pasarle el poder del cpu al scheduler (el cual funciona como un hilo especial por cada cpu).

En `void scheduler(void)` se elige un nuevo proceso para ser ejecutado, se llama nuevamente a la funcion swtch() y finaliza el cambio de contexto. 

Para reanudar la ejecuciòn normal del sistema se vuelve a la funciòn que llamo a yield() en kernel/trap.c 

`// the yield() may have caused some traps to occur,`<br>
`// so restore trap registers for use by kernelvec.S's sepc instruction` <br>
`w_sepc(sepc);` <br>
`w_sstatus(sstatus);` <br> 

Estas ultimas rutinas permiten que se retome el punto donde ocurrio la interrupcion con coherencia.

El tiempo total del cambio de contexto seria el que tardaria en ejecutar todas las instrucciones de esta traza de funciones. 

- ¿El cambio de contexto consume tiempo de un quantum?

Si, el cambio de contexto consume tiempo de quantum en cada proceso. 
Calcular el tiempo de un cambio de contexto representaria una busqueda sobre la velocidad del procesador que emula qemu. 
Ademas el unico momento donde no se computaria el tiempo de quantum serian aquellas donde las interrupciones no esten activadas, y esto solo ocurre por dos instrucciones en my_proc. 


### Los procesos <div id='id1c'>

- ¿Hay alguna forma de que a un proceso se le asigne menos tiempo? 

No, el quantum esta determinado por el hardware y no es posible asignarle una cantidad de tiempo distinta a los proceso (al menos en este sistema operativo con estas politicas).
La syscall Uptime nos permite obtener el tiempo desde que se inicio el sistema operativo. Con esta syscall podemos calcular el tiempo que lleva un proceso en ejecucion, ya que definimos en la struct proc el *startTime*, es decir, el tiempo en el que el proceso nacio y el *lastexe*, el cual nos dice cuando fue que se ejecuto el proceso por ultima vez (o mas precisamente cuando fue elegido por el scheduler por ultima vez). Restando estos dos parametros del proceso podemos calcular cuanto tiempo lleva ejecutandose. Esta medida es muy similar a la metrica Turnaround time. 
Con esta informacion una politica como MLFQ puede decidir si el proceso debe seguir ejecutandose o prescindir de èl. 
Lo que ocurre cuando calculamos el tiempo de ejecucion de un proceso y este es menor al quantum previamente establecido, es que el mismo proceso entrega la cpu antes de que finalice su tiempo correspondiente. La funcion que permite esto es `yield(void)`. 

- ¿Cúales son los estados en los que un proceso pueden permanecer en xv6-riscv y que los hace cambiar de estado?

    - UNUSED: El proceso no esta siendo utilizado.
    - USED: El proceso esta siendo creado.
    - SLEEPING: El proceso esta dormido.
    - RUNNABLE: El proceso esta listo para ser ejecutado.
    - RUNNING: El proceso esta siendo ejecutado.
    - ZOMBIE: El proceso ha terminado su ejecucion y esta esperando que su padre lo limpie.



### CPUbench solo  <div id='id2a'>

1. A modo de ejemplificacion, una unica ejecusion de cpubench daria un output similar a este

Se ejecuto un ctrl-p para ver el proceso 

    27: 2391 MFLOP100T
    27: 2415 MFLOP100T
    27: 2368 MFLOP100T
    27: 2391 MFLOP100T
    27: 2368 MFLOP100T
    27: 2391 MFLOP100T
    27: 2368 MFLOP100T
    27: 2391 MFLOP100T
    27: 2391 MFLOP100T
    27: 2391 MFLOP100T
    27: 2391 MFLOP100T
    27: 2391 MFLOP100T
    27: 2391 MFLOP100T


    [Name: init, Pid: 1, State: sleep 
    Prio: 2, Counter: 34, timeexe: 8310, lastexe: 8310] 


    [Name: sh, Pid: 2, State: sleep 
    Prio: 2, Counter: 39, timeexe: 8846, lastexe: 8847] 


    [Name: cpubench, Pid: 27, State: run   
    Prio: 2, Counter: 1402, timeexe: 1401, lastexe: 10248] 


    27: 2391 MFLOP100T
    27: 2368 MFLOP100T
    27: 2415 MFLOP100T
    27: 2391 MFLOP100T
    27: 2368 MFLOP100T
    27: 2368 MFLOP100T
    Termino cpubench 27: total ops 2952790016u --> 
    prioridad: 2
    veces elegido: 2105
    ultima ejecuciòn: 10951 

2. Ejecusiones paralelas del mismo proceso cpubench con el quantum inicial

Bajo la hipotesis 
~~~
En un escenario de cpubench todos los procesos deberían recibir la misma cantidad de quantums y tener el mismo número de MFLOPs totales. 
~~~
Ejecutar 
`$ cpubench &; cpubench &` 
deberia devolver en la llamada a `pstat(pid)` el mismo valor de *ultima ejecucion* y uno muy similar en *veces elegido* ya que la forma de leer ambos datos estan en la funcion `scheduler(void)`. La cual va del primer cpubench elegido (mediante `swtch(&c->context, &p->context)`), luego ocurre una interrupcion de reloj y al ser un proceso de cpu, ocupa todo su quantum hasta que `yield(void)` trasnfiere el control nuevamente al punto donde quedo `scheduler(void)`, para que en proximas iteraciones encuentre el otro cpubench y lo ejecute. 
Ambos procesos son elegidos alternadamente ya que no hay otros en estado RUNNABLE.  

Vemos el output de `$ cpubench &; cpubench &`

    4: 559 MFLOP100T
    6: 554 MFLOP100T
    4: 559 MFLOP100T
    6: 554 MFLOP100T
    4: 688 MFLOP100T
    6: 700 MFLOP100T
    4: 1172 MFLOP100T
    6: 1161 MFLOP100T
    6: 1172 MFLOP100T
    .
    .
    .
    6: 1195 MFLOP100T
    4: 1184 MFLOP100T
    6: 1172 MFLOP100T
    4: 1184 MFLOP100T
    Termino cpubench 4: total ops 4160749568u --> 
    prioridad: 0
    veces elegido: 1054
    ultima ejecuciòn: 2676 
    Termino cpubench 6: total ops 4160749568u --> 
    prioridad: 2
    veces elegido: 1058
    ultima ejecuciòn: 2676 

3. Disminuir el quantum 

~~~
En escenarios de todos cpubench: al achicar el quantum el output total de trabajo debería ser menor o igual.
~~~

Pasando de un quantum seteado en 1.000.000 a uno 10 veces mas chico, es decir 
`int interval = 100.000;`

    11: 1172 MFLOP100T
    9: 1172 MFLOP100T
    11: 1172 MFLOP100T
    9: 1161 MFLOP100T
    .
    .
    .
    11: 1172 MFLOP100T
    9: 1184 MFLOP100T
    11: 1184 MFLOP100T
    9: 1172 MFLOP100T
    Termino cpubench 11: total ops 1476395008u --> 
    prioridad: 0
    veces elegido: 10560
    ultima ejecuciòn: 56745 
    Termino cpubench 9: total ops 1476395008u --> 
    prioridad: 0
    veces elegido: 10573
    ultima ejecuciòn: 56757 

Los quantums mas cortos implican menos tiempo de ejecucion para cada proceso y mas tiempo para las intrucciones de cambio de contexto, las cuales pueden acumularse y tomar gran parte del tiempo de un proceso. 
Por esto, la cifra de operaciones totales que se contabilizan al finalizar el programa, se mantiene similar o menor. 


### I/Obench solo <div id='id2b'>
A modo de ejemplificacion, una unica ejecusion de cpubench daria un output similar a este. <br>
A comparacion de los procesos cpubench, iobench incluye interrupciones adicionales a las de reloj, esto genera mayor gasto de quantum en instrucciones extra y el tiempo de respuesta de los dispositivos de lectura y escritura. Podemos suponer que la cantidad de operaciones que realice sera considerablemente menor a la de cpubench. 

Se ejecuto un ctrl-p para ver el proceso.

    [Name: init, Pid: 1, State: sleep 
    Prio: 2, Counter: 24, timeexe: 1, lastexe: 1] 


    [Name: sh, Pid: 2, State: sleep 
    Prio: 2, Counter: 15, timeexe: 97, lastexe: 98] 


    [Name: iobench, Pid: 4, State: run   
    Prio: 2, Counter: 14890, timeexe: 43, lastexe: 141] 


					4: 11520 OPW100T, 11520 OPR100T
					4: 11712 OPW100T, 11712 OPR100T
					4: 11712 OPW100T, 11712 OPR100T
					4: 11712 OPW100T, 11712 OPR100T
					4: 11712 OPW100T, 11712 OPR100T
    				4: 11648 OPW100T, 11648 OPR100T
                    4: 11712 OPW100T, 11712 OPR100T
					4: 11712 OPW100T, 11712 OPR100T
					4: 11776 OPW100T, 11776 OPR100T
					4: 11712 OPW100T, 11712 OPR100T
					4: 11392 OPW100T, 11392 OPR100T
					4: 11584 OPW100T, 11584 OPR100T
					4: 11520 OPW100T, 11520 OPR100T
					4: 11456 OPW100T, 11456 OPR100T
					4: 11328 OPW100T, 11328 OPR100T
					4: 11520 OPW100T, 11520 OPR100T
					4: 11648 OPW100T, 11648 OPR100T
					4: 11520 OPW100T, 11520 OPR100T
					4: 11584 OPW100T, 11584 OPR100T
    Termino iobench 4: total ops 440960u -->	
    prioridad: 2
    veces elegido: 726598
    ultima ejecuciòn: 2195

### iobench &; cpubench & <div id='id2c'>

1. A modo de ejemplificacion, una ejecucion con el quantum original daria un output similar a este

Se ejecuto un ctrl-p para ver el proceso 

    7: 1128 MFLOP100T
    7: 1118 MFLOP100T
    7: 1118 MFLOP100T
                        6: 58 OPW100T, 58 OPR100T
    7: 1118 MFLOP100T
    7: 1118 MFLOP100T
                        6: 33 OPW100T, 33 OPR100T
    7: 1128 MFLOP100T
    7: 1098 MFLOP100T
                        6: 33 OPW100T, 33 OPR100T
    7: 1108 MFLOP100T
    7: 1108 MFLOP100T
                        6: 33 OPW100T, 33 OPR100T
    7: 1118 MFLOP100T
                        6: 33 OPW100T, 33 OPR100T
    7: 1128 MFLOP100T
    7: 1118 MFLOP100T
                        6: 33 OPW100T, 33 OPR100T
    7: 1118 MFLOP100T


    [Name: init, Pid: 1, State: sleep 
    Prio: 2, Counter: 26, timeexe: 119, lastexe: 119] 


    [Name: sh, Pid: 2, State: sleep 
    Prio: 2, Counter: 16, timeexe: 118, lastexe: 119] 


    [Name: cpubench, Pid: 7, State: run   
    Prio: 2, Counter: 1564, timeexe: 1558, lastexe: 1677] 


    [Name: iobench, Pid: 6, State: runble
    Prio: 2, Counter: 1564, timeexe: 1558, lastexe: 1677] 


    7: 1128 MFLOP100T
                        6: 33 OPW100T, 33 OPR100T
    7: 1108 MFLOP100T
    7: 1108 MFLOP100T
                        6: 33 OPW100T, 33 OPR100T
    7: 1118 MFLOP100T
                        6: 33 OPW100T, 33 OPR100T
    7: 1128 MFLOP100T

    Termino cpubench 7: total ops 268435456u --> 
    prioridad: 2
    veces elegido: 2121
    ultima ejecuciòn: 2234 

                        6: 33 OPW100T, 33 OPR100T

    Termino iobench 6: total ops 1408u -->	
    prioridad: 2
    veces elegido: 2236
    ultima ejecuciòn: 2235 

2. Analizamos el caso de un quantum diez veces mas pequeño
~~~
Con RR, un quantum menor debería favorecer a los procesos que hacen IO (en comparación al mismo escenario con quantum mas grande)
~~~
Ambos programas tienen resultados diferentes, pues uno depende de la escritura y la lectura del disco (genera interrupciones de entrada/salida) y el otro usa el cpu completamente.  
Se ve que el scheduler elige a ambos programas la misma cantidad de veces, pero la cantidad de operaciones es diferente para el programas I/O, mientras que cpubench se mantiene.

    7: 1078 MFLOP100T
                        5: 382 OPW100T, 382 OPR100T
    7: 1078 MFLOP100T
                        5: 333 OPW100T, 333 OPR100T
    7: 1068 MFLOP100T
                        5: 336 OPW100T, 336 OPR100T
    7: 1088 MFLOP100T
                        5: 333 OPW100T, 333 OPR100T
    .
    .
    .

    7: 1088 MFLOP100T
                        5: 336 OPW100T, 336 OPR100T
    7: 1078 MFLOP100T
                        5: 333 OPW100T, 333 OPR100T
    7: 1088 MFLOP100T
                        5: 336 OPW100T, 336 OPR100T
    7: 1078 MFLOP100T
                        5: 336 OPW100T, 336 OPR100T
    Termino iobench 5: total ops 13184u -->	
    prioridad: 0
    veces elegido: 21037
    ultima ejecuciòn: 23459 
    Termino cpubench 7: total ops 3355443200u --> 
    prioridad: 2
    veces elegido: 21113
    ultima ejecuciòn: 23535 

La cantidad de operaciones no es similar a la cantidad de veces que se eligio, en cpubench tiene sentido porque las operaciones la superan ¿Que sucede con iobench?

Acortar el quantum le da mayor posibilidades a los programas que ocupan poco tiempo de control sobre la cpu (como iobench) de ser elegidos nuevamente por scheduler cuando vuelvan a estar RUNNEABLES. 

Las lecturas y escrituras son mas lentas que los calculos, y dependen de elementos externos del procesador, por lo que agregar interrupciones va a aumentar la posibilidad de ser elegido. Aunque van a seguir siendo mucho menos que las operaciones total de cpubench. 

### cpubench &; iobench & <div id='id2d'>
Se ejecuto un ctrl-p para ver la lista de proceso antes de comenzar 

    12: 1108 MFLOP100T
    12: 1128 MFLOP100T
    12: 1128 MFLOP100T
                        13: 60 OPW100T, 60 OPR100T
    12: 1128 MFLOP100T
    12: 1128 MFLOP100T
                        13: 33 OPW100T, 33 OPR100T
    12: 1128 MFLOP100T
    12: 1108 MFLOP100T
                        13: 33 OPW100T, 33 OPR100T
    12: 1098 MFLOP100T
    12: 1108 MFLOP100T
                        13: 33 OPW100T, 33 OPR100T
    12: 1118 MFLOP100T


    [Name: init, Pid: 1, State: sleep 
    Prio: 2, Counter: 30, timeexe: 2540, lastexe: 2540] 


    [Name: sh, Pid: 2, State: sleep 
    Prio: 2, Counter: 22, timeexe: 2539, lastexe: 2540] 


    [Name: iobench, Pid: 13, State: runble
    Prio: 2, Counter: 1183, timeexe: 1182, lastexe: 3721] 


    [Name: cpubench, Pid: 12, State: run   
    Prio: 2, Counter: 1184, timeexe: 1183, lastexe: 3721] 


                        13: 33 OPW100T, 33 OPR100T
    12: 1118 MFLOP100T
    12: 1108 MFLOP100T
                        13: 33 OPW100T, 33 OPR100T
    12: 1128 MFLOP100T
    12: 1118 MFLOP100T
                        13: 33 OPW100T, 33 OPR100T
    12: 1118 MFLOP100T
    12: 1118 MFLOP100T
                        13: 33 OPW100T, 33 OPR100T
    12: 1118 MFLOP100T
                        13: 33 OPW100T, 33 OPR100T
    12: 1128 MFLOP100T

    Termino cpubench 12: total ops 268435456u --> 
    prioridad: 2
    veces elegido: 2115
    ultima ejecuciòn: 4652 

                        13: 33 OPW100T, 33 OPR100T

    Termino iobench 13: total ops 1408u -->	
    prioridad: 2
    veces elegido: 2224
    ultima ejecuciòn: 4652 

### cpubench &; cpubench &; iobench & <div id='id2e'>

Podemos suponer algo muy similar al caso anterior. Donde las operaciones del programa iobench sean considerablemente menores a las de ambos cpubench, ya que ahora el scheduler tendria que alternar entre tres programas, lo cual corresponde a menor tiempo de ejecucion total para cada proceso, particularmente, menos tiempo de ejecucion para el programa iobench.

    9: 575 MFLOP100T
    7: 564 MFLOP100T
    9: 564 MFLOP100T
    7: 564 MFLOP100T
    9: 1128 MFLOP100T
    7: 1128 MFLOP100T
    9: 1195 MFLOP100T
    7: 1184 MFLOP100T
    9: 1195 MFLOP100T
    7: 1195 MFLOP100T
    9: 1195 MFLOP100T
    7: 1195 MFLOP100T
    7: 1207 MFLOP100T
    9: 1214 MFLOP100T
                        5: 29 OPW100T, 29 OPR100T
    7: 1195 MFLOP100T
    9: 1195 MFLOP100T
    7: 1195 MFLOP100T
    9: 1195 MFLOP100T
    7: 1195 MFLOP100T
    9: 1207 MFLOP100T
    7: 1195 MFLOP100T
    9: 1195 MFLOP100T
                        5: 16 OPW100T, 16 OPR100T
    7: 1195 MFLOP100T
    9: 1195 MFLOP100T
    7: 1195 MFLOP100T
    9: 1214 MFLOP100T
    7: 1195 MFLOP100T
    9: 1195 MFLOP100T
    7: 1195 MFLOP100T
    9: 1195 MFLOP100T
                        5: 16 OPW100T, 16 OPR100T
    7: 1195 MFLOP100T
    9: 1207 MFLOP100T
    7: 1207 MFLOP100T
    9: 1195 MFLOP100T
    7: 1195 MFLOP100T
    9: 1207 MFLOP100T
    7: 1195 MFLOP100T
    9: 1195 MFLOP100T
                        5: 16 OPW100T, 16 OPR100T
    Termino cpubench 7: total ops 268435456u --> 
    prioridad: 2
    veces elegido: 1057
    ultima ejecuciòn: 2443 
    Termino cpubench 9: total ops 671088640u --> 
    prioridad: 2
    veces elegido: 1057
    ultima ejecuciòn: 2447 
                        5: 16 OPW100T, 16 OPR100T
    Termino iobench 5: total ops 768u -->	
    prioridad: 0
    veces elegido: 1239
    ultima ejecuciòn: 2448 

De la misma forma que en el caso anterior, reducir el quantum a uno diez veces mas pequeño tambien favorecera al programa iobench tal y como se ve a continuacion. 

    18: 1172 MFLOP100T
    16: 1139 MFLOP100T
                        14: 215 OPW100T, 215 OPR100T
    18: 1172 MFLOP100T
    16: 1139 MFLOP100T
                        14: 168 OPW100T, 168 OPR100T
    18: 1172 MFLOP100T
    16: 1139 MFLOP100T
                        14: 166 OPW100T, 166 OPR100T
    18: 1184 MFLOP100T
    16: 1139 MFLOP100T
                        14: 168 OPW100T, 168 OPR100T
    .
    .
    .

    [Name: init, Pid: 1, State: sleep 
    Prio: 2, Counter: 30, timeexe: 36063, lastexe: 36063] 


    [Name: sh, Pid: 2, State: sleep 
    Prio: 2, Counter: 29, timeexe: 36057, lastexe: 36060] 


    [Name: iobench, Pid: 14, State: runble
    Prio: 0, Counter: 7925, timeexe: 15848, lastexe: 51908] 


    [Name: cpubench, Pid: 16, State: run   
    Prio: 0, Counter: 7925, timeexe: 15848, lastexe: 51908] 


    [Name: cpubench, Pid: 18, State: runble
    Prio: 0, Counter: 7923, timeexe: 15846, lastexe: 51907] 

    .
    .
    .
    16: 1139 MFLOP100T
    18: 1172 MFLOP100T
                        14: 166 OPW100T, 166 OPR100T
    Termino cpubench 18: total ops 1476395008u --> 
    prioridad: 0
    veces elegido: 10483
    ultima ejecuciòn: 57027 
    Termino cpubench 16: total ops 268435456u --> 
    prioridad: 0
    veces elegido: 10531
    ultima ejecuciòn: 57073 
    Termino iobench 14: total ops 6656u -->	
    prioridad: 0
    veces elegido: 10630
    ultima ejecuciòn: 57076 

### CPUbench solo  <div id='id3a'> <div id='id3'>

Bajo la misma hipotesis pero con la politica mlfq con 
//Priority boost
~~~
En un escenario de cpubench todos los procesos deberían recibir la misma cantidad de quantums y tener el mismo número de MFLOPs totales. 
~~~
Vemos que como ambos procesos pasan por la funcion `yield(void)` al abandonar el control solo cuando llega una interrupcion de reloj. En nuestro codigo, cada vez que un proceso pasa por esta funcion ocurre

  `if (p->priority > MINPRIO)`<br>
  `{`<br>
    `p->priority--;`<br>
  `}`<br>

Luego cuando el control pasa a la funcion scheduler su campo *priority* sera inferior a la maxima. De este modo, si hubiese un proceso con mayor prioridad y en estado RUNNEABLE este se ejecutara antes. 
En este caso, ambos procesos tienen el mismo comportamiento con respecto al manejo de su prioridad. Luego como son los unicos y entonces la interacion por la lista de procesos finalizara sin elegir otro. Para que el scheduler vuelva a elegir alguno de los cpubench, baja la prioridad siempre que esta no sea la minima

   `if (curr_prio == MINPRIO)`<br>
   `{`<br>
    `curr_prio = MAXPRIO;`<br>
    `}`<br>
    `else`<br>
    `{`<br>
    `curr_prio--;`<br>
    `}`<br>

Como ambos procesos siguen la misma estructura, ambos seran elegidos alternadamente la misma cantidad de veces por el scheduler, ya que este elije solo por la prioridad. 

El output final de dos procesos cpubench es similar a este 

    Termino cpubench 6: total ops 2415919104u --> prioridad: 0 
    veces elegido: 1067 
    Termino cpubench 5: total ops 2415919104u --> prioridad: 0 
    veces elegido: 1072 


### cpubench & cpubench & iobench  <div id='id3b'>

Bajo la hipotesis 
~~~
En un escenario con dos(2) cpubench y un(1) IObench: MLFQ debería favorecer al proceso que hace IO en comparación con RR. O sea se debería observar más cambios de contexto y más IOPs totales
~~~
Esto es acorde con la implementacion de la politica MLFQ que hicimos, pues asi como la unica forma de que el programa cpubench deje el control del cpu es pasando por yield(void), el programa iobench pasa por una interrupcion que funciona distinto. 

Las interrupciones de otros dispositivos no llaman a yield(void) en su lugar llaman a sleep(void *chan, struct spinlock *lk)

  `if (p->priority < MAXPRIO)`<br>
  `{`<br>
    `p->priority++;`<br>
  `}`<br>

Como el scheduler funciona de la misma forma que en el caso anterior, el proceso iobench siempre tendra mayor prioridad y sera elegido en las primeras iteraciones 

~~~
¿Se puede producir starvation en el nuevo planificador?
~~~
Si, los programas cpubench siempre tendran prioridad 0, mientras que el programa iobench siempre tendra prioridad 2. Este ultimo se ejecutara siempre que este RUNNEABLE. Es por esto que incluimos Priority boost, el cual sube la prioridad cada cierta cantidad de ticks para todos los procesos. Dandole mas oportunidades a los programas cpubench de ejecutarse, aunque su prioridad no dejaria de bajar en cada iteracion. Por lo que incluso con esta medida, los programas cpubench siempre terminaran con prioridad 0 y el programa iobench con prioridad 2. 

    Termino cpubench 19: total ops 2415919104u --> prioridad: 0 
    veces elegido: 1047 
    ultima ejecucion: 6665 
    Termino cpubench 21: total ops 2415919104u --> prioridad: 0 
    veces elegido: 1056 
    ultima ejecucion: 6674 
                        22: 11 OPW100T, 11 OPR100T
    Termino iobench 22: total ops 1024u -->	prioridad: 2 
    veces elegido: 1634 
    ultima ejecucion: 6676







 

