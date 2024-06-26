#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <getopt.h>

#include <immintrin.h>  //vector intrinsic
 

 struct timespec ts;
#define CPU_TIME (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts), (double) ts.tv_sec + (double) ts.tv_nsec * 1e-9)

#define MAXVAL 1

#define INIT 1
#define RUN  2

#define K_DFLT 100

#define ORDERED 0
#define STATIC 1

void write_pgm_image( char *image, int maxval, int xsize, int ysize, const char *image_name){
	/*
	* image        : a pointer to the memory region that contains the image
	* maxval       : either 255 or 65536
	* xsize, ysize : x and y dimensions of the image
	* image_name   : the name of the file to be written
	*
	*/

	FILE* image_file; 
	image_file = fopen(image_name, "wb");
	
	if(image_file == NULL)
		printf("The file didn't open!\n");
	
	
	// Writing header
	// The header's format is as follows, all in ASCII.
	// "whitespace" is either a blank or a TAB or a CF or a LF
	// - The Magic Number (see below the magic numbers)
	// - the image's width
	// - the height
	// - a white space
	// - the image's height
	// - a whitespace
	// - the maximum color value, which must be between 0 and 65535
	//
	// if he maximum color value is in the range [0-255], then
	// a pixel will be expressed by a single byte; if the maximum is
	// larger than 255, then 2 bytes will be needed for each pixel
	//


	int color_depth = 1 + ( maxval > 255 );

	fprintf(image_file, "P5\n# generated by\n# Gianmarco Sarnelli\n%d %d\n%d\n", xsize, ysize, maxval);

	// Writing file

	fwrite( image, 1, xsize*ysize*color_depth, image_file); 

	fclose(image_file); 
	return ;

}

// *********************************************************************************************************************************

// *********************************************************************************************************************************


void read_pgm_image( void **image, int *maxval, int *xsize, int *ysize, const char *image_name)
/*
 * image        : a pointer to the pointer that will contain the image
 * maxval       : a pointer to the int that will store the maximum intensity in the image
 * xsize, ysize : pointers to the x and y sizes
 * image_name   : the name of the file to be read
 *
 */
{
  FILE* image_file;
  image_file = fopen(image_name, "r"); 

  *image = NULL;
  *xsize = *ysize = *maxval = 0;
  
  char    MagicN[2];
  char   *line = NULL;
  size_t  k, n = 0;
  
  // get the Magic Number
  k = fscanf(image_file, "%2s%*c", MagicN );

  // skip all the comments
  k = getline( &line, &n, image_file);
  while ( (k > 0) && (line[0]=='#') )
    k = getline( &line, &n, image_file);

  if (k > 0)
    {
      k = sscanf(line, "%d%*c%d%*c%d%*c", xsize, ysize, maxval);
      if ( k < 3 )
	      fscanf(image_file, "%d%*c", maxval);
    }
  else
    {
      *maxval = -1;         // this is the signal that there was an I/O error
			    // while reading the image header
      free( line );
      return;
    }
  free( line );
  
  int color_depth = 1 + ( *maxval > 255 );
  unsigned int size = *xsize * *ysize * color_depth;
  
  if ( (*image = (char*)malloc( size )) == NULL )
    {
      fclose(image_file);
      *maxval = -2;         // this is the signal that memory was insufficient
      *xsize  = 0;
      *ysize  = 0;
      return;
    }
  
  if ( fread( *image, 1, size, image_file) != size )
    {
      free( image );
      image   = NULL;
      *maxval = -3;         // this is the signal that there was an i/o error
      *xsize  = 0;
      *ysize  = 0;
    }  

  fclose(image_file);
  return;
}

// *********************************************************************************************************************************

// *********************************************************************************************************************************

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
	
	// Return the pointer to the allocated memory
	//return (void *)my_grid;
	return my_grid;
}

// *********************************************************************************************************************************

// *********************************************************************************************************************************

