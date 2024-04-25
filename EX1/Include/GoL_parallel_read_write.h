#ifndef GOL_PARALLEL_READ_WRITE
#define GOL_PARALLEL_READ_WRITE

void write_pgm_image( void *image, int maxval, int xsize, int ysize, const char *image_name);
void read_pgm_image( void **image, int *maxval, int *xsize, int *ysize, const char *image_name);
void parallel_read_pgm_image(void **image, const char *image_name, int offset, int portion_size);
void parallel_write_pgm_image(void *image, int maxval, int xsize, int my_chunk, const char *image_name, int offset);
void write_snapshot(unsigned char *playground, int maxval, int xsize, int ysize, const char *basename, int iteration, int offset);

#endif
