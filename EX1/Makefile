
OBJECTS=GoL_parallel_main.o GoL_parallel_init_evol.o GoL_parallel_read_write.o


parallel.x: $(OBJECTS)
	mpicc -O3 -fopenmp -march=native -g -Wall -IInclude -lm $(OBJECTS) -o parallel.x

GoL_parallel_main.o: GoL_parallel_main.c
	mpicc -fopenmp -march=native -g -IInclude -c GoL_parallel_main.c
	
GoL_parallel_init_evol.o: GoL_parallel_init_evol.c
	mpicc -fopenmp -march=native -g -IInclude -c GoL_parallel_init_evol.c

GoL_parallel_read_write.o: GoL_parallel_read_write.c
	mpicc -fopenmp -march=native -g -IInclude -c GoL_parallel_read_write.c
	
	
serial.x: GoL_serial.c
	mpicc -march=native -g GoL_serial.c -o serial.x
	

clean:
	rm *.x *.o *.pgm *.csv









