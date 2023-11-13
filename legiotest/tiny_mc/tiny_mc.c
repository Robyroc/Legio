char t1[80] = "Tiny Monte Carlo by Scott Prahl (https://omlc.org)";
char t2[80] = "1 W Point Source Heating in Infinite Isotropic Scattering Medium";

#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mpi.h"
#define SHELL_MAX 33

double mu_a = 2;                /* Absorption Coefficient in 1/cm !!non-zero!! */
double mu_s = 20;               /* Reduced Scattering Coefficient in 1/cm */
double microns_per_shell = 100; /* Thickness of spherical shells in microns */
long i, shell, photons = 1000000;
double x, y, z, u, v, w, weight;
double albedo, shells_per_mfp, xi1, xi2, t, heat[SHELL_MAX] = {0.0};

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    srand(time(NULL));
    albedo = mu_s / (mu_s + mu_a);
    shells_per_mfp = 1e4 / microns_per_shell / (mu_a + mu_s);

    int rank, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int fail_size;
    if (argc < 3)
        fail_size = 0;
    else
        fail_size = atoi(argv[2]);
    if (fail_size > size - 2)
        fail_size = size - 2;
    int have_to_fail;
    if (rank == 0)
    {
        srand(time(NULL));
        int* fails = malloc(sizeof(int) * size);
        memset(fails, 0, sizeof(int) * size);
        int fail_count = 0;
        while (fail_count < fail_size)
        {
            int to_fail = (rand() % (size - 1)) + 1;
            if (!fails[to_fail])
            {
                fails[to_fail] = 1;
                fail_count++;
            }
        }
        MPI_Scatter(fails, 1, MPI_INT, &have_to_fail, 1, MPI_INT, 0, MPI_COMM_WORLD);
        free(fails);
    }
    else
        MPI_Scatter(NULL, 0, MPI_INT, &have_to_fail, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // printf("I'm %d, have_to_fail: %d\n", rank, have_to_fail);
    MPI_Barrier(MPI_COMM_WORLD);
    if (have_to_fail)
    {
        printf("Rank %d, suiciding\n", rank);
        raise(SIGINT);
    }
    long photons_per_node =
        (rank == size - 1 ? photons - (photons / size) * (size - 1) : photons / size);

    for (i = 1; i <= photons_per_node; i++)
    {
        x = 0.0;
        y = 0.0;
        z = 0.0; /*launch*/
        u = 0.0;
        v = 0.0;
        w = 1.0;
        weight = 1.0;

        for (;;)
        {
            t = -log((rand() + 1.0) / (RAND_MAX + 1.0)); /*move*/
            x += t * u;
            y += t * v;
            z += t * w;

            shell = sqrt(x * x + y * y + z * z) * shells_per_mfp; /*absorb*/
            if (shell > SHELL_MAX - 1)
                shell = SHELL_MAX - 1;
            heat[shell] += (1.0 - albedo) * weight;
            weight *= albedo;

            for (;;)
            { /*new direction*/
                xi1 = 2.0 * rand() / RAND_MAX - 1.0;
                xi2 = 2.0 * rand() / RAND_MAX - 1.0;
                if ((t = xi1 * xi1 + xi2 * xi2) <= 1)
                    break;
            }
            u = 2.0 * t - 1.0;
            v = xi1 * sqrt((1 - u * u) / t);
            w = xi2 * sqrt((1 - u * u) / t);

            if (weight < 0.001)
            { /*roulette*/
                if (rand() > 0.1 * RAND_MAX)
                    break;
                weight /= 0.1;
            }
        }
    }
    if (rank != 0)
        MPI_Reduce(heat, NULL, SHELL_MAX, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    else
    {
        MPI_Reduce(MPI_IN_PLACE, heat, SHELL_MAX, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
        printf("%s\n%s\n\nScattering = %8.3f/cm\nAbsorption = %8.3f/cm\n", t1, t2, mu_s, mu_a);
        printf("Photons    = %8ld\n\n Radius         Heat\n[microns]     [W/cm^3]\n", photons);
        t = 4 * 3.14159 * pow(microns_per_shell, 3) * photons / 1e12;
        FILE* fp = NULL;
        if (argc > 1)
            fp = fopen(argv[1], "w");
        else
            fp = NULL;
        for (i = 0; i < SHELL_MAX - 1; i++)
        {
            printf("%6.0f    %12.5f\n", i * microns_per_shell,
                   heat[i] / t / (i * i + i + 1.0 / 3.0));
            if (fp != NULL)
                fprintf(fp, "%.0f,%.5f\n", i * microns_per_shell,
                        heat[i] / t / (i * i + i + 1.0 / 3.0));
        }
        printf(" extra    %12.5f\n", heat[SHELL_MAX - 1] / photons);
        if (fp != NULL)
        {
            fprintf(fp, "extra,%.5f\n", heat[SHELL_MAX - 1] / photons);
            fclose(fp);
        }
    }
    MPI_Finalize();
    return 0;
}
