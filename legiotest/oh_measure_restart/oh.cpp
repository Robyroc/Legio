#include "mpi.h"
#include "mpi-ext.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <restart.h>

#define MULT 100

int print_to_file(double, int, int, FILE*, char*);

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int ranks[size];
    
    for (int i = 0; i < size; i++) {
        ranks[i] = i;
    }

    MPI_Comm comms[100];
    for (int i = 0; i < 100; i++) {
        initialize_comm(size, ranks, &comms[i]);
    }

    FILE* file_p;

    double start;
    double end;

    if (!is_respawned()) {
        MPI_Barrier(MPI_COMM_WORLD);
    }

    if(!is_respawned() && rank == 0)
        raise(SIGINT);
    
    start = MPI_Wtime();
    for (int i = 0; i < 100; i++) {
        MPI_Barrier(comms[i]);
    }
    end = MPI_Wtime();

   if (rank == 1) {
      FILE *file_p = fopen("output.csv", "a");
      fseek(file_p, 0, SEEK_END);
      fprintf(file_p, "%f\n", end-start);
      fclose(file_p);
    }


    MPI_Finalize();

    return MPI_SUCCESS;
}