void ordered_evolution(unsigned char *mygrid, int xsize, int ysize, int n, int s){

	// Here we use only a single grid of chars for the evolution, alive cells are 1 and dead cells are 0
	// To make it easier to move in the grid we declare the variables ***_move
	
	//variables for the snapshot file
	char * fname;
	fname = (char*) malloc(46);
	
	char nei; // number of live neighbours
	
	//This will help in the computation of neighbuouring cells
	long int left_move;    // left_move = -1 + (xsize if x == 0). This will make it go up a row if on the left border
	long int right_move;   // right_move = +1 - (xsize if x == xsize-1). This will make it go down a row if on the right border
	long int up_move;      // up_move = -xsize + (xsize*ysize if y == 0)
	long int down_move;    // down_move = xsize - (xsize*ysize if y == ysize-1)
	long int pos;          // pos = y*xsize + x.   Current position
	
	char my_current;

	for (int gen = 0; gen < n; gen++){
	
		//elapsed time :  2.088 sec (the use of a single grid for work and for snapshot hepled a bit)

		for (int y = 0; y < ysize; y++){

			for (int x = 0; x < xsize; x++){
				
				left_move = -1 + (xsize * (x == 0)); //This will make it go up a row if on the left border
				right_move = +1 - (xsize * (x == xsize-1)); //This will make it go down a row if on the right border
				up_move = -xsize + (xsize*ysize * (y == 0));
				down_move = xsize - (xsize*ysize * (y == ysize-1));
				pos = y*xsize + x;   //Current position
				
				nei = 0;
				
				// in the following steps the position of neighbouring cells are computed using i and "left_r", "right_r"
				nei += mygrid[pos + up_move + left_move];
				nei += mygrid[pos + up_move];
				nei += mygrid[pos + up_move + right_move];
				nei += mygrid[pos + left_move];
				nei += mygrid[pos + right_move];
				nei += mygrid[pos + down_move + left_move];
				nei += mygrid[pos + down_move];
				nei += mygrid[pos + down_move + right_move];

				/*
				if (!mygrid[pos] && nei == 3) {           // if the cell is born
					mygrid[pos] = 1;
				} else if (mygrid[pos] && !(nei == 2 || nei == 3)){     // if the cell dies
					mygrid[pos] = 0;
				// if the cell stays the same nothing happens in the ordered evolution
				}
				*/
				
				// Rewritten without if jumps
				my_current = mygrid[pos];
				mygrid[pos] = (!(my_current) && (nei == 3))  ||  (my_current && (nei == 2 || nei == 3)); //The value is 1 only if the cell is born
															// or if the cell stayed alive	
			}
		}// end of iteration on cells
		
		if (gen%s == 0){		
			//snapshot name
			snprintf(fname, 46, "./Snapshots/serial_ordered/snapshot_%05d.pgm", gen);
			
			write_pgm_image( mygrid, 1, xsize, ysize, fname);
			}
	}// End of iteration on gen
	
	if (s == n){		
		//snapshot name
		snprintf(fname, 46, "./Snapshots/serial_ordered/snapshot_%05d.pgm", n);
		
		write_pgm_image( mygrid, 1, xsize, ysize, fname);
	}
	
	if ( fname != NULL )
		free(fname);
}

// *********************************************************************************************************************************

// *********************************************************************************************************************************

