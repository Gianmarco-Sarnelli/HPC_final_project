
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "mpi.h"
#include <omp.h>
#include <getopt.h>
#include <math.h>
#include "GoL_parallel_init_evol.h"
#include "GoL_parallel_read_write.h"


struct timeval start_time, end_time;
#define INIT 1
#define RUN  2

#define ORDERED 0
#define STATIC 1

int main ( int argc, char **argv ) {
	/*Each character in the optstring represents a single-character
	option that the program accepts. If a character is followed by a colon (:),
	it indicates that the option requires an argument.
	-i: No argument required. Initialize playground.
	-r: No argument required. Run a playground.
	-k: Requires an argument (e.g., -k 100). Playground size.
	-e: Requires an argument (e.g., -e 1). Evolution type.
	-f: Requires an argument (e.g., -f filename.pgm). 
	Name of the file to be either read or written
	-n: Requires an argument (e.g., -n 10000). Number of steps.
	-s: Requires an argument (e.g., -s 1). Frequency of dump.*/
	int   action = 0;
	int   k      = 100;  //size of the squared  playground
	int   e      = 0; //evolution type [0\1]
	int   n      = 100;  // number of iterations 
	int   s      = 1;      // every how many steps a dump of the system is saved on a file
	// 0 meaning only at the end.
	char *fname  = NULL;
	char *optstring = "irk:e:f:n:s:";

	int c;
	/*When the getopt function is called in the while loop,
	it processes the command-line arguments according to
	the format specified in optstring.
	The getopt function returns the next option character or -1 if there are no more options
	*/
	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch(c) {
			case 'i':
				action = INIT; 
				break;
			case 'r':
				action = RUN; 
				break;
			case 'k':
				k = atoi(optarg); //NOTE: if the value excedes 46340 we might have integer overflow on the position!
				break;
			case 'e':
				e = atoi(optarg); 
				break;
			case 'f':
				fname = (char*)malloc( sizeof(optarg)+1 );
				sprintf(fname, "%s", optarg );
				break;  
			case 'n':
				n = atoi(optarg); 
				break;
			case 's':
				s = atoi(optarg); 
				break;
			default :
				printf("argument -%c not known\n", c ); 
				break;
		}
	}
  

	if(action==INIT){
	
		unsigned long int n_cells = k*k;
		// Initializing the playground
		char * grid = init_playground(n_cells);		
		// Writing the pgm file
		write_pgm_image(grid, 1, k, k, fname);
	}

	if (action==RUN){
		
		unsigned long int n_cells = k*k;
		double mean_time;
		double time_elapsed;
		// int mpi_provided_thread_level;  // the current thread support (how MPI will interface with other threads)
		// MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &mpi_provided_thread_level);
		
		// Initializing MPI
		MPI_Init(NULL, NULL);
		int my_rank;
		int size;
		
		// Getting the rank and the size
		MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
		MPI_Comm_size(MPI_COMM_WORLD, &size);
		
		// Checking the number of processes and threads
		if (my_rank == 0){
			printf("MPI initialized with %d processes\n", size);
			printf("There are %d omp threads \n", omp_get_num_threads());
		}
		
		// Creating variables to subdivide the playground among MPI processes
		int chunk = k / size;
		int mod = k % size;
		int my_chunk = chunk + (my_rank < mod); // Number of rows for the MPI process
		int my_n_cells = my_chunk * k;  // Number fo cells for the MPI process
		
		unsigned char *grid = (unsigned char *)malloc(n_cells * sizeof(unsigned char));
		unsigned char *my_grid = (unsigned char *)malloc(my_n_cells * sizeof(unsigned char));
		
		// Reading the initial pgm file
		if (rank == 0){
			int maxval = 1;
			read_pgm_image((void **)&grid, &maxval, &k, &k, fname);
		}
		
		// Getting the arguments for MPI_SCATTER
		int *num_cells = (int *)malloc(size * sizeof(int));
		int *displs = (int *)malloc(size * sizeof(int));
		for (int i=0; i<my_chunk; i++) {
			num_cells[i] = ( (i < k%my_chunk) ? (k/my_chunk)+1 : k/my_chunk ) * k;
			displs[i] = (i==0 ? 0 : (displs[i-1] + num_cells[i-1]) );
		}
		
		// Scattering the grid across the processes
		MPI_Scatterv(grid, num_cells, displs, MPI_UNSIGNED_CHAR, my_playground, num_cells[my_rank], MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
		
		// Starting the evolution
		gettimeofday(&start_time, NULL);
		if(e == ORDERED){

			if(s>0){
				ordered_evolution(my_grid, grid, num_cells, displs, k, my_chunk, n, s);
			}else if (s==0){
				ordered_evolution(my_grid, grid, num_cells, displs, k, my_chunk, n, n);
			}
		}else if(e == STATIC){
		
			if(s>0){
				static_evolution(my_grid, grid, num_cells, displs, k, my_chunk, n, s);
			}else if (s==0){
				static_evolution(my_grid, grid, num_cells, displs, k, my_chunk, n, n);
			}
		}
				
		MPI_Finalize();
		gettimeofday(&end_time, NULL);

		time_elapsed = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_usec - start_time.tv_usec) / 1e6;

		mean_time = time_elapsed / n;
		
		if( grid != NULL)
			free(grid);
		if( my_grid != NULL)
			free(my_grid)
		
		if (my_rank == 0) {
			// 		MODIFY THIS!!!!!!!
			
			printf("%f\n", mean_time);
		
			//FILE *fp = fopen("timing.csv", "a"); 
			//fprintf(fp, "%f,", mean_time);
			//fclose(fp);
		}

	}
	if ( fname != NULL )
		free ( fname );
	
	return 0; 


}  

