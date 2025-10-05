==========================================================
Esta es la documentación para compilar y ejecutar su tarea
==========================================================

Se está ejecutando el comando: less README.txt

***************************
*** Para salir: tecla q ***
***************************

Para avanzar a una nueva página: tecla <page down>
Para retroceder a la página anterior: tecla <page up>
Para avanzar una sola línea: tecla <enter>
Para buscar un texto: tecla / seguido del texto (/...texto...)
         por ejemplo: /ddd

-----------------------------------------------

Instrucciones para la tarea 4

Ud. debe programar las funciones solicitadas en el archivo pedir.c.
Ya hay una plantilla de lo pedido en pedir.c.plantilla.

Pruebe su tarea bajo Debian 12 de 64 bits.  Estos son los requerimientos
para aprobar su tarea:

+ make run debe felicitarlo por aprobar este modo de ejecución.
+ make run-g debe felicitarlo por aprobar este modo de ejecución.
+ make run-san debe felicitarlo por aprobar este modo de ejecución y no
  señalar ningún problema como errores de manejo de memoria, excepto
  el señalado a continuación.
  ==5008==WARNING: ASan doesn't fully support makecontext/swapcontext functions
  and may produce false positives in some cases!
+ make run-thr *no* funciona en esta tarea.
 
Cuando pruebe su tarea asegúrese de que su computador está
configurado en modo alto rendimiento y que no se estén corriendo otros
procesos intensivos en uso de CPU al mismo tiempo.  De otro modo podría
no aprobar el test de prueba.

Para depurar:

make ddd

Tenga en consideración que depurar programas con procesos multi nano threaded
es más difícil que depurar programas secuenciales.  El panel de threads
muestra los pthreads que se están ejecutando, y para el caso de esta
tarea, los pthreads corresponden a los cores.  No hay una opción para mostrar
los nano threads en ddd.

Video con ejemplos de uso de ddd: https://youtu.be/FtHZy7UkTT4
Archivos con los ejemplos:
  https://www.u-cursos.cl/ingenieria/2020/2/CC3301/1/novedades/r/demo-ddd.zip

-----------------------------------------------

Entrega de la tarea

Invoque el comando make zip para ejecutar todos los tests y generar un
archivo pedir.zip que contiene pedir.c, con su solución, y resultados.txt,
con la salida de make run, make run-g y make run-san.

Entregue por U-cursos el archivo pedir.zip

A continuación es muy importante que descargue de U-cursos el mismo
archivo que subió.  Luego examine el archivo pedir.c revisando que es
su última versión.  Es frecuente que no lo sea producto de un defecto
de U-cursos.

-----------------------------------------------

Limpieza de archivos

make clean

Hace limpieza borrando todos los archivos que se pueden volver
a reconstruir a partir de los fuentes: *.o binarios etc.
