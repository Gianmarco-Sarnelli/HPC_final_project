#!/bin/bash

# Iterating run of the same sbatch file with different options
# The options are:
# Name of the job that will be displayed in the queue: --job-name= "${partition}_fixed_${fixed}_${policy}"
# Partition that will be used: --partition=EPYC, THIN
# Policy : close, spread
# Number of threads for each task: --cpus-per-task


export OMP_PLACES=cores  # Each place corresponds to a single core (having one or more hardware threads) on the target machine.

export FREE_RUN=1  # Global variable to run all the jobs in order

for partition in EPYC THIN; do

  # Choosing the right architecture for the partition
  if [ "$partition" == "EPYC" ];then
    architecture=AMD
  else
    architecture=Intel
  fi
  
  # Choosing the nodelist
  if [ "$partition" == "EPYC" ];then
    nodelist=epyc[001-008]
  else
    nodelist=thin[001-010]
  fi


  for fixed in cores size; do
    
    #Selectiong the right number of processes to run
    if [ "$partition" == "EPYC" ];then
      n_thread=64
    else
      n_thread=12
    fi
      
    export OMP_NUM_THREADS=$n_thread
  
    for policy in close spread; do
      
      export OMP_PROC_BIND=$policy  # Instructs the execution environment to assign the threads in the team to the places that are close to the place of the parent thread
      
      # Running the sbatch file
      sbatch --job-name="${partition}_fixed_${fixed}_${policy}" --partition="${partition}" --account dssc -N 1 -n 1 --cpus-per-task="${n_thread}" --output="${partition}_fixed_${fixed}_${policy}.out" --nodelist="${nodelist}" my_job.sh "${partition}" "${architecture}" "${fixed}" "${policy}" "${n_thread}"
        
      echo "job ${partition}_fixed_${fixed}_${policy} is running"
       
    done # End cycle on policy
    
  done # End cycle on type of scaling
  
done # End cycle on partition
