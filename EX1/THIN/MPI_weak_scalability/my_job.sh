#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="ThinWeak"
#SBATCH --partition=THIN
#SBATCH --nodes=2
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=12
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --nodelist=thin[009-010]
#SBATCH --output="ThinWeak.out"

module load openMPI/4.1.5/gnu/12.2.1 
export OMP_PLACES=cores
export OMP_PROC_BIND=close

loc=$(pwd)
cd ../..
make parallel.x

datafile=thin_weak_timing.csv

echo "size, procs, ordered_mean, static_mean" > $datafile

## initialize a playground
export OMP_NUM_THREADS=12

procs=0
## increasing the sizes as 10000 * sqrt(num_processes)
for size in 10000 14142 17320 20000; do
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
