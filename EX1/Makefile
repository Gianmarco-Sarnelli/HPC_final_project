
OBJECTS=GoL_parallel_main.o GoL_parallel_init_evol.o GoL_parallel_read_write.o
loc=$(pwd)


parallel.x: $(OBJECTS)
	mpicc -fopenmp -march=native -g -Iinclude &(OBJECTS) -o parallel

GoL_parallel_main.o: GoL_parallel_main.c
	mpicc -fopenmp -march=native -g -Iinclude -c GoL_parallel_main.c
	
GoL_parallel_init_evol.o: GoL_parallel_init_evol.c
	mpicc -fopenmp -march=native -g -Iinclude -c GoL_parallel_init_evol.c

GoL_parallel_read_write.o: GoL_parallel_read_write.c
	mpicc -fopenmp -march=native -g -Iinclude -c GoL_parallel_read_write.c
	
	
serial.x: GoL_serial.c
	mpicc -march=native -g GoL_serial.c -o serial.x
	

clean:
	rm $(loc)/*.x $(loc)/*.o