void static_evolution( char *mygrid, int xsize, int ysize, int n, int s){
	
	// Here we use a single grid of chars for the evolution and the state of the cell alternates between
	// the first and the second bit of the char. This way we don't need to allocate a new grid

	//variables for the snapshot file
	char * fname;
	char * snap_grid;
	fname = (char*) malloc(46);
	snap_grid = (char*) malloc(xsize*ysize);
	
	// alternating positions of the current and next states of the system
	char current_state;
	char next_state;

	char nei; // number of live neighbours
			
	//This will help in the computation of neighbuouring cells
	long int left_move;    // left_move = -1 + (xsize if x == 0). This will make it go up a row if on the left border
	long int right_move;   // right_move = +1 - (xsize if x == xsize-1). This will make it go down a row if on the right border
	long int up_move;      // up_move = -xsize + (xsize*ysize if y == 0)
	long int down_move;    // down_move = xsize - (xsize*ysize if y == ysize-1)
	long int pos;          // pos = y*xsize + x.   Current position
	
	char my_current;
	
	for (int gen=0; gen<n; gen++){
		
		// alternating positions of the current and next states of the system
		current_state = gen % 2 + 1;
		next_state = 2 - gen % 2;
		
		for(int y=0; y<ysize; y++){
			for(int x=0; x<xsize; x++){
				
				left_move = -1 + (xsize * (x == 0)); //This will make it go up a row if on the left border
				right_move = +1 - (xsize * (x == xsize-1)); //This will make it go down a row if on the right border
				up_move = -xsize + (xsize*ysize * (y == 0));
				down_move = xsize - (xsize*ysize * (y == ysize-1));
				pos = y*xsize + x;   //Current position
				
				nei = 0;
				
				// in the following steps the position of neighbouring cells are computed using i and "left_r", "right_r"
				nei += (mygrid[pos + up_move + left_move] & current_state) == current_state ;
				nei += (mygrid[pos + up_move] & current_state) == current_state ;
				nei += (mygrid[pos + up_move + right_move] & current_state) == current_state ;
				nei += (mygrid[pos + left_move] & current_state) == current_state ;
				nei += (mygrid[pos + right_move] & current_state) == current_state ;
				nei += (mygrid[pos + down_move + left_move] & current_state) == current_state ;
				nei += (mygrid[pos + down_move] & current_state) == current_state ;
				nei += (mygrid[pos + down_move + right_move] & current_state) == current_state ;

				/*
				if (!(mygrid[pos] & current_state) && nei == 3) {           // if the cell is born
					mygrid[pos] |= next_state;
				} else if ((mygrid[pos] & current_state) && !(nei == 2 || nei == 3)){     // if the cell dies
					mygrid[pos] &= current_state;			     
					// this is a trick to change the next state to zero while maintaining the current state intact
				}else{      // if the cell stays the same
					mygrid[pos]  = 3 * ((mygrid[pos] & current_state) == current_state);	// this is a trick to have them both
														// equal to 1 (state 3) or to 0 (if the
														// multiplication gives 0)
				}*/
				
				//REWITTEN IN A FASTER WAY (without if jumps):
				
				my_current = current_state & mygrid[pos];
				mygrid[pos] = my_current + next_state * (  (!(my_current) && (nei == 3))  ||  (my_current && (nei == 2 || nei == 3))  );
				//this is done to preserve the current state and modify the next state (they are located on different bits)
				//explaination: the "next_state" bit will be one only if the cell is born or nothing happens on a live cell
				
			}
		}// end iteration on cells
				
		
		if (gen%s == 0){			
			//snapshot name
			snprintf(fname, 46, "./Snapshots/serial_static/snapshot_%05d.pgm", gen);
			
			//writing the temporary grid
			for (int i=0; i<xsize*ysize; i++){
				//snap_grid will have the value of the grid at the current state
				snap_grid[i] = ((current_state & mygrid[i]) == current_state);
			}
			
			write_pgm_image( snap_grid, 1, xsize, ysize, fname);
			
			
		}
		
	if (s == n){			
		//snapshot name
		snprintf(fname, 46, "./Snapshots/serial_static/snapshot_%05d.pgm", n);
		
		//writing the temporary grid
		for (int i=0; i<xsize*ysize; i++){
			//snap_grid will have the value of the grid at the current state
			snap_grid[i] = ((current_state & mygrid[i]) == current_state);
		}
		
		write_pgm_image( snap_grid, 1, xsize, ysize, fname);
		
		
	}
	
	}//end iterations on gen
	if ( fname != NULL )
		free(fname);
	if ( snap_grid != NULL )
		free(snap_grid);	
}

// *********************************************************************************************************************************

// *********************************************************************************************************************************

