#ifndef GOL_PARALLEL_INIT_EVOL
#define GOL_PARALLEL_INIT_EVOL

char *  init_playground(unsigned long int n_cells);
void static_evolution(unsigned char *local_playground, int xsize, int my_chunk, int my_offset,  int n, int s);
void ordered_evolution(unsigned char *my_grid, int xsize, int my_chunk, int my_offset,  int n, int s);
int l_ind(unsigned char *my_grid, int xsize, int stride, int *l_ind_pos, int *l_ind_dist);
int sanity_check_ordered(unsigned char *my_grid, int xsize, int my_chunk, unsigned char *top_ghost_row, unsigned char *bottom_ghost_row);

#endif
