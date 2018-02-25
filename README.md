# Arquitectura de computadoras
# Simulador de memoria cache

En este repositorio se encuentra el código en lenguaje C que se escribió para simular una memoria cache. El archivo que se modificó, principalmente, fue `cache.c`. El objetivo principal del simulador es recopilar ciertas estadísticas que muestran el desempeño de la memoria cache dependiendo de los diferentes parámetros que se le impongan.

Para correr una instancia del simulador lo único que se tiene que hacer es ingresar al directorio en el que se tienen guardados los archivos y correr los siguientes comandos en la terminal:

`make clean`
`make`
`./sim -is 1024 -ds 1024 -bs 64 -a 16   trazas/spice.trace`

Esto imprimirá los parámetros que se utilizaron y las estadísticas correspondientes.