void static_evolution2( char *mygrid, int xsize, int ysize, int n, int s){ 	//(It passed the Pentadecathlon test)
										//(Elapsed time: 2.779 sec(without SIMD))
	// This algorithm computes the serial evolution of game of life as
	// a combination of a convolution and a if condition.
	// The convolution computes the number of neighbours and the state
	// of the cell, while the if condition decides the next state of it.
	//
	// This is just a proof of concept for static_evolutionVEC

	//variables for the snapshot file
	char * fname;
	char * padd_grid; //padded grid for the convolution
	fname = (char*) malloc(34);
	padd_grid = (char*) malloc((xsize+2)*(ysize+2));

	char state;
			
	//This will help in the computation of neighbuouring cells
	long int n_cells = xsize*ysize;
	long int padded_row;
	long int my_row;
	long int padded_last_row;
	long int my_last_row;
	long int left_move;    // left_move = -1 + (xsize if x == 0). This will make it go up a row if on the left border
	long int right_move;   // right_move = +1 - (xsize if x == xsize-1). This will make it go down a row if on the right border
	long int up_move;      // up_move = -xsize + (xsize*ysize if y == 0)
	long int down_move;    // down_move = xsize - (xsize*ysize if y == ysize-1)
	long int pos;          // pos = y*xsize + x.   Current position
	
	for (int gen=0; gen<n; gen++){
		
		
		//Filling the padded grid
		
		//Here the first row of the padded grid is filled
		my_last_row = n_cells-xsize;  //useful variable
		padd_grid[0] = mygrid[n_cells-1];		
		for(int x=0; x<xsize; x++){
			padd_grid[x+1] = mygrid[my_last_row+x];
		}		
		padd_grid[xsize+1] = mygrid[my_last_row];
		
		//Here the central rows of the padded grid are filled
		for(int y=0; y<ysize; y++){
			padded_row = (y+1)*(xsize+2);  
			my_row = y*xsize;			//useful variables
			padd_grid[(y+1)*(xsize+2)] = mygrid[my_row +xsize-1];			
			for(int x=0; x<xsize; x++){
				padd_grid[padded_row+x+1] = mygrid[my_row +x];
			}			
			padd_grid[padded_row+xsize+1] = mygrid[my_row];			
		}
		
		//Here the final row of the padded grid is filled
		padded_last_row = (ysize+1)*(xsize+2);		
		padd_grid[padded_last_row] = mygrid[xsize-1];		
		for(int x=0; x<xsize; x++){
			padd_grid[padded_last_row+x+1] = mygrid[x];
		}		
		padd_grid[padded_last_row+xsize+1] = mygrid[0];
		//The padded grid is complete
		
		
		//starting the iteration using SIMD
		for(int y=0; y<ysize; y++){		//multiplying the initial grid by ten
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				mygrid[my_row+x] *=10;
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = y*(xsize+2);	//upper left position
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = y*(xsize+2)+1;	//upper position
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = y*(xsize+2)+2;	//upper right position
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = (y+1)*(xsize+2);	//left position
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = (y+1)*(xsize+2)+2;	//right position
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = (y+2)*(xsize+2);	//lower left position
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = (y+2)*(xsize+2)+1;	//lower position
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = (y+2)*(xsize+2)+2;	//lower right position
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){		//evolving mygrid according to neighbours and cell state
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				state = mygrid[my_row+x];
				mygrid[my_row+x] = (state == 12) || (state == 13) || (state == 3);
				//Explaination:
				//The cell is alive is the state is greater than 10, while the last digit represents the number of
				//neighbours. Hence the state will become 1 iff one of the rules above is true.
				//Idea from cbrew: https://gist.github.com/cbrew/3710694
			}
		}
		
		if (gen%s == 0){			
			//snapshot name
			snprintf(fname, 34, "./Snap_folder2/snapshot_%05d.pgm", gen);
			
			write_pgm_image( mygrid, 1, xsize, ysize, fname);
		}
	
	}//end iterations on gen
	
	if (s == n){			
		//snapshot name
		snprintf(fname, 34, "./Snap_folder2/snapshot_%05d.pgm", n);
		
		write_pgm_image( mygrid, 1, xsize, ysize, fname);
	}
	
	if ( fname != NULL )
		free(fname);
	if ( padd_grid != NULL )
		free(padd_grid);	
}

