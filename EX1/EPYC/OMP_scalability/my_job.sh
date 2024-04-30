#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="EpycOMP"
#SBATCH --partition=EPYC
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=64
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --nodelist=epyc[004]
#SBATCH --output="EpycOMP.out"

module load openMPI/4.1.5/gnu/12.2.1 
export OMP_PLACES=cores
export OMP_PROC_BIND=close

loc=$(pwd)
cd ../..
make parallel.x

datafile=epyc_omp_timing.csv

echo "threads_per_socket, ordered_mean, static_mean" >> $datafile

mpirun -np 1 -N 1 --map-by socket parallel.x -i -f "initial_10000.pgm" -k 10000


## running a single time with only 1 thread to have a serial result
th_socket=1
export OMP_NUM_THREADS=$th_socket
echo -n "${th_socket}," >> $datafile
mpirun -np 1 --map-by socket parallel.x -r -f "initial_10000.pgm" -e 0 -n 100 -s 0 -k $size >>$datafile
mpirun -np 1 --map-by socket parallel.x -r -f "initial_10000.pgm" -e 1 -n 100 -s 0 -k $size >>$datafile
truncate -s -1 $datafile
echo >> $datafile


for th_socket in $(seq 4 4 64); do

  export OMP_NUM_THREADS=$th_socket
  echo -n "${th_socket}," >> $datafile
  mpirun -np 1 --map-by socket parallel.x -r -f "initial_10000.pgm" -e 0 -n 100 -s 0 -k $size >>$datafile
  mpirun -np 1 --map-by socket parallel.x -r -f "initial_10000.pgm" -e 1 -n 100 -s 0 -k $size >>$datafile
  
  truncate -s -1 $datafile  # removing the last comma in the line
  echo >> $datafile # printing the new line in the csv file
done

## Moving the result file into the right folder
mv $datafile $loc

module purge
