#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <vector>
#include "mpi.h"

extern "C" {
#include "legio.h"
}

#include "mpi-ext.h"

#define MULT 1000

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Group group;
    MPI_Comm fix, duplicate;
    MPI_Comm bogus[MULT];
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Comm_group(MPI_COMM_WORLD, &group);
    MPI_Comm_dup(MPI_COMM_WORLD, &duplicate);

    FILE* file_p;

    if (argc < 2)
    {
        if (rank == 0)
            printf("Provide the fail size as a parameter\n");
        return 0;
    }
    int fail_size = atoi(argv[1]);
    if (fail_size > size - 1)
        fail_size = size - 1;
    int have_to_fail;
    if (rank == 0)
    {
        srand(time(NULL));
        std::vector<int> fails;
        while (fails.size() < fail_size)
        {
            int to_fail = rand() % size;
            if (std::find(fails.begin(), fails.end(), to_fail) == fails.end())
                fails.push_back(to_fail);
        }
        int* fail_array = (int*)malloc(sizeof(int) * size);
        for (int i = 0; i < size; i++)
            fail_array[i] = (std::find(fails.begin(), fails.end(), i) != fails.end());
        MPI_Scatter(fail_array, 1, MPI_INT, &have_to_fail, 1, MPI_INT, 0, MPI_COMM_WORLD);
        free(fail_array);
    }
    else
        MPI_Scatter(NULL, 0, MPI_INT, &have_to_fail, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // printf("I'm %d, have_to_fail: %d\n", rank, have_to_fail);
    if (have_to_fail)
    {
        printf("Rank %d, suiciding\n", rank);
        raise(SIGINT);
    }
    else
    {
        double start = MPI_Wtime();
        for (int i = 0; i < MULT; i++)
            MPI_Comm_create_from_group(group, "TAG", MPI_INFO_NULL, MPI_ERRORS_RETURN, &(bogus[i]));
        double end = MPI_Wtime();

        int rank_inside;
        MPI_Comm_rank(bogus[0], &rank_inside);
        double recv_buf, recv_buf2, recv_buf3;
        double send_buf = end - start;
        PMPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 0, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf2, 1, MPI_DOUBLE, MPI_MIN, 0, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf3, 1, MPI_DOUBLE, MPI_MAX, 0, bogus[0]);
        if (rank_inside == 0)
        {
            FILE* file_p = fopen("output.csv", "a");
            recv_buf /= (size - fail_size);
            fprintf(file_p, "%d, %d, %s, %f, %f, %f\n", fail_size, size, "from_group", recv_buf,
                    recv_buf2, recv_buf3);
            fclose(file_p);
        }

        for (int i = 1; i < MULT; i++)
            PMPI_Comm_free(&(bogus[i]));
        PMPI_Barrier(bogus[0]);
        PMPI_Comm_free(&(bogus[0]));

        start = MPI_Wtime();
        for (int i = 0; i < MULT; i++)
            MPIX_Comm_shrink(MPI_COMM_WORLD, &(bogus[i]));
        end = MPI_Wtime();

        MPI_Comm_rank(bogus[0], &rank_inside);
        send_buf = end - start;
        PMPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 0, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf2, 1, MPI_DOUBLE, MPI_MIN, 0, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf3, 1, MPI_DOUBLE, MPI_MAX, 0, bogus[0]);
        if (rank_inside == 0)
        {
            FILE* file_p = fopen("output.csv", "a");
            recv_buf /= (size - fail_size);
            fprintf(file_p, "%d, %d, %s, %f, %f, %f\n", fail_size, size, "shrink", recv_buf,
                    recv_buf2, recv_buf3);
            fclose(file_p);
        }

        for (int i = 1; i < MULT; i++)
            PMPI_Comm_free(&(bogus[i]));
        PMPI_Barrier(bogus[0]);
        PMPI_Comm_free(&(bogus[0]));

        int flag = rank % 2;

        start = MPI_Wtime();
        for (int i = 0; i < MULT; i++)
            MPIX_Comm_agree(MPI_COMM_WORLD, &flag);
        end = MPI_Wtime();
        MPIX_Comm_shrink(MPI_COMM_WORLD, &(bogus[0]));

        MPI_Comm_rank(bogus[0], &rank_inside);
        send_buf = end - start;
        PMPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 0, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf2, 1, MPI_DOUBLE, MPI_MIN, 0, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf3, 1, MPI_DOUBLE, MPI_MAX, 0, bogus[0]);
        if (rank_inside == 0)
        {
            FILE* file_p = fopen("output.csv", "a");
            recv_buf /= (size - fail_size);
            fprintf(file_p, "%d, %d, %s, %f, %f, %f\n", fail_size, size, "agree", recv_buf,
                    recv_buf2, recv_buf3);
            fclose(file_p);
        }

        flag = rank % 2;
        start = MPI_Wtime();
        for (int i = 0; i < MULT; i++)
        {
            MPIX_Comm_agree_group(MPI_COMM_WORLD, group, &flag);
        }
        end = MPI_Wtime();

        MPI_Comm_rank(bogus[0], &rank_inside);
        send_buf = end - start;
        PMPI_Reduce(&send_buf, &recv_buf, 1, MPI_DOUBLE, MPI_SUM, 0, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf2, 1, MPI_DOUBLE, MPI_MIN, 0, bogus[0]);
        PMPI_Reduce(&send_buf, &recv_buf3, 1, MPI_DOUBLE, MPI_MAX, 0, bogus[0]);
        if (rank_inside == 0)
        {
            FILE* file_p = fopen("output.csv", "a");
            recv_buf /= (size - fail_size);
            fprintf(file_p, "%d, %d, %s, %f, %f, %f\n", fail_size, size, "custom_agree", recv_buf,
                    recv_buf2, recv_buf3);
            fclose(file_p);
        }

        for (int i = 0; i < MULT; i++)
            PMPI_Comm_free(&(bogus[i]));
    }
    MPI_Finalize();
    return 0;
}