#! /bin/sh
mpirun --hostfile hostfile -np 1 parking 6 3 : -np 4 coche : -np 2 camion
