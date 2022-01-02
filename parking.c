#include "mpi.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*
#define COCHE 1
#define CAMION 2
*/

#define ENTRAR -1
#define SALIR -2
#define COCHES 10
#define CAMIONES 20 

/* variables y declaraciones de metodos y tipos de datos globales */
/*
mensaje[0] - id del proceso
mensaje[1] - accion que realiza el vehiculo: entrar/salir 
mensaje[2] - palnta
mensaje[3] - plaza 
*/
int mensaje[4] = {0,0,0,0};
/*
respuesta[0] - planta
respuesta[1] - plaza, en el caso de camion se indica plazasLibresCamion[0] ya que plazasLibresCamion[1] siempre sera la contigua, es decir +1
*/
int respuesta[2] = {-1,-1};
int plantaLibre = -1;
int plazaLibre = -1;
int plazasLibresCamion[2] = {-1,-1};
int contadorColaCoches = 0;
int contadorColaCamiones = 0;
int *colaDeEsperaCoches;    /* array dinamico que funciona como cola de espera para coches*/
int *colaDeEsperaCamiones;  /* array dinamico que funciona como cola de espera para camiones */

/* definicion de cabeceras de funciones auxiliares */
void inicializarParking(int plantas, int plazas);
void imprimirMatrizParking();
void buscarPlazaLibre();
void buscarPlazaLibreCamion();
void aparcarCoche(int id, int planta, int plaza);
void aparcarCamion(int id, int planta, int plaza1, int plaza2);
void salirCoche(int id, int planta, int plaza);
void salirCamion(int id, int planta, int plaza1, int plaza2);
int obtenerPrimeroCola(int *cola);
void imprimirColaDeEspera(char *nombreCola, int *cola);
void comprobarColaDeEspera();
void liberarRecursos();

typedef struct
{
    int **matriz;
    int plantas;
    int plazas;
    int plazas_libres;
} Parking;

Parking parking;

/* codigo */

