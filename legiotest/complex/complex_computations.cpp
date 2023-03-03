#include "mpi.h"
#ifdef MPICH
#include "mpi_proto.h"
#else
#include "mpi-ext.h"
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
    MPI_Comm world_comm, split_comm;
    int rank, size;
    int *bufferA, *bufferB;
    int* temp;
    char* str;
    MPI_Win winA, winB, winC, winD;
    MPI_File fileA, fileB, fileC, fileD;

    MPI_Init(&argc, &argv);

    MPI_Comm_dup(MPI_COMM_WORLD, &world_comm);
    MPI_Comm_rank(world_comm, &rank);
    MPI_Comm_size(world_comm, &size);
    MPI_Comm_split(world_comm, (rank >= size / 2), 0, &split_comm);

    bufferA = (int*)malloc(sizeof(int) * size);
    bufferB = (int*)malloc(sizeof(int) * size);
    temp = (int*)malloc(sizeof(int) * size);
    str = (char*)malloc(sizeof(char) * size * 2);

    for (int i = 0; i < size; i++)
    {
        bufferA[i] = i;
        bufferB[i] = 2 * i;
        str[i] = 2 * i;
        str[i + 1] = 2 * i + 1;
    }

    MPI_Win_create(bufferA, size * sizeof(int), sizeof(int), MPI_INFO_NULL, world_comm, &winA);
    MPI_Win_create(bufferB, size * sizeof(int), sizeof(int), MPI_INFO_NULL, world_comm, &winB);
    MPI_Win_create(bufferA, size * sizeof(int), sizeof(int), MPI_INFO_NULL, split_comm, &winC);
    MPI_Win_create(bufferB, size * sizeof(int), sizeof(int), MPI_INFO_NULL, split_comm, &winD);

    MPI_File_open(world_comm, "A.txt", MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE,
                  MPI_INFO_NULL, &fileA);
    MPI_File_open(world_comm, "B.txt", MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE,
                  MPI_INFO_NULL, &fileB);
    MPI_File_open(split_comm, "A.txt", MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE,
                  MPI_INFO_NULL, &fileC);
    MPI_File_open(split_comm, "B.txt", MPI_MODE_RDWR | MPI_MODE_CREATE | MPI_MODE_DELETE_ON_CLOSE,
                  MPI_INFO_NULL, &fileD);

    MPI_Gather(&(bufferB[5]), 1, MPI_INT, temp, 1, MPI_INT, 0, world_comm);
    MPI_Bcast(temp, size, MPI_INT, 0, split_comm);

    MPI_Win_fence(0, winA);
    MPI_Put(temp, size, MPI_INT, (rank + 1) % size, 0, size, MPI_INT, winA);
    MPI_Win_fence(0, winA);

    if (rank == 0)
    {
        printf("rank %d 1st phase: ", rank);
        for (int i = 0; i < size; i++)
            printf("%d ", bufferA[i]);
        printf("\n");
        int i = 1;
        MPI_Send(&i, 1, MPI_INT, (rank + 1) % size, 1, world_comm);
        MPI_Recv(&i, 1, MPI_INT, (rank - 1 + size) % size, 1, world_comm, MPI_STATUS_IGNORE);
    }
    else
    {
        int i = 1;
        MPI_Recv(&i, 1, MPI_INT, (rank - 1 + size) % size, 1, world_comm, MPI_STATUS_IGNORE);
        printf("rank %d 1st phase: ", rank);
        for (int i = 0; i < size; i++)
            printf("%d ", bufferA[i]);
        printf("\n");
        MPI_Send(&i, 1, MPI_INT, (rank + 1) % size, 1, world_comm);
    }

    // MPI_File_write_ordered(fileA, str, size * 2, MPI_CHAR, MPI_STATUS_IGNORE);
    //...

    MPI_Finalize();

    return MPI_SUCCESS;
}