// *********************************************************************************************************************************

// *********************************************************************************************************************************

void static_evolutionVEC( char *mygrid, int xsize, int ysize, int n, int s){
	//(It passed the Pentadecathlon test)
	//(Elapsed time: 1.744 sec(with SIMD))
	// There's a performance improvement over the standard evolution! 
	//(But the test is not balanced,there have been some performance changes after restarting the pc)

	//variables for the snapshot file
	char * fname;
	char * padd_grid; //padded grid for the convolution
	fname = (char*) malloc(34);
	padd_grid = (char*) malloc((xsize+2)*(ysize+2));

	char state;
			
	//This will help in the computation of neighbuouring cells
	long int n_cells = xsize*ysize;
	long int padded_row;
	long int my_row;
	long int padded_last_row;
	long int my_last_row;
	long int left_move;    // left_move = -1 + (xsize if x == 0). This will make it go up a row if on the left border
	long int right_move;   // right_move = +1 - (xsize if x == xsize-1). This will make it go down a row if on the right border
	long int up_move;      // up_move = -xsize + (xsize*ysize if y == 0)
	long int down_move;    // down_move = xsize - (xsize*ysize if y == ysize-1)
	long int pos;          // pos = y*xsize + x.   Current position
	
	for (int gen=0; gen<n; gen++){
		
		
		//Filling the padded grid
		
		//Here the first row of the padded grid is filled
		my_last_row = n_cells-xsize;  //useful variable
		padd_grid[0] = mygrid[n_cells-1];		
		for(int x=0; x<xsize; x++){
			padd_grid[x+1] = mygrid[my_last_row+x];
		}		
		padd_grid[xsize+1] = mygrid[my_last_row];
		
		//Here the central rows of the padded grid are filled
		for(int y=0; y<ysize; y++){
			padded_row = (y+1)*(xsize+2);  
			my_row = y*xsize;			//useful variables
			padd_grid[(y+1)*(xsize+2)] = mygrid[my_row +xsize-1];			
			for(int x=0; x<xsize; x++){
				padd_grid[padded_row+x+1] = mygrid[my_row +x];
			}			
			padd_grid[padded_row+xsize+1] = mygrid[my_row];			
		}
		
		//Here the final row of the padded grid is filled
		padded_last_row = (ysize+1)*(xsize+2);		
		padd_grid[padded_last_row] = mygrid[xsize-1];		
		for(int x=0; x<xsize; x++){
			padd_grid[padded_last_row+x+1] = mygrid[x];
		}		
		padd_grid[padded_last_row+xsize+1] = mygrid[0];
		//The padded grid is complete
		
		
		//starting the iteration using SIMD
		for(int y=0; y<ysize; y++){		//multiplying the initial grid by ten
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				mygrid[my_row+x] *=10;
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = y*(xsize+2);	//upper left position
			my_row = y*xsize;
			// Process elements in blocks of 16 characters using SSE2
			for (int x=0; x <= xsize-16; x += 16) {
				__m128i vec1 = _mm_loadu_si128((__m128i*)(mygrid + my_row + x));
				__m128i vec2 = _mm_loadu_si128((__m128i*)(padd_grid + padded_row + x));
				vec1 = _mm_add_epi8(vec1, vec2);
				_mm_storeu_si128((__m128i*)(mygrid + my_row + x), vec1);
			}
			// Handle remaining elements (less than a full SIMD block)
			for (int x=xsize-(xsize%16); x < xsize; x++) {
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = y*(xsize+2)+1;	//upper position
			my_row = y*xsize;
			// Process elements in blocks of 16 characters using SSE2
			for (int x=0; x <= xsize-16; x += 16) {
				__m128i vec1 = _mm_loadu_si128((__m128i*)(mygrid + my_row + x));
				__m128i vec2 = _mm_loadu_si128((__m128i*)(padd_grid + padded_row + x));
				vec1 = _mm_add_epi8(vec1, vec2);
				_mm_storeu_si128((__m128i*)(mygrid + my_row + x), vec1);
			}
			// Handle remaining elements (less than a full SIMD block)
			for (int x=xsize-(xsize%16); x < xsize; x++) {
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = y*(xsize+2)+2;	//upper right position
			my_row = y*xsize;
			// Process elements in blocks of 16 characters using SSE2
			for (int x=0; x <= xsize-16; x += 16) {
				__m128i vec1 = _mm_loadu_si128((__m128i*)(mygrid + my_row + x));
				__m128i vec2 = _mm_loadu_si128((__m128i*)(padd_grid + padded_row + x));
				vec1 = _mm_add_epi8(vec1, vec2);
				_mm_storeu_si128((__m128i*)(mygrid + my_row + x), vec1);
			}
			// Handle remaining elements (less than a full SIMD block)
			for (int x=xsize-(xsize%16); x < xsize; x++) {
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = (y+1)*(xsize+2);	//left position
			my_row = y*xsize;
			// Process elements in blocks of 16 characters using SSE2
			for (int x=0; x <= xsize-16; x += 16) {
				__m128i vec1 = _mm_loadu_si128((__m128i*)(mygrid + my_row + x));
				__m128i vec2 = _mm_loadu_si128((__m128i*)(padd_grid + padded_row + x));
				vec1 = _mm_add_epi8(vec1, vec2);
				_mm_storeu_si128((__m128i*)(mygrid + my_row + x), vec1);
			}
			// Handle remaining elements (less than a full SIMD block)
			for (int x=xsize-(xsize%16); x < xsize; x++) {
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = (y+1)*(xsize+2)+2;	//right position
			my_row = y*xsize;
			// Process elements in blocks of 16 characters using SSE2
			for (int x=0; x <= xsize-16; x += 16) {
				__m128i vec1 = _mm_loadu_si128((__m128i*)(mygrid + my_row + x));
				__m128i vec2 = _mm_loadu_si128((__m128i*)(padd_grid + padded_row + x));
				vec1 = _mm_add_epi8(vec1, vec2);
				_mm_storeu_si128((__m128i*)(mygrid + my_row + x), vec1);
			}
			// Handle remaining elements (less than a full SIMD block)
			for (int x=xsize-(xsize%16); x < xsize; x++) {
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = (y+2)*(xsize+2);	//lower left position
			my_row = y*xsize;
			// Process elements in blocks of 16 characters using SSE2
			for (int x=0; x <= xsize-16; x += 16) {
				__m128i vec1 = _mm_loadu_si128((__m128i*)(mygrid + my_row + x));
				__m128i vec2 = _mm_loadu_si128((__m128i*)(padd_grid + padded_row + x));
				vec1 = _mm_add_epi8(vec1, vec2);
				_mm_storeu_si128((__m128i*)(mygrid + my_row + x), vec1);
			}
			// Handle remaining elements (less than a full SIMD block)
			for (int x=xsize-(xsize%16); x < xsize; x++) {
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = (y+2)*(xsize+2)+1;	//lower position
			my_row = y*xsize;
			// Process elements in blocks of 16 characters using SSE2
			for (int x=0; x <= xsize-16; x += 16) {
				__m128i vec1 = _mm_loadu_si128((__m128i*)(mygrid + my_row + x));
				__m128i vec2 = _mm_loadu_si128((__m128i*)(padd_grid + padded_row + x));
				vec1 = _mm_add_epi8(vec1, vec2);
				_mm_storeu_si128((__m128i*)(mygrid + my_row + x), vec1);
			}
			// Handle remaining elements (less than a full SIMD block)
			for (int x=xsize-(xsize%16); x < xsize; x++) {
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){
			padded_row = (y+2)*(xsize+2)+2;	//lower right position
			my_row = y*xsize;
			// Process elements in blocks of 16 characters using SSE2
			for (int x=0; x <= xsize-16; x += 16) {
				__m128i vec1 = _mm_loadu_si128((__m128i*)(mygrid + my_row + x));
				__m128i vec2 = _mm_loadu_si128((__m128i*)(padd_grid + padded_row + x));
				vec1 = _mm_add_epi8(vec1, vec2);
				_mm_storeu_si128((__m128i*)(mygrid + my_row + x), vec1);
			}
			// Handle remaining elements (less than a full SIMD block)
			for (int x=xsize-(xsize%16); x < xsize; x++) {
				mygrid[my_row+x] += padd_grid[padded_row+x];
			}
		}
		for(int y=0; y<ysize; y++){		//evolving mygrid according to neighbours and cell state
			my_row = y*xsize;
			for(int x=0; x<xsize; x++){
				state = mygrid[my_row+x];
				mygrid[my_row+x] = (state == 12) || (state == 13) || (state == 3);
				//Explaination:
				//The cell is alive if the first digit is one, while the last digit represents the number of
				//neighbours. Hence the state will become 1 iff one of the rules above is true.
				//Idea from cbrew: https://gist.github.com/cbrew/3710694
			}
		}	
		
		if (gen%s == 0){			
			//snapshot name
			snprintf(fname, 34, "./Snap_folder2/snapshot_%05d.pgm", gen);
			
			write_pgm_image( mygrid, 1, xsize, ysize, fname);
		}
	
	}//end iterations on gen
	
	if (s == n){			
		//snapshot name
		snprintf(fname, 34, "./Snap_folder2/snapshot_%05d.pgm", n);
		
		write_pgm_image( mygrid, 1, xsize, ysize, fname);
	}
	
	if ( fname != NULL )
		free(fname);
	if ( padd_grid != NULL )
		free(padd_grid);	
}

// *********************************************************************************************************************************

// *********************************************************************************************************************************

int   action = 0;
int   k      = K_DFLT;  //size of the squared  playground
int   e      = ORDERED; //evolution type [0\1]
int   n      = 10000;  // number of iterations 
int   s      = 1;      // every how many steps a dump of the system is saved on a file
                      // 0 meaning only at the end.
char *fname  = NULL;  // name of the file to be either read or written



int main ( int argc, char **argv )
{
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
	char *optstring = "irk:e:f:n:s:";
	int maxval = 1;
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
				k = atoi(optarg); 
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
		char * my_grid = init_playground(n_cells);		
		
		write_pgm_image(my_grid, 1, k, k, fname);
	}

	if (action==RUN){
		
		if(s==0){
			s=n;  // Here we print only at the end
		}
	
		if(e == ORDERED){

			unsigned long int n_cells = k*k;
			unsigned char *my_grid_o = (unsigned char *)malloc(n_cells * sizeof(unsigned char));
			read_pgm_image((void **)&my_grid_o, &maxval, &k, &k, fname);
			double t_start = CPU_TIME;
			
			ordered_evolution(my_grid_o, k, k, n, s);
			
			double time = CPU_TIME - t_start;
			printf("elapsed time ordered: %f sec\n\n", time);
			//write_pgm_image((void *)my_grid_o, 1, k, k, "test_dump_ordered.pgm");
			free(my_grid_o);
		}
		
		else if(e > 0){

			unsigned long int n_cells = k*k;
			unsigned char *my_grid_s = (unsigned char *)malloc(n_cells*sizeof(unsigned char));
			read_pgm_image((void **)&my_grid_s, &maxval, &k, &k, fname);
			double t_start = CPU_TIME;
			
			if(e = 1){
				static_evolution(my_grid_s, k, k, n, s);	//classical static evolution
			}
			else if(e = 2){
				static_evolution2(my_grid_s, k, k, n, s);	//modified static evolution with padded grid
			}
			else if(e == 3){
				static_evolutionVEC(my_grid_s, k, k, n, s);	//modified static evolution with SIMD structure
			}
			else{
				printf("Error!");
			}
			
			
			double time = CPU_TIME - t_start;
			printf("elapsed time static: %f sec\n\n", time);
			//write_pgm_image((void *)my_grid_s, 1, k, k, "test_dump_static.pgm");
			free(my_grid_s);
		}
		
	}


	if ( fname != NULL )
		free ( fname );

	return 0;
}




