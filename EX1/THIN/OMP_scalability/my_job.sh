#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="ThinOMP"
#SBATCH --partition=THIN
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=12
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --nodelist=thin[004]
#SBATCH --output="ThinOMP.out"

module load openMPI/4.1.5/gnu/12.2.1 
export OMP_PLACES=cores
export OMP_PROC_BIND=close

loc=$(pwd)
cd ../..
make parallel.x

datafile=thin_omp_timing.csv

echo "threads_per_socket, ordered_mean, static_mean" > $datafile

mpirun -np 1 -N 1 --map-by socket parallel.x -i -f "initial_10000.pgm" -k 10000

for th_socket in $(seq 1 1 12); do

  export OMP_NUM_THREADS=$th_socket
  echo -n "${th_socket}," >> $datafile
  (mpirun -np 1 --map-by socket parallel.x -r -f "initial_10000.pgm" -e 0 -n 50 -s 0 -k 10000) >>$datafile
  (mpirun -np 1 --map-by socket parallel.x -r -f "initial_10000.pgm" -e 1 -n 50 -s 0 -k 10000) >>$datafile
  
  truncate -s -1 $datafile  # removing the last comma in the line
  echo >> $datafile # printing the new line in the csv file
done

## Moving the result file into the right folder
mv $datafile $loc

module purge
