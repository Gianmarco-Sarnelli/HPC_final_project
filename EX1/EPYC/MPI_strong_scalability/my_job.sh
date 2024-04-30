#!/bin/bash
#SBATCH --no-requeue
#SBATCH --job-name="EpycStrong"
#SBATCH --partition=EPYC
#SBATCH --nodes=2
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=2
#SBATCH --cpus-per-task=64
#SBATCH --exclusive
#SBATCH --time=02:00:00
#SBATCH --nodelist=epyc[007-008]
#SBATCH --output="EpycStrong.out"

module load openMPI/4.1.5/gnu/12.2.1 
export OMP_PLACES=cores
export OMP_PROC_BIND=close

loc=$(pwd)
cd ../..
make parallel.x

datafile=epyc_strong_timing.csv

echo "size, procs, ordered_mean, static_mean" >> $datafile

## initialize a playground
export OMP_NUM_THREADS=64

## repeating the test for different sizes
for size in 8000 1000 12000; do
  mpirun -np 1 -N 1 --map-by socket parallel.x -i -f "initial_${size}.pgm" -k $size
  for procs in 1 $(seq 1 1 4); do
    echo -n "${size}," >> $datafile
    echo -n "${procs},">> $datafile
    (mpirun -np $procs --map-by socket parallel.x -r -f "initial_${size}.pgm" -e 0 -n 50 -s 0 -k $size) >>$datafile
    (mpirun -np $procs --map-by socket parallel.x -r -f "initial_${size}.pgm" -e 1 -n 50 -s 0 -k $size) >>$datafile
    
    truncate -s -1 $datafile  # removing the last comma in the line
    echo >> $datafile # printing the new line in the csv file
    
  done
done

## Moving the result file into the right folder
mv $datafile $loc

module purge
