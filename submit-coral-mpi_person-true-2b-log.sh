#!/bin/bash
#SBATCH --job-name              person_true-2b-log
#SBATCH --partition             mpi-short
#SBATCH --nodes                 1
#SBATCH --tasks-per-node        1
#SBATCH --time                  24:00:00
#SBATCH --cpus-per-task         40
#SBATCH --mem                   50G
#SBATCH --output                person_true-2b-log.%j.out
#SBATCH --error                 person_true-2b-log.%j.err
#SBATCH --mail-type             ALL
#SBATCH --mail-user             ou.yaozhong@connect.um.edu.mo

source /etc/profile
source /etc/profile.d/modules.sh

#Adding modules
module add openmpi/4.0.0/gcc/4.8.5

ulimit -s unlimited

#Your program starts here
module load matlab/R2019a
cd ~/pascalraw-true-2b-log-person/voc-release5-raw
matlab -nodisplay -r "addpath(genpath('~/pascalraw-true-2b-log-person')); pascal('person',3);" -logfile ~/pascalraw-true-2b-log-person/log/person_true-2b-log.log
