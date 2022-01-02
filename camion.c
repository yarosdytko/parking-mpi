#include "mpi.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define ENTRAR -1
#define SALIR -2
#define COCHES 10
#define CAMIONES 20 

int main(int argc, char *argv[])
{
    int rank, nprocs;
    int mensaje[4];
    int respuesta[2];
    MPI_Status status;

    /* inicializacion de algunas variables */
    respuesta[0]=-1;    /* planta */
    respuesta[1]=-1;    /* plaza */
    
    /* inicializacion de mpi */
    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    //printf("Proceso camion ejecutandose, soy el proceso %d de %d procesos\n",rank,nprocs);

    while (1)
    {   
        mensaje[0] = rank;      /* id del proceso */
        mensaje[1] = ENTRAR;    /* accion que realiza el coche, por defecto entrar / numero de planta si ha aparcado*/
        mensaje[2] = -100;    /* tipo de vehiculo / numero de plaza si ha aparcado */
        mensaje[3] = -100;      /* solo se usa cuando para salir, en este caso no se usa */
        /* vehiculo quiere entrar en el parking */
        MPI_Send(&mensaje,4,MPI_INT,0,CAMIONES,MPI_COMM_WORLD);
        /* recibiendo respuesta */
        MPI_Recv(&respuesta,2,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD,&status);

        sleep(rand()%11);  /* tiempo de espera aleatorio entre 0 y 10 */
        /* si en el mensaje de respuesta se ha recibido un numero de plaza entoces el coche ha aparcado */
        if (respuesta[0]>=0)    
        {
            mensaje[0] = rank;
            mensaje[1] = SALIR;
            mensaje[2] = respuesta[0];
            mensaje[3] = respuesta[1];
            MPI_Send(&mensaje,4,MPI_INT,0,CAMIONES,MPI_COMM_WORLD);    /* envio mensaje de que quiero salir del parking */
            MPI_Recv(&respuesta,2,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD,&status); /* espero confirmacion */
        }
    }
    MPI_Finalize();
    return 0;
}
