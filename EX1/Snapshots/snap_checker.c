#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <getopt.h>

#define MAXVAL 255

void write_pgm_image( void *image, int maxval, int xsize, int ysize, const char *image_name) {
	/*
	* image        : a pointer to the memory region that contains the image
	* maxval       : either 255 or 65536
	* xsize, ysize : x and y dimensions of the image
	* image_name   : the name of the file to be written
	*
	*/

	FILE* image_file; 
	image_file = fopen(image_name, "w"); 

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

	fprintf(image_file, "P5\n%8d %8d\n%d\n", xsize, ysize, maxval);

	// Writing file
	fwrite( image, 1, xsize*ysize*color_depth, image_file);  

	fclose(image_file); 
	return;

}

// ######################################################################################################################################

// ######################################################################################################################################

void read_pgm_image( void **image, int *maxval, int *xsize, int *ysize, const char *image_name){
	/*
	* image        : a pointer to the pointer that will contain the image
	* maxval       : a pointer to the int that will store the maximum intensity in the image (also controlls errors)
	* xsize, ysize : pointers to the x and y sizes
	* image_name   : the name of the file to be read
	*
	*/

	FILE* image_file; 
	image_file = fopen(image_name, "r"); 

	*image = NULL;
	*xsize = *ysize = *maxval = 0;

	char    MagicN[2];
	char   *line = NULL;
	size_t  k, n = 0;

	// get the Magic Number
	k = fscanf(image_file, "%2s%*c", MagicN ); 
	// fscanf reads formatted input from a stream
	// This function returns the number of input items successfully matched and assigned
	//"%*c" discards the next character (usually a newline or whitespace).

	// skip all the comments, the purpose of the while is to skip all the 
	// comments.
	// getline reads an entire line from stream, storing the address of the buffer containing the text into the first argument 
	// If successful, getline() returns the number of characters that are read, including the newline character
	k = getline( &line, &n, image_file); 
	// n stores the size of the read buffer.
	while ( (k > 0) && (line[0]=='#') ) {
		k = getline( &line, &n, image_file);
	}

	// this conditions is true when the comments end and we have read the
	// first meaningful line of data
	if (k > 0)
	{
		k = sscanf(line, "%d%*c%d%*c%d%*c", xsize, ysize, maxval);
		//On success, sscanf returns the number of variables filled
		//If sscanf didn't successfully parse three 
		//items from the line, this line of code attempts 
		//to read an integer from the image_file stream 
		//directly into maxval.
		if ( k < 3 )
			fscanf(image_file, "%d%*c", maxval);
	}
	else //executed if comments end and there is no content in the file
		// or there was an error
	{
		*maxval = -1;         // this is the signal that there was an I/O error
		// while reading the image header
		printf("There was an I/O error while reading the image header");
		free( line );
		return;
	}
	free( line );

	int color_depth = 1 + ( *maxval > 255 );
	unsigned int size = *xsize * *ysize * color_depth;

	if ( (*image = (char*)malloc( size )) == NULL ){
		fclose(image_file);
		*maxval = -2;         // this is the signal that memory was insufficient
		printf("The memory was isufficient");
		*xsize  = 0;
		*ysize  = 0;
		return;
	}

	if ( fread( *image, 1, size, image_file) != size )
	{
		free( image );
		image   = NULL;
		*maxval = -3;         // this is the signal that there was an i/o error
		printf("There was an I/O error");
		*xsize  = 0;
		*ysize  = 0;
	}  

	fclose(image_file);
	return;
}

// ######################################################################################################################################

// ######################################################################################################################################

int main(){
	char* serial_grid;
	char* parallel_grid;
	int maxval = 1;
	int xsize;
	int ysize;
	char serial_base[] = "./serial_ordered/snapshot";
	char parallel_base[] = "./parallel_ordered/snapshot";
	char serialname[36];
	char parallelname[38];
	char diffname[26];
	for(int i=0; i<100; i++){
		snprintf(serialname, 36 , "%s_%05d.pgm", serial_base, i);
		snprintf(parallelname, 38 , "%s_%05d.pgm", parallel_base, i);
		read_pgm_image((void **)&serial_grid, &maxval, &xsize, &ysize, serialname);
		read_pgm_image((void **)&parallel_grid, &maxval, &xsize, &ysize, parallelname);
		unsigned char *diff_grid = (unsigned char *)malloc(xsize*ysize * sizeof(unsigned char));
		for(int x=0; x<xsize; x++){
			for(int y=0; y<ysize; y++){
				diff_grid[y*xsize + x] = serial_grid[y*xsize + x] ^ parallel_grid[y*xsize + x];
			}
		}
		snprintf(diffname, 26 , "./differences/n_%05d.pgm", i);
		write_pgm_image( (void *)diff_grid, 1, xsize, ysize, diffname);
		}
	return 0;
}