int main(int argc, char **argv)
{
    /* rank - numero de proceso */
    /* nprocs - numero de procesos en total */
    int plazas, plantas, rank, nprocs, id, grupo, m1, m2, m3;
    MPI_Status status;

    if (argc!=3)
    {
        printf("Error, numero incorrecto de argumentos pasados al programa\n");
        return 1;
    } 
    /* si ha pasado el primer control de ejecucion se obtienen plazas y plantas */

    plazas = atoi(argv[1]);
    plantas = atoi(argv[2]);

    if (plantas<1)
    {
        printf("Error, numero minimo de plantas = 1\n");
        return 1;
    }
    if (plazas<1)
    {
        printf("Error, numero minimo de plazas = 1\n");
        return 1;
    }

    /* inicializacion de mpi */
    MPI_Init(&argc, &argv);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    inicializarParking(plantas, plazas);

    printf("Proceso parking ejecutandose, soy el proceso %d de %d procesos\n",rank,nprocs);
    
    while (1)
    {   
        /* recepcion de mensaje */
        MPI_Recv(&mensaje,4,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
        id = mensaje[0];
        m1 = mensaje[1];
        m2 = mensaje[2];
        m3 = mensaje[3];
        grupo = status.MPI_TAG;
        if (m1==ENTRAR)
        {
            switch (grupo)
            {
            case COCHES:
                /* comprobando si hay al menos una plaza libre */
                if (parking.plazas_libres>0)    
                {
                    buscarPlazaLibre();
                    aparcarCoche(id,plantaLibre,plazaLibre);
                    
                } else {
                    printf("ENTRADA: No quedan plazas libres, coche id: %d no puede aparcar\n",id);
                    colaDeEsperaCoches[contadorColaCoches] = id; /* se guarda el id del proceso a la espera */
                    contadorColaCoches++;
                }
                break;
            case CAMIONES:
                buscarPlazaLibreCamion();
                if (plazasLibresCamion[0]>=0 && plazasLibresCamion[1]>=0)
                {
                    aparcarCamion(id,plantaLibre,plazasLibresCamion[0],plazasLibresCamion[1]);
                } else {
                    printf("ENTRADA: No quedan plazas libres, camion id: %d no puede aparcar\n",id);
                    colaDeEsperaCamiones[contadorColaCamiones] = id;
                    contadorColaCamiones++;
                }
                break;
            }
        }

        if (m1==SALIR)
        {
            switch (grupo)
            {
            case COCHES:
                salirCoche(id, m2, m3);
                comprobarColaDeEspera();
                break;
            case CAMIONES:
                salirCamion(id,m2,m3,(m3+1));
                comprobarColaDeEspera();
                break;
            }
        }
        sleep(rand()%11);
    }

    MPI_Finalize();
    liberarRecursos();
    return 0;
}

/* funciones auxiliares */
void inicializarParking(int plantas, int plazas){
    /* Parking - cada fila es una Planta, cada posicion de fila es una Plaza */
    int i, j;
    parking.plantas = plantas;
    parking.plazas = plazas;
    parking.plazas_libres = plantas*plazas;
    
    /* reserva de memoria para cola de espera */
    colaDeEsperaCoches = (int*) malloc(sizeof(int)*parking.plazas_libres);
    colaDeEsperaCamiones = (int*) malloc(sizeof(int)*parking.plazas_libres);
    /* se inicializa la cola de espera */

    /* reserva de memoria para parking y con sus plantas */
    parking.matriz = (int **) malloc(sizeof(int*)*plantas);

    for (i = 0; i < plantas; i++)
    {
        parking.matriz[i] = (int*) malloc(sizeof(int)*plazas);
    }

    /* se asignan valores a cada Plaza del parking */
    for ( i = 0; i < plantas; i++)
    {   
        for ( j = 0; j < plazas; j++)
        {
            parking.matriz[i][j] = 0;
        }
    }
    printf("Parking inicializado\n");
    printf("PARKING: Total plazas: %d\n",parking.plazas_libres);
    imprimirMatrizParking();
}

void imprimirMatrizParking(){
    int i, j;
    printf("PARKING:");
    for ( i = 0; i < parking.plantas; i++)
    {   
        if (i==0)
        {
            printf(" Planta %d:",i);
        } else {
            printf("\t Planta %d:",i);
        }
        
        for ( j = 0; j < parking.plazas; j++)
        {
            printf(" [%d]",parking.matriz[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

void buscarPlazaLibre(){
    int i, j;
    for (i = 0; i < parking.plantas; i++)
    {
        for (j = 0; j < parking.plazas; j++)
        {
            if (parking.matriz[i][j] == 0)
            {
                plantaLibre = i;
                plazaLibre = j;
                return;
            }
        }
    }
}

void buscarPlazaLibreCamion(){
    int i, j;
    for ( i = 0; i < parking.plantas; i++)
    {
        /* 
        dado que el camion ocupa 2 plazas
        no tiene sentido comprobar las ultimas plazas de cada planta ya que ahi no cabe un camion
        de este modo se puede evitar que un camion aparque entre dos plantas
        segun esta logica la ultima plaza solo es valida si contigua(la penultima) tambien el valida
        */
        for ( j = 0; j < parking.plazas-1; j++) 
        {
            if (parking.matriz[i][j]==0 && parking.matriz[i][j+1]==0)
            {
                plantaLibre=i;
                plazasLibresCamion[0]=j;
                plazasLibresCamion[1]=j+1;
                return;
            }
        }
    }
}
void aparcarCoche(int id, int planta, int plaza){
    parking.matriz[planta][plaza] = id;
    parking.plazas_libres--;
    printf("ENTRADA: Coche %d aparca en Planta: %d, Plaza: %d. Plazas libres: %d\n", id, planta, plaza, parking.plazas_libres);
    imprimirMatrizParking();
    respuesta[0] = planta;
    respuesta[1] = plaza;
    MPI_Send(&respuesta, 2, MPI_INT, id, COCHES, MPI_COMM_WORLD);
}

void aparcarCamion(int id, int planta, int plaza1, int plaza2){
    parking.matriz[planta][plaza1]=id;
    parking.matriz[planta][plaza2]=id;
    parking.plazas_libres-=2;
    printf("ENTRADA: Camion %d aparca en Planta: %d, Plazas: %d y %d. Plazas libres: %d\n", id, planta, plaza1, plaza2, parking.plazas_libres);
    imprimirMatrizParking();
    respuesta[0] = planta;
    respuesta[1] = plaza1;
    MPI_Send(&respuesta, 2, MPI_INT, id, CAMIONES, MPI_COMM_WORLD);
}

void salirCoche(int id, int planta, int plaza){
    parking.matriz[planta][plaza] = 0;
    parking.plazas_libres++;
    printf("SALIDA: Coche %d saliendo. Plazas libres %d\n", id, parking.plazas_libres);
    imprimirMatrizParking();
    respuesta[0] = -2;
    respuesta[1] = -2;
    printf("PARKING: contador de cola de espera: %d\n", contadorColaCoches);
    if (contadorColaCoches>0)
    {
        imprimirColaDeEspera("COCHES",colaDeEsperaCoches);
    }
    if (contadorColaCamiones>0)
    {
        imprimirColaDeEspera("CAMIONES",colaDeEsperaCamiones);
    }
    MPI_Send(&respuesta, 2, MPI_INT, id, COCHES, MPI_COMM_WORLD);
}

void salirCamion(int id, int planta, int plaza1, int plaza2){
    parking.matriz[planta][plaza1]=0;
    parking.matriz[planta][plaza2]=0;
    parking.plazas_libres+=2;
    printf("SALIDA: Camion %d saliendo. Plazas libres %d\n", id, parking.plazas_libres);
    imprimirMatrizParking();
    respuesta[0] = -2;
    respuesta[1] = -2;
    if (contadorColaCoches>0)
    {
        imprimirColaDeEspera("COCHES",colaDeEsperaCoches);
    }
    if (contadorColaCamiones>0)
    {
        imprimirColaDeEspera("CAMIONES",colaDeEsperaCamiones);
    }
    MPI_Send(&respuesta, 2, MPI_INT, id, CAMIONES, MPI_COMM_WORLD);
}

int obtenerPrimeroCola(int *cola){
    int i;
    for ( i = 0; i < (parking.plantas*parking.plazas); i++)
    {
        if (cola[i]!=-1)
        {
            return i;
        }
    }
    return 0;
}

void imprimirColaDeEspera(char *nombreCola,int *cola){
    int i;
    printf("COLA DE ESPERA %s: ",nombreCola);
    for ( i = 0; i < (parking.plantas*parking.plazas)-1; i++)
    {
        if (cola[i]!=-1)
        {
            printf("[%d] ",cola[i]);
        }
    }
    printf("\n");
}

void comprobarColaDeEspera(){
    if (contadorColaCoches > 0)
    {
        int primero = obtenerPrimeroCola(colaDeEsperaCoches);
        int idEsperando = colaDeEsperaCoches[primero];
        printf("PARKING: Coche %d entrando desde cola de espera\n", idEsperando);
        buscarPlazaLibre();
        aparcarCoche(idEsperando, plantaLibre, plazaLibre);
        contadorColaCoches--;
        colaDeEsperaCoches[primero]=-1;   /* se libera el hueco en la cola de espera */
    }
    if (contadorColaCamiones > 0)
    {
        /* si hay plazas libres para camion, camion aparca */
        buscarPlazaLibreCamion();
        if (plazasLibresCamion[0] >= 0 && plazasLibresCamion[1] >= 0)
        {
            int primero = obtenerPrimeroCola(colaDeEsperaCamiones);
            int idEsperando = colaDeEsperaCamiones[primero];
            printf("PARKING: Camion %d entrando desde cola de espera\n", idEsperando);
            aparcarCamion(idEsperando, plantaLibre, plazasLibresCamion[0], plazasLibresCamion[1]);
            contadorColaCamiones--;
            colaDeEsperaCamiones[primero]=-1;
        }
    }
    
    
}

void liberarRecursos(){
    int i;
    for (i = 0; i < parking.plantas; i++)
    {
        free(parking.matriz[i]);
    }
    free(parking.matriz);
    free(colaDeEsperaCoches);
    free(colaDeEsperaCamiones);
}
