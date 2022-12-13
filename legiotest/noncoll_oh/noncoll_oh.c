#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define MULT 1000

int print_to_file(double, int, int, FILE*, char*);

int main(int argc, char** argv)
{
    int rank, size;
    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    FILE* file_p;

    if (rank == 1)
    {
        file_p = fopen("output.csv", "a");
    }
    if (argc < 2)
    {
        if (rank == 1)
            printf("Provide the group size as a parameter\n");
        return 0;
    }
    int group_size = atoi(argv[1]);
    if (group_size > size)
        group_size = size;
    int* group_array = malloc(sizeof(int) * group_size);
    for (int i = 0; i < group_size; i++)
        group_array[i] = i;
    MPI_Group group, total_group;
    MPI_Comm_group(MPI_COMM_WORLD, &total_group);
    MPI_Group_incl(total_group, group_size, group_array, &group);
    MPI_Comm bogus[MULT];
    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();
    for (int i = 0; i < MULT; i++)
        if (rank < group_size)
            MPI_Comm_create_group(MPI_COMM_WORLD, group, 0, &(bogus[i]));
    double end = MPI_Wtime();
    if (rank < group_size)
    {
        double recv_buf, recv_buf2, recv_buf3;
        double send_buf = end - start;
        MPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 1, bogus[0]);
        MPI_Reduce(&send_buf, &recv_buf2, 1, MPI_DOUBLE, MPI_MIN, 1, bogus[0]);
        MPI_Reduce(&send_buf, &recv_buf3, 1, MPI_DOUBLE, MPI_MAX, 1, bogus[0]);
        if (rank == 1)
        {
            recv_buf /= group_size;
            fprintf(file_p, "%d, %d, %s, %f, %f, %f\n", group_size, size, "mpi_group", recv_buf,
                    recv_buf2, recv_buf3);
        }
    }
    for (int i = 0; i < MULT; i++)
        if (rank < group_size)
            MPI_Comm_free(&(bogus[i]));
    if (rank == 1)
        printf("mpi_group done\n\n\n");
    MPI_Barrier(MPI_COMM_WORLD);

    start = MPI_Wtime();
    for (int i = 0; i < MULT; i++)
        if (rank < group_size)
            PMPI_Comm_create_group(MPI_COMM_WORLD, group, 0, &(bogus[i]));
    end = MPI_Wtime();
    if (rank < group_size)
    {
        double recv_buf, recv_buf2, recv_buf3;
        double send_buf = end - start;
        PMPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 1, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf2, 1, MPI_DOUBLE, MPI_MIN, 1, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf3, 1, MPI_DOUBLE, MPI_MAX, 1, bogus[0]);
        if (rank == 1)
        {
            recv_buf /= group_size;
            fprintf(file_p, "%d, %d, %s, %f, %f, %f\n", group_size, size, "pmpi_group", recv_buf,
                    recv_buf2, recv_buf3);
        }
    }
    for (int i = 0; i < MULT; i++)
        if (rank < group_size)
            PMPI_Comm_free(&(bogus[i]));

    if (rank == 1)
        printf("pmpi_group done\n\n\n");
    MPI_Barrier(MPI_COMM_WORLD);

    start = MPI_Wtime();
    for (int i = 0; i < MULT; i++)
        if (rank < group_size)
            MPI_Comm_create_from_group(group, "A", MPI_INFO_NULL, MPI_ERRORS_RETURN, &(bogus[i]));
    end = MPI_Wtime();
    if (rank < group_size)
    {
        double recv_buf, recv_buf2, recv_buf3;
        double send_buf = end - start;
        PMPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 1, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf2, 1, MPI_DOUBLE, MPI_MIN, 1, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf3, 1, MPI_DOUBLE, MPI_MAX, 1, bogus[0]);
        if (rank == 1)
        {
            recv_buf /= group_size;
            fprintf(file_p, "%d, %d, %s, %f, %f, %f\n", group_size, size, "mpi_from_group",
                    recv_buf, recv_buf2, recv_buf3);
        }
    }
    for (int i = 0; i < MULT; i++)
        if (rank < group_size)
            PMPI_Comm_free(&(bogus[i]));

    if (rank == 1)
        printf("mpi_from_group done\n\n\n");
    MPI_Barrier(MPI_COMM_WORLD);

    start = MPI_Wtime();
    for (int i = 0; i < MULT; i++)
        if (rank < group_size)
            PMPI_Comm_create_from_group(group, "A", MPI_INFO_NULL, MPI_ERRORS_RETURN, &(bogus[i]));
    end = MPI_Wtime();
    if (rank < group_size)
    {
        double recv_buf, recv_buf2, recv_buf3;
        double send_buf = end - start;
        PMPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 1, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf2, 1, MPI_DOUBLE, MPI_MIN, 1, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf3, 1, MPI_DOUBLE, MPI_MAX, 1, bogus[0]);
        if (rank == 1)
        {
            recv_buf /= group_size;
            fprintf(file_p, "%d, %d, %s, %f, %f, %f\n", group_size, size, "pmpi_from_group",
                    recv_buf, recv_buf2, recv_buf3);
        }
    }
    for (int i = 0; i < MULT; i++)
        if (rank < group_size)
            PMPI_Comm_free(&(bogus[i]));

    if (rank == 1)
        printf("pmpi_from_group done\n\n\n");
    MPI_Barrier(MPI_COMM_WORLD);
    /*
    start = MPI_Wtime();
    for(int i = 0; i < MULT; i++)
        MPI_Comm_create(MPI_COMM_WORLD, group, &(bogus[i]));
    end = MPI_Wtime();

    if(rank < group_size)
    {
        double recv_buf, recv_buf2, recv_buf3;
        double send_buf = end-start;
        MPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 1, bogus[0]);
        MPI_Reduce(&send_buf, &recv_buf2, 1, MPI_DOUBLE, MPI_MIN, 1, bogus[0]);
        MPI_Reduce(&send_buf, &recv_buf3, 1, MPI_DOUBLE, MPI_MAX, 1, bogus[0]);
        if(rank == 1)
        {
            recv_buf /= group_size;
            fprintf(file_p, "%d, %d, %s, %f, %f, %f\n", group_size, size, "mpi_comm", recv_buf,
    recv_buf2, recv_buf3);
        }
    }

    for(int i = 0; i < MULT; i++)
        if(rank < group_size)
            MPI_Comm_free(&(bogus[i]));

    if(rank == 1)
        printf("mpi_comm done\n\n\n");
    MPI_Barrier(MPI_COMM_WORLD);
    */

    start = MPI_Wtime();
    for (int i = 0; i < MULT; i++)
        PMPI_Comm_create(MPI_COMM_WORLD, group, &(bogus[i]));
    end = MPI_Wtime();

    if (rank < group_size)
    {
        double recv_buf, recv_buf2, recv_buf3;
        double send_buf = end - start;
        PMPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 1, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf2, 1, MPI_DOUBLE, MPI_MIN, 1, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf3, 1, MPI_DOUBLE, MPI_MAX, 1, bogus[0]);
        if (rank == 1)
        {
            recv_buf /= group_size;
            fprintf(file_p, "%d, %d, %s, %f, %f, %f\n", group_size, size, "pmpi_comm", recv_buf,
                    recv_buf2, recv_buf3);
        }
    }

    for (int i = 0; i < MULT; i++)
        if (rank < group_size)
            PMPI_Comm_free(&(bogus[i]));

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}