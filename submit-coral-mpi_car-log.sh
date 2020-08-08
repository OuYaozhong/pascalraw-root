#!/bin/bash
#SBATCH --job-name              car_log
#SBATCH --partition             mpi-short
#SBATCH --nodes                 1
#SBATCH --tasks-per-node        1
#SBATCH --time                  24:00:00
#SBATCH --cpus-per-task         40
#SBATCH --mem                   50G
#SBATCH --output                car_log.%j.out
#SBATCH --error                 car_log.%j.err
#SBATCH --mail-type             ALL
#SBATCH --mail-user             ou.yaozhong@connect.um.edu.mo

source /etc/profile
source /etc/profile.d/modules.sh

#Adding modules
module add openmpi/4.0.0/gcc/4.8.5

ulimit -s unlimited

#Your program starts here
module load matlab/R2019a
cd ~/pascalraw-log-car/voc-release5-raw
matlab -nodisplay -r "addpath(genpath('~/pascalraw-log-car')); pascal('car',3);" -logfile ~/pascalraw-log-car/log/car_log.log
