#!/bin/bash

# Do not automatically requeue the job if there is a system failure
#SBATCH --no-requeue

#The job has exclusive acces to the requested node
#SBATCH --exclusive

#Maximum time
#SBATCH --time=2:00:00

# Loading the input variables
partition=$1 # $1 is the first argument passed to the sbatch file
architecture=$2
fixed=$3
policy=$4
n_thread=$5

module load architecture/${architecture} 
module load mkl/latest
module load openBLAS/0.3.23-omp

export LD_LIBRARY_PATH=/:$LD_LIBRARY_PATH

# Setting the right library path for blis
if [ "${partition}" == "EPYC" ]; then
  export LD_LIBRARY_PATH=/u/dssc/gsarne00/myblis/lib:$LD_LIBRARY_PATH
  blislib=myblis
else
  export LD_LIBRARY_PATH=/u/dssc/gsarne00/myblis2/lib:$LD_LIBRARY_PATH
  blislib=myblis2
fi

# Running make to compile the executables in the right folder
location="${partition}/fixed_${fixed}/${policy}"
make clean loc=$location lib=$blislib
make cpu loc=$location lib=$blislib

# Changing the location
cd $location

# Preparing the csv files
echo "matrix_size,num_threads,openblas_float,openblas_double,mkl_float,mkl_double,blis_float,blis_double" > results.csv 


# Running the code
# we will repeat the run 4 times and saving the mean of the GFLOPS
echo "running the code"

# fixed cores, scale on size
if [ "${fixed}" == "cores" ]; then

  
  export OMP_NUM_THREADS="${n_thread}"
  for MSIZE in {2000..20000..2000}; do
    echo -n "${MSIZE},${OMP_NUM_THREADS}," >> results.csv
    
    for lib in openblas mkl blis; do
      for prec in float double; do
        
        if ! [ -f ./${lib}_${prec}.x ]; then
          echo "executable not found"
        fi
        
        # Running the program 4 times to obtain the average of gflops
        GFLOP1=$(./${lib}_${prec}.x $MSIZE $MSIZE $MSIZE | grep GFLOPS | cut -f3- | grep -Eo '^[0-9]+' | tr -d '\n') 
        # this allows to obtain only the resulting gigaflops
        GFLOP2=$(./${lib}_${prec}.x $MSIZE $MSIZE $MSIZE | grep GFLOPS | cut -f3- | grep -Eo '^[0-9]+' | tr -d '\n')
        GFLOP3=$(./${lib}_${prec}.x $MSIZE $MSIZE $MSIZE | grep GFLOPS | cut -f3- | grep -Eo '^[0-9]+' | tr -d '\n')
        GFLOP4=$(./${lib}_${prec}.x $MSIZE $MSIZE $MSIZE | grep GFLOPS | cut -f3- | grep -Eo '^[0-9]+' | tr -d '\n')
        # Summing the results and printing the average in the csv file
        g1=$((GFLOP1))
        g2=$((GFLOP2))
        g3=$((GFLOP3))
        g4=$((GFLOP4))
        x=$(($g1 + $g2 + $g3 + $g4))
        echo -n "$(($x / 4))," >> results.csv 
        
        echo "Using ${lib} ${prec} with ${n_thread} threads on a matrix of size ${MSIZE} the average GFLOPS are $(($x / 4))"
        
      done
    done
    truncate -s -1 results.csv  # removing the last comma in the line
    echo >> results.csv # printing the new line in the csv file
    
  done

# fixed size, scale on cores
elif [ "${fixed}" == "size" ]; then
  MSIZE=10000
  # There are different cycles depending on the partition, on THIN there will be 12 runs, while in EPYC there will be 8
  start=$((${n_thread} / 8))
  step=$((${n_thread} / 8))
  
  for (( OMP_NUM_THREADS=$start; OMP_NUM_THREADS <= $n_thread; OMP_NUM_THREADS+=$step )); do
    export OMP_NUM_THREADS
    echo -n "${MSIZE},${OMP_NUM_THREADS}," >> results.csv

    for lib in openblas mkl blis; do
      for prec in float double; do
      
        if ! [ -f ./${lib}_${prec}.x ]; then
          echo "executable not found"
        fi
        
        # Running the program 4 times to obtain the average of gflops
        GFLOP1=$(./${lib}_${prec}.x $MSIZE $MSIZE $MSIZE | grep GFLOPS | cut -f3- | grep -Eo '^[0-9]+' | tr -d '\n') 
        # this allows to obtain only the resulting gigaflops
        GFLOP2=$(./${lib}_${prec}.x $MSIZE $MSIZE $MSIZE | grep GFLOPS | cut -f3- | grep -Eo '^[0-9]+' | tr -d '\n')
        GFLOP3=$(./${lib}_${prec}.x $MSIZE $MSIZE $MSIZE | grep GFLOPS | cut -f3- | grep -Eo '^[0-9]+' | tr -d '\n')
        GFLOP4=$(./${lib}_${prec}.x $MSIZE $MSIZE $MSIZE | grep GFLOPS | cut -f3- | grep -Eo '^[0-9]+' | tr -d '\n')
        # Summing the results and printing the average in the csv file
        g1=$((GFLOP1))
        g2=$((GFLOP2))
        g3=$((GFLOP3))
        g4=$((GFLOP4))
        x=$(($g1 + $g2 + $g3 + $g4))
        echo -n "$(($x / 4))," >> results.csv 
        
        echo "Using ${lib} ${prec} with ${OMP_NUM_THREADS} threads on a matrix of size ${MSIZE} the average GFLOPS are $(($x / 4))"
        
      done
    done
    truncate -s -1 results.csv  # removing the last comma in the line
    echo >> results.csv # printing the new line in the csv file
    
  done
fi

echo "Job ${partition}_fixed_${fixed}_${policy} ended"

cd ../../..
make clean loc=$location lib=$blislib
module purge
