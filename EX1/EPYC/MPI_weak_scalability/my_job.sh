#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="EpycWeak"
#SBATCH --partition=EPYC
#SBATCH --nodes=2
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=64
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --nodelist=epyc[002-003]
#SBATCH --output="EpycWeak.out"

module load openMPI/4.1.5/gnu/12.2.1 
export OMP_PLACES=cores
export OMP_PROC_BIND=close

loc=$(pwd)
cd ../..
make parallel.x

datafile=epyc_weak_timing.csv

echo "size, procs, ordered_mean, static_mean" >> $datafile

## initialize a playground
export OMP_NUM_THREADS=64

procs=0
## increasing the sizes as 7000 * sqrt(num_processes)
for size in 7000 9900 12124 14000; do
  mpirun -np 1 -N 1 --map-by socket parallel.x -i -f "initial_${size}.pgm" -k $size
  procs=$(($procs + 1)) # increasing the number of processes
  echo -n "${size}," >> $datafile
  echo -n "${procs},">> $datafile
  (mpirun -np $procs --map-by socket parallel.x -r -f "initial_${size}.pgm" -e 0 -n 50 -s 0 -k $size) >>$datafile
  (mpirun -np $procs --map-by socket parallel.x -r -f "initial_${size}.pgm" -e 1 -n 50 -s 0 -k $size) >>$datafile
  
  truncate -s -1 $datafile  # removing the last comma in the line
  echo >> $datafile # printing the new line in the csv file
done

## Moving the result file into the right folder
mv $datafile $loc

module purge
