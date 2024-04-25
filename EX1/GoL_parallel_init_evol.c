#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include "mpi.h"
#include "GoL_parallel_read_write.h"
#include <omp.h>

char *  init_playground(unsigned long int n_cells){
	// initialising grid to random values

	char* my_grid = (char*) malloc(n_cells*sizeof(char));
	
	// The following is the standard use of random generators:
	
	//double x;  // What will become a random number
	//struct drand48_data randBuffer;  // create the random number generator buffer
	//srand48_r(time(NULL), &randBuffer);  //crates the generator initializer
	//drand48_r(&randBuffer, &x);  //this is the random number generator: takes an initializer and the address of the number to generate
	//printf("Random number: %f\n", x);

	// setting the buffer
	struct drand48_data rand_gen;
	
	// generating a seed
	long int seed;
	seed = time(NULL);
	
	//creating the initializer
	srand48_r(seed, &rand_gen);   // srand48 is the function that initializes the buffer of the function drand48.

	for (int i=0; i<n_cells; i++) {
		// producing a random integer among 0 and 1
		double random_number;
		drand48_r(&rand_gen, &random_number);
		int rand_bool = (int) (random_number>0.5);
		// converting random number to char
		my_grid[i] = (char) rand_bool;
	}//The grid is initialized
	
	return my_grid;
}

// ######################################################################################################################################

// ######################################################################################################################################

void static_evolution(unsigned char *local_playground, int xsize, int my_chunk, int my_offset,  int n, int s) {

	/*
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); //get the rank of the current process
	MPI_Comm_size(MPI_COMM_WORLD, &size); //get the total number of processes

	//int local_size = my_chunk * xsize * sizeof(unsigned char); // Memory size for the MPI process  // remove  !!

	unsigned char *top_ghost_row = (unsigned char *)malloc(xsize * sizeof(unsigned char));
	unsigned char *bottom_ghost_row = (unsigned char *)malloc(xsize * sizeof(unsigned char));

	int top_neighbour = (rank - 1 + size) % size; // Rank of the MPI process above
	int bottom_neighbour = (rank + 1) % size; // Rank of the MPI process below
	
	// Starting the iteration on the generations
	for (int gen=0; gen<n; gen++) {
		
		unsigned char *updated_playground = (unsigned char *)malloc(my_chunk * xsize * sizeof(unsigned char));
		MPI_Request request[2]; // Handle for the initialization comm.
	
		// Getting the ghost rows
		
		// Each process sends its top row to its top neighbour
		MPI_Isend(&local_playground[0], xsize, MPI_UNSIGNED_CHAR, top_neighbour, 0, MPI_COMM_WORLD, &request[0]);
		// Each process sends its bottom row to its bottom neighbour
		MPI_Isend(&local_playground[(my_chunk - 1) * xsize], xsize, MPI_UNSIGNED_CHAR, bottom_neighbour, 1, MPI_COMM_WORLD, &request[1]);
		// Each process receives its bottom ghost row from its bottom neighbour
		MPI_Recv(bottom_ghost_row, xsize, MPI_UNSIGNED_CHAR, bottom_neighbour, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		// Each process receives its top ghost row from its top neighbour
		MPI_Recv(top_ghost_row, xsize, MPI_UNSIGNED_CHAR, top_neighbour, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		// Wait for both routines to complete
		MPI_Waitall(2, request, MPI_STATUSES_IGNORE);
		
		// modify!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		#pragma omp parallel for collapse(2) 
		for (int y = 0; y < my_chunk; y++)
		{
		for (int x = 0; x < xsize; x++)
		{   
		update_cell_static((y == 0 ? top_ghost_row : &local_playground[(y - 1) * xsize]),
		(y == my_chunk - 1 ? bottom_ghost_row : &local_playground[(y + 1) * xsize]),
		local_playground, updated_playground, xsize, my_chunk, x, y);
		}
		}

		//        memcpy(local_playground, updated_playground, local_size * sizeof(unsigned char));
		
		// modify!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		unsigned char *temp = local_playground;
		local_playground = updated_playground;
		updated_playground = temp;
		free(updated_playground); 

		if(gen % s == 0){
		write_snapshot(local_playground, 1, xsize, my_chunk, "./Snapshots_parallel_static/snapshot", gen, my_offset); //CHECK IT!!!!
		}

	} // End cycle on gen

	free(top_ghost_row);
	free(bottom_ghost_row);

	if(s != n){
		write_snapshot(local_playground, 1, xsize, my_chunk, "./Snapshots_parallel_static/snapshot", n, my_offset);
	}
	*/return
}

