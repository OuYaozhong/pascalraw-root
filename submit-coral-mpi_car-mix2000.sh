#!/bin/bash
#SBATCH --job-name              car_mix2000
#SBATCH --partition             mpi-normal
#SBATCH --nodes                 1
#SBATCH --tasks-per-node        1
#SBATCH --time                  72:00:00
#SBATCH --cpus-per-task         40
#SBATCH --mem                   50G
#SBATCH --output                car_mix2000.%j.out
#SBATCH --error                 car_mix2000.%j.err
#SBATCH --mail-type             ALL
#SBATCH --mail-user             ou.yaozhong@connect.um.edu.mo

source /etc/profile
source /etc/profile.d/modules.sh

#Adding modules
module add openmpi/4.0.0/gcc/4.8.5

ulimit -s unlimited

#Your program starts here
module load matlab/R2019a
cd ~/pascalraw-mix2000-car/voc-release5-raw
matlab -nodisplay -r "addpath(genpath('~/pascalraw-mix2000-car')); pascal('car',3);" -logfile ~/pascalraw-mix2000-car/log/car_mix2000.log
