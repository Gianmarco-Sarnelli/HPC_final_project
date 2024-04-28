#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "mpi.h"
#include <omp.h>
#include <getopt.h>


void parallel_read_pgm_image(void **image, const char *image_name, int offset, int portion_size) {
 
	MPI_File fh;
	MPI_Status status;
	MPI_File_open(MPI_COMM_WORLD, image_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &fh);
	MPI_File_seek(fh, offset, MPI_SEEK_SET);
	MPI_File_read(fh, *image, portion_size, MPI_UNSIGNED_CHAR, &status);

	MPI_Barrier(MPI_COMM_WORLD);

	MPI_File_close(&fh);

}

// ######################################################################################################################################

// ######################################################################################################################################

void parallel_write_pgm_image(void *image, int maxval, int xsize, int my_chunk, const char *image_name, int offset) {

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int color_depth = 1 + (maxval > 255);
	MPI_File fh;

	MPI_Status status;
	int err=0;
	const int header_size = 23;
	if (rank == 0) {
		char header[header_size];
		sprintf(header, "P5\n%8d %8d\n%d\n", xsize, xsize, color_depth);
		err = MPI_File_open(MPI_COMM_SELF, image_name, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
		// Mode: write only + create the file if it doesn't exist
		if (err!=0) {
			printf("Error opening file for writing header: %d\n", err);
			return;
		}
		err = MPI_File_write(fh, (const void *)header, header_size, MPI_CHAR, &status);
		if (err!=0) {
			printf("Error writing header: %d\n", err);
			return;
		}
		MPI_File_close(&fh);
	}
	
	//modify!!!!!!!!!!
	MPI_Barrier(MPI_COMM_WORLD);

	err = MPI_File_open(MPI_COMM_WORLD, image_name, MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
	if (err!=0) {
		printf("Error opening file for writing data: %d\n", err);
		return;
	}

	err += MPI_File_write_at_all(fh, (MPI_Offset)offset,(const void *)image, my_chunk * xsize * color_depth, MPI_UNSIGNED_CHAR, &status);
	if (err!=0) {
		printf("Error writing data: %d\n", err);
		return;
	}

	MPI_Barrier(MPI_COMM_WORLD);

	err = MPI_File_close(&fh);
	if (err!=0) {
		printf("Error closing file: %d\n", err);
		return;
	}
}

// ######################################################################################################################################

// ######################################################################################################################################

void write_snapshot(unsigned char *playground, int maxval, int xsize, int ysize, const char *basename, int iteration, int offset)
{
	//char filename[256];
	char *filename = (char *)malloc(strlen(basename)+10);
	if (snprintf(filename, strlen(basename)+10, "%s_%05d.pgm", basename, iteration) < 0)
		printf("Error writing the file name\n");
	
	parallel_write_pgm_image((void *)playground, maxval, xsize, ysize, (const char*)filename, offset);
}

// ######################################################################################################################################

// ######################################################################################################################################

int main(){
	
	int   k      = 24;  //size of the squared  playground
	
	char image_name[] = "TESTTEST.pgm";
	
	// Initializing MPI
	MPI_Init(NULL, NULL);
	int my_rank;
	int size;
	
	// Getting the rank and the size
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	// Checking the number of processes and threads
	if (my_rank == 0)
		printf("MPI initialized with %d processes\n", size);
		printf("There are %d omp threads \n", omp_get_num_threads());
	
	// Creating variables to subdivide the playground among MPI processes
	int chunk = k / size;
	int mod = k % size;
	int my_chunk = chunk + (my_rank < mod); // Number of rows for the MPI process
	int my_first = my_rank*chunk + (my_rank < mod ? my_rank : mod); // Starting row for the MPI process
	int my_n_cells = my_chunk * k;  // Number fo cells for the MPI process
	
	const int header_size = 23;
	int my_offset = header_size + my_first * k; // Offset in the pgm file of the data for the MPI process
	
	unsigned char *playground = (unsigned char *)malloc(my_n_cells * sizeof(unsigned char));
	unsigned char *new_playground = (unsigned char *)malloc(my_n_cells * sizeof(unsigned char));
	
	//MODIFYING THE PLAYGROUND
	for (int i=0; i<my_n_cells; i++){
		playground[i] = i%7;
	}
	
	// WRITING THE FILE IN PARALLEL USING THE OLD METHOD
	parallel_write_pgm_image((void *)playground, 1, k, my_chunk, (const char *)image_name, my_offset);
	
	// Reading the pgm file in parallel
	parallel_read_pgm_image((void *)new_playground, image_name, my_offset, my_n_cells);
	
	int errors = 0;
	int error_sum;
	
	for (int i=0; i<my_n_cells; i++){
		if (new_playground[i] != playground[i]){
			errors++;
		}
	}
	MPI_Reduce(&errors, &error_sum, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
	if (my_rank == 0){
		printf("There are %d errors in the playground\n", error_sum);
	}
	return 0;
	
	
	
}
