// ######################################################################################################################################

// ######################################################################################################################################

void ordered_evolution(unsigned char *my_grid, int xsize, int my_chunk, int my_offset,  int n, int s) {

	// The idea to parallelize the evolution is to find the "line_independent" cells for each central line.
	// The line_independent cells are cells whose evolution doesn't depend on the evolution of the previous cell (see image).
	// Knowing this cells we can start a new thread that works from that position.
	// What we are achieving is a decomposition of each row in smaller fragments marked by line_independent cells.
	//
	// Note that this parallelization only efficiently uses OMP, while MPI is used only as an exercise.
	// 
	// To achieve this parallelization we use each char to encode: (see image)
	// # The state of the cell (1 or 0)
	// # The value (prev) of the cell on the left (1 or 0)
	// # The number of neighbours (nei) of the cell including the previous one ( values from 0 to 8 )
	// Only specific cell values can be considered line_indepenent (see image)

	// Algorithm structure:
	// 
	// Initialization
	// 	MPI communications
	// 	Initialization of the grid
	// Evolution
	// 	Evolving the first row and MPI communications
	// 	Evolving central rows
	// 		Finding the line-independent points
	// 		OMP parallelization
	// 		Modifying nei for the last elements of fragments
	// 	Evolving the last row and MPI communications
	// 	Error checking and printing (optional)
	
	// MPI communications stucture:
	//
	// Recv(top_ghost_row) (blocking)
	// IRecv(bottom_ghost_row) (handle "request")
	// 	First row
	// ISend(first row) (no handle)
	//	Central rows
	// Wait(request)
	// 	Last row
	// Isend(last row) (no handle)
	
	char val;
	char diff;
	char prev; // Will be 1 if the previous cell is alive and 0 otherwise
	char nei; // Number of live neighbours
	char my_current, my_new; // current/new state of the cell. It can be either 1 or 0
	int y;
	int stride = 128;  // The minimum size of the fragment of the row in which a thread will work

	//This will help in the computation of neighbuouring cells
	int left_move;    // left_move = -1 + (xsize if x == 0). This will make it go up a row if on the left border
	int right_move;   // right_move = +1 - (xsize if x == xsize-1). This will make it go down a row if on the right border
	int up_move = -xsize;      // This will make it go up a row (There's no way of looping back to the top row because we use ghost rows)
	int down_move = xsize;     // This will make it go down a row
	int pos;          // pos = y*xsize + x.   Current position
	
	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); //get the rank of the current process
	MPI_Comm_size(MPI_COMM_WORLD, &size); //get the total number of processes
	
	int top_neighbour = (rank - 1 + size) % size; // Rank of the MPI process above
	int bottom_neighbour = (rank + 1) % size; // Rank of the MPI process below
	
	unsigned char *top_ghost_row = (unsigned char *)malloc(xsize * sizeof(unsigned char));
	unsigned char *bottom_ghost_row = (unsigned char *)malloc(xsize * sizeof(unsigned char));
	
	int* l_ind_pos = (int *)malloc(((x_size/stride)+1) * sizeof(int))  // positions of line_independent cells 
	int* l_ind_dist = (int *)malloc(((x_size/stride)+1) * sizeof(int)) // distance from the nth l_ind cell to the following l_ind cell (including the first cell)
	int count; // number of l_ind cells found
	int errors;
	int error_sum;
	
	MPI_Request initial[2]; // Handle for the initialization comm.
	
	
	// Getting the ghost rows
	// Each process sends its top row to its top neighbour
	MPI_Isend(&local_playground[0], xsize, MPI_UNSIGNED_CHAR, top_neighbour, 0, MPI_COMM_WORLD, &initial[0]);
	// Each process sends its bottom row to its bottom neighbour
	MPI_Isend(&local_playground[(my_chunk - 1) * xsize], xsize, MPI_UNSIGNED_CHAR, bottom_neighbour, 1, MPI_COMM_WORLD, &initial[1]);
	// Each process receives its bottom ghost row from its bottom neighbour
	MPI_Recv(bottom_ghost_row, xsize, MPI_UNSIGNED_CHAR, bottom_neighbour, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	// Each process receives its top ghost row from its top neighbour
	MPI_Recv(top_ghost_row, xsize, MPI_UNSIGNED_CHAR, top_neighbour, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	// Wait for both routines to complete
        MPI_Waitall(2, initial, MPI_STATUSES_IGNORE);
	
	// Initializing the grid to get the right value of prev and nei for all cells 
	for (int y = 0; y<my_chunk; y++){
		for (int x = 0; x<xsize; x++){
		
			pos = y*xsize + x;   //Current position
			nei = 0;
			left_move = -1 + (xsize * (x == 0)); //This will make it go up a row if on the left border
			right_move = +1 - (xsize * (x == xsize-1)); //This will make it go down a row if on the right border
			
			//computing the number of neighbours
			if (y==0){  // Using the top_ghost_row
				nei+=top_ghost_row[x + left_move] & 1; //  & 1 will give the value of the first bit
				nei+=top_ghost_row[x] & 1;
				nei+=top_ghost_row[x + right_move] & 1;
			}else{
				nei+=my_grid[pos - xsize + left_move] & 1;
				nei+=my_grid[pos - xsize] & 1;
				nei+=my_grid[pos - xsize + right_move] & 1;
			}
			nei+=my_grid[pos + left_move] & 1;
			nei+=my_grid[pos + right_move] & 1;
			if (y==my_chunk-1){  // Using the bottom_ghost_row
				nei+=bottom_ghost_row[x + left_move] & 1;
				nei+=bottom_ghost_row[x] & 1;
				nei+=bottom_ghost_row[x + right_move] & 1;
			}else{
				nei+=my_grid[pos + xsize + left_move] & 1;
				nei+=my_grid[pos + xsize] & 1;
				nei+=my_grid[pos + xsize + right_move] & 1;
			}
			//computing prev
			if (pos!=0){
				prev = my_grid[pos -1] & 1;
			}else{
				prev = 0;
			}
			// the value of each cell will encode its state, the previous cell state and the live neighbours
			my_grid[pos] = (nei<<2) + (prev<<1) + (my_grid[pos] & 1);
		}	
	} // The grid is initialized with the right encoding

	// Sending the bottom row of the last MPI process to begin the gen cycle. The tag is 0
	if (rank == size-1){
		MPI_Isend(&my_grid[(my_chunk - 1) * xsize], xsize, MPI_UNSIGNED_CHAR, bottom_neighbour, 0, MPI_COMM_WORLD, &MPI_REQUEST_NULL);
	}else{
		// Also the top row of each MPI process (except the fist one!) should be sent for the cycle to begin. The tag is 1
		MPI_Isend(&my_grid[0], xsize, MPI_UNSIGNED_CHAR, top_neighbour, 1, MPI_COMM_WORLD, &MPI_REQUEST_NULL);
	}
	
	
	// Starting the iteration on the generations
	for (int gen=0; gen<n; gen++) {
		MPI_Request request;
		
		// The beginning of an MPI cycle is marked by the blocking receive of the upper ghost row. The tag is 0
		MPI_Recv(top_ghost_row, xsize, MPI_UNSIGNED_CHAR, top_neighbour, 0, MPI_COMM_WORLD, &MPI_STATUS_IGNORE);
		
		// From the beginning we ask for the bottom ghost row, but we put a wait only on the last line. The tag is 1
		MPI_IRecv(bottom_ghost_row, xsize, MPI_UNSIGNED_CHAR, bottom_neighbour, 1 , MPI_COMM_WORLD, &request);
		
		// Updating the first line (no parallelization)
		y = 0;
		for (int x = 0; x < xsize; x++){
			
			pos = x;   //Current position
			nei = 0;
			left_move = -1 + (xsize * (x == 0)); //This will make it go up a row if on the left border
			right_move = +1 - (xsize * (x == xsize-1)); //This will make it go down a row if on the right border
			// computing the number of neighbours
			nei+=top_ghost_row[x + left_move] & 1;
			nei+=top_ghost_row[x] & 1;
			nei+=top_ghost_row[x + right_move] & 1;
			nei+=my_grid[pos + left_move] & 1;
			nei+=my_grid[pos + right_move] & 1;
			nei+=my_grid[pos + down_move + left_move] & 1;
			nei+=my_grid[pos + down_move] & 1;
			nei+=my_grid[pos + down_move + right_move] & 1;
			// computing prev 
			if (pos!=0){
				prev = my_grid[pos -1] & 1;
			}else{
				prev = 0;
			}
			// Evolving the state
			my_current = my_grid[pos] & 1;
			my_new = (!(my_current) && (nei == 3))  ||  (my_current && (nei == 2 || nei == 3)); // Evaluates the new state of the grid
			diff = my_new - my_current;
			my_grid[pos] = (nei<<2) + (prev<<1) + my_new;
			// Updating the value of prev in the next cell 
			my_grid[pos + 1] += diff*2; // The value of the second bit will increase or decrease by one
			// Updating the value nei in the other cells
			my_grid[pos + left_move]              += diff*4;
			my_grid[pos + down_move + left_move]  += diff*4; // nei is stored starting from the third bit, it will 
			my_grid[pos + down_move]              += diff*4; // increase or decrease by one
			my_grid[pos + down_move + right_move] += diff*4;
		}// end of work on the first line
		
		// Sending the fist line. The tag is 1
		MPI_Isend(&my_grid[0], xsize, MPI_UNSIGNED_CHAR, top_neighbour, 1, MPI_COMM_WORLD, &MPI_REQUEST_NULL);
		// No need to wait. The cycle cannot continue if the fist row cannot arrive
		
		
		// Creating the arrays of the line_independent points
		count = int l_ind(&my_grid, xsize, stride, &l_ind_pos, &l_ind_dist);
		
						
		// Updating the central lines ( PARALLELIZATION ) 
		for (int y = 1, y<my_chunk-1; y++){
			#pragma omp parallel for schedule( static, 1 ) private(pos, nei, left_move, right_move, prev, my_current, my_new, val, diff)
			for (int i = 0; i<count; i++){
				// Updating the first element
				pos = y*xsize + l_ind_pos[i];
				left_move = -1 + (xsize * ((pos%xsize) == 0));
				right_move = +1 - (xsize * ((pos%xsize) == xsize-1));
				val = my_grid[pos]; // value of the grid in pos
				if (i != 0){
					// Here the first element is a l_ind point
					my_current = val & 1;
					// if the first element is a l_ind point, it will become 1 only if its value is either 9 or 15 (see image)
					my_new = ((val==9) || (val==15)) ? 1 : 0; 
				}else{
					// Here the first element is the first cell of the row
					my_current = val & 1;
					nei = val>>2; // the number of neighbour is stored starting from the third bit on the char
					my_new = (!(my_current) && (nei == 3))  ||  (my_current && (nei == 2 || nei == 3)); 
				}
				diff = my_new - my_current;
				// Updating the value of prev in the next cell
				my_grid[pos + 1] += diff*2					
				// Updating the value of nei in the near cells
				diff *=4;
				mygrid[pos + up_move + left_move]    += diff;  // diff now stores 4*(my_new - my_current)
				mygrid[pos + up_move]                += diff;
				mygrid[pos + up_move + right_move]   += diff;
				// mygrid[pos + left_move]              += diff; // Sending the this information backward would create some irregularities, 
										 // so the right way is to modify the last element of each fragment at the end
				mygrid[pos + right_move]             += diff;
				mygrid[pos + down_move + left_move]  += diff;
				mygrid[pos + down_move]              += diff;
				mygrid[pos + down_move + right_move] += diff;
				
				// Working on the other elements of the fragment
				for(int j = 1; j<l_ind_dist[i] - 1; j++){
					pos++;
					nei = 0;
					left_move = -1 + (xsize * ((pos%xsize) == 0));
					right_move = +1 - (xsize * ((pos%xsize) == xsize-1));
					val = my_grid[pos];
					my_current = val & 1;
					nei = val>>2;
					my_new = (!(my_current) && (nei == 3))  ||  (my_current && (nei == 2 || nei == 3)); 
					diff = my_new - my_current;
					// Updating the value of prev in the next cell
					my_grid[pos + 1] += diff*2					
					// Updating the value of nei in the near cells
					diff *=4;
					mygrid[pos + up_move + left_move]    += diff;
					mygrid[pos + up_move]                += diff;
					mygrid[pos + up_move + right_move]   += diff;
					mygrid[pos + left_move]              += diff;
					mygrid[pos + right_move]             += diff;
					mygrid[pos + down_move + left_move]  += diff;
					mygrid[pos + down_move]              += diff;
					mygrid[pos + down_move + right_move] += diff;
				}// End of work on the fragment	
			}// End of the omp parallel
			
			// Modifying the value of nei in the last element of each fragment
			for (int i = 0; i<count; i++){
				pos = y*xsize + l_ind_pos[i] + l_ind_dist[i] - 1;
				left_move = -1 + (xsize * ((pos%xsize) == 0));
				right_move = +1 - (xsize * ((pos%xsize) == xsize-1));
				nei = 0;
				nei+=my_grid[pos + up_move + left_move] & 1;
				nei+=my_grid[pos + up_move] & 1;
				nei+=my_grid[pos + up_move + right_move] & 1;
				nei+=my_grid[pos + left_move] & 1;
				nei+=my_grid[pos + right_move] & 1;
				nei+=my_grid[pos + down_move + left_move] & 1;
				nei+=my_grid[pos + down_move] & 1;
				nei+=my_grid[pos + down_move + right_move] & 1;
				my_grid[pos] = (nei<<2) + (my_grid[pos] & 3); // The first two bits stay the same
			}
				
		}// End of iteration on central lines
		
		// Updating the last line (no parallelization)
		
		// Waiting for the bottom ghost row to arrive
		MPI_Wait(request, MPI_STATUS_IGNORE);
		
		y = my_chunk - 1;
		for (int x = 0; x < xsize; x++){
			
			pos = y*xsize + x;   // Current position
			nei = 0;
			left_move = -1 + (xsize * (x == 0)); //This will make it go up a row if on the left border
			right_move = +1 - (xsize * (x == xsize-1)); //This will make it go down a row if on the right border
			// Computing the number of neighbours
			nei+=my_grid[pos + up_move + left_move] & 1;
			nei+=my_grid[pos + up_move] & 1;
			nei+=my_grid[pos + up_move + right_move] & 1;
			nei+=my_grid[pos + left_move] & 1;
			nei+=my_grid[pos + right_move] & 1;
			nei+=bottom_ghost_row[x + left_move] & 1;
			nei+=bottom_ghost_row[x] & 1;
			nei+=bottom_ghost_row[x + right_move] & 1;
			// Computing prev 
			prev = my_grid[pos -1] & 1;
			// Evolving the state
			my_current = my_grid[pos] & 1;
			my_new = (!(my_current) && (nei == 3))  ||  (my_current && (nei == 2 || nei == 3));
			diff = my_new - my_current;
			my_grid[pos] = (nei<<2) + (prev<<1) + my_new;
			// Updating the value nei in the other cells
			my_grid[pos + left_move]            += diff*4;
			my_grid[pos + up_move + left_move]  += diff*4; 
			my_grid[pos + up_move]              += diff*4;
			my_grid[pos + up_move + right_move] += diff*4;
			
		}// end of work on the last line
		
		// Checking if the grid is correct
		errors = sanity_check_ordered(&my_grid, xsize, my_chunk, &top_ghost_row, $bottom_ghost_row);
		MPI_Reduce(&errors, &error_sum, 1, MPI_FLOAT, MPI_SUM, 0, MPI_COMM_WORLD);
		if (rank == 0){
			printf("In gen %d there are %d errors in the grid", gen, error_sum);
		}
		
		if(gen % s == 0){
			write_snapshot(my_grid, 1, xsize, my_chunk, "./Snapshots_parallel_ordered/snapshot", gen, my_offset);
		}
		
		// The MPI ends by sending the last row, without it the new gen wouldn't start. The tag is 0
		MPI_Isend(&my_grid[(my_chunk - 1) * xsize], xsize, MPI_UNSIGNED_CHAR, bottom_neighbour, 0, MPI_COMM_WORLD, &MPI_REQUEST_NULL);
		// We don't care about the handle because there's no way of altering the last grid of this chunk if the new MPI process doesn't start

	} // End cycle on gen
	
	free(top_ghost_row);
	free(bottom_ghost_row);
	free(l_ind_pos);
	free(l_ind_dist);

	if(s != n){
		write_snapshot(my_grid, 1, xsize, my_chunk, "./Snapshots_parallel_ordered/snapshot", n, my_offset);
	}
}

// ######################################################################################################################################

// ######################################################################################################################################

int l_ind(unsigned char *my_grid, int xsize, int stride, int *l_ind_pos, int *l_ind_dist){
	// Creates the arrays of the line_independent points and the number count
	char val, check;
	int i = stride -1; // starting position
	int dist = stride; // starting distance
	int count = 1;
	l_ind_pos[0] = 0; // makes sure that the first l_ind cell is the first one
	while (i<xsize){
		val = my_grid[xsize + i]; // value of the cell in the following line for x = i
		// Checking if the cell is a l_ind point
		check = (val<4) || (val>19) || (val==9) || (val==15) || (val==7) || (val==17) || (val==4) || (val==6) || (val==10) || (val==16);
		if (check){
			l_ind_pos[count] = i;
			l_ind_dist[count-1] = dist;
			i+=stride;
			dist = stride;
			count++;
		}else{
			i++;
			dist++;
		}
	}
	// The final element of l_ind_dist will be the dist from the last l_ind cell to the end of the row + 1 (the last element of the row)
	l_ind_dist[count-1] = xsize - l_ind_pos[count-1]; 
	return count
	// The array are created and both use "count" elements	
}

// ######################################################################################################################################

// ######################################################################################################################################

int sanity_check_ordered(unsigned char *my_grid, int xsize, int my_chunk, unsigned char *top_ghost_row, unsigned char *bottom_ghost_row){

	// checks if the grid is correctly evaluated
	// Returns 0 if the grid is correct and the number of errors otherwise
	
	char prev; // Will be 1 if the previous cell is alive and 0 otherwise
	char nei; // Number of live neighbours
	char my_current // current state of the cell. It can be either 1 or 0

	//This will help in the computation of neighbuouring cells
	int left_move;    // left_move = -1 + (xsize if x == 0). This will make it go up a row if on the left border
	int right_move;   // right_move = +1 - (xsize if x == xsize-1). This will make it go down a row if on the right border
	int pos;          // pos = y*xsize + x.   Current position
	int errors = 0;
	
	
	for (int y = 0; y<my_chunk; y++){
		for (int x = 0; x<xsize; x++){
		
			pos = y*xsize + x;   //Current position
			nei = 0;
			left_move = -1 + (xsize * (x == 0)); //This will make it go up a row if on the left border
			right_move = +1 - (xsize * (x == xsize-1)); //This will make it go down a row if on the right border
			
			//computing the number of neighbours
			if (y==0){  // Using the top_ghost_row
				nei+=top_ghost_row[x + left_move] & 1; //  & 1 will give the value of the first bit
				nei+=top_ghost_row[x] & 1;
				nei+=top_ghost_row[x + right_move] & 1;
			}else{
				nei+=my_grid[pos - xsize + left_move] & 1;
				nei+=my_grid[pos - xsize] & 1;
				nei+=my_grid[pos - xsize + right_move] & 1;
			}
			nei+=my_grid[pos + left_move] & 1;
			nei+=my_grid[pos + right_move] & 1;
			if (y==my_chunk-1){  // Using the bottom_ghost_row
				nei+=bottom_ghost_row[x + left_move] & 1;
				nei+=bottom_ghost_row[x] & 1;
				nei+=bottom_ghost_row[x + right_move] & 1;
			}else{
				nei+=my_grid[pos + xsize + left_move] & 1;
				nei+=my_grid[pos + xsize] & 1;
				nei+=my_grid[pos + xsize + right_move] & 1;
			}
			//computing prev
			if (pos!=0){
				prev = my_grid[pos -1] & 1;
			}else{
				prev = 0;
			}
			
			my_current = my_grid[pos] & 1;
			
			if ( my_grid[pos] != (nei<<2) + (prev<<1) + my_current){
				errors++;
			}			
		}	
	}
	if (errors == 0){
		return 0;
	}else{
		return 1;
	}
}



