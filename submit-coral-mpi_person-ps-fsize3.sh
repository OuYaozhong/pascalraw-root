#!/bin/bash
#SBATCH --job-name              person_ps-fsize3
#SBATCH --partition             mpi-normal
#SBATCH --nodes                 1
#SBATCH --tasks-per-node        1
#SBATCH --time                  72:00:00
#SBATCH --cpus-per-task         40
#SBATCH --mem                   50G
#SBATCH --output                person_ps-fsize3.%j.out
#SBATCH --error                 person_ps-fsize3.%j.err
#SBATCH --mail-type             ALL
#SBATCH --mail-user             ou.yaozhong@connect.um.edu.mo

source /etc/profile
source /etc/profile.d/modules.sh

#Adding modules
module add openmpi/4.0.0/gcc/4.8.5

ulimit -s unlimited

#Your program starts here
module load matlab/R2019a
cd ~/pascalraw-ps-size3-person/voc-release5-raw
matlab -nodisplay -r "addpath(genpath('~/pascalraw-ps-size3-person')); pascal('person',3);" -logfile ~/pascalraw-ps-size3-person/log/person_ps-fsize3.log
