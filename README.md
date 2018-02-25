# Arquitectura de computadoras
# Simulador de memoria cache

En este repositorio se encuentra el código en lenguaje C que se escribió para simular una memoria cache. El archivo que se modificó, principalmente, fue `cache.c`. El objetivo principal del simulador es recopilar ciertas estadísticas que muestran el desempeño de la memoria cache dependiendo de los diferentes parámetros que se le impongan.

Las banderas son las siguientes:

./sim -h
	-h:  		this message

	-bs <bs>: 	set cache block size to <bs>
	-us <us>: 	set unified cache size to <us>
	-is <is>: 	set instruction cache size to <is>
	-ds <ds>: 	set data cache size to <ds>
	-a <a>: 	set cache associativity to <a>
	-wb: 		set write policy to write back
	-wt: 		set write policy to write through
	-wa: 		set allocation policy to write allocate
	-nw: 		set allocation policy to no write allocate

Para correr una instancia del simulador lo único que se tiene que hacer es ingresar al directorio en el que se tienen guardados los archivos y correr los siguientes comandos en la terminal:

`make clean`
`make`
`./sim -is 1024 -ds 1024 -bs 64 -a 16   trazas/spice.trace`

Esto imprimirá los parámetros que se utilizaron y las estadísticas correspondientes.
