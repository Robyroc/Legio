#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "complex_comm.h"

extern ComplexComm *cur_complex;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

int MPI_File_open(MPI_Comm comm, char *filename, int amode, MPI_Info info, MPI_File *mpi_fh)
{
    while(1)
    {
        int rc;
        std::function<int(MPI_Comm, MPI_File *)> func;
        if(comm == MPI_COMM_WORLD)
        {
            MPI_Barrier(MPI_COMM_WORLD);
            func = [filename, amode, info] (MPI_Comm c, MPI_File* f) -> int
            {
                int rc = PMPI_File_open(c, filename, amode, info, f);
                MPI_File_set_errhandler(*f, MPI_ERRORS_RETURN);
                return rc;
            };
            rc = func(cur_complex->get_comm(), mpi_fh);
        }
        else
            rc = PMPI_File_open(comm, filename, amode, info, mpi_fh);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(comm, &size);
            PMPI_Comm_rank(comm, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: file opened (error: %s)\n", rank, size, errstr);
        }
        if(comm != MPI_COMM_WORLD)
            return rc;
        else if(rc == MPI_SUCCESS)
        {
            cur_complex->add_structure(*mpi_fh, func);
            return rc;
        }
        else
            replace_comm(cur_complex);
    }
}

int MPI_File_close(MPI_File *mpi_fh)
{
    int flag;
    cur_complex->check_global(*mpi_fh, &flag);
    if(flag)
        cur_complex->remove_structure(*mpi_fh);
    return MPI_SUCCESS;
}

int MPI_File_seek(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    int rc; //, flag;
    //cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);
    /*
    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);
    */
    rc = PMPI_File_seek(translated, offset, whence);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: seek done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_read(MPI_File mpi_fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc; //, flag;
    //cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);
    /*
    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);
    */
    rc = PMPI_File_read(translated, buf, count, datatype, status);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: read done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_write(MPI_File mpi_fh, void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc; //, flag;
    //cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);
    /*
    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);
    */
    rc = PMPI_File_write(translated, buf, count, datatype, status);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: write done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}