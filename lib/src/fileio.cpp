#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "complex_comm.h"
#include <string.h>
#include "multicomm.h"

extern Multicomm *cur_comms;;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;

int MPI_File_open(MPI_Comm comm, const char *filename, int amode, MPI_Info info, MPI_File *mpi_fh)
{
    int consequent_amode = amode;
    int first_amode = amode;
    if(amode & MPI_MODE_EXCL)
    {
        consequent_amode ^= MPI_MODE_EXCL;
    }
    if(amode & MPI_MODE_DELETE_ON_CLOSE)
    {
        consequent_amode ^= MPI_MODE_DELETE_ON_CLOSE;
        first_amode ^= MPI_MODE_DELETE_ON_CLOSE;
        //Find a way to delete files on close only in this case
    }
    if(amode & MPI_MODE_UNIQUE_OPEN)
    {
        consequent_amode ^= MPI_MODE_UNIQUE_OPEN;
        first_amode ^= MPI_MODE_UNIQUE_OPEN;
        //Find a way to keep unique open functionality while allowing more file handlers for same file
    }
    while(1)
    {
        int rc, flag;
        cur_comms->part_of(comm, &flag);
        AdvComm* translated = cur_comms->translate_into_complex(comm);
        std::function<int(MPI_Comm, MPI_File *)> func;
        if(flag)
        {
            MPI_Barrier(translated->get_alias());
            func = [filename, consequent_amode, info] (MPI_Comm c, MPI_File* f) -> int
            {
                int rc = PMPI_File_open(c, filename, consequent_amode, info, f);
                MPI_File_set_errhandler(*f, MPI_ERRORS_RETURN);
                return rc;
            };
            rc = PMPI_File_open(translated->get_comm(), filename, consequent_amode, info, mpi_fh);
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
        if(!flag)
            return rc;
        else if(rc == MPI_SUCCESS)
        {
            bool result = cur_comms->add_file(translated, *mpi_fh, func);
            if(result)
                return rc;
        }
        else
            replace_comm(translated);
    }
}

int MPI_File_close(MPI_File *mpi_fh)
{
    cur_comms->remove_file(mpi_fh);
    return MPI_SUCCESS;
}

int MPI_File_read_at(MPI_File mpi_fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        MPI_File translated = comm->translate_structure(mpi_fh);
        rc = PMPI_File_read_at(translated, offset, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_read_at(mpi_fh, offset, buf, count, datatype, status);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: read_at done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_write_at(MPI_File mpi_fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc; 
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    
    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        MPI_File translated = comm->translate_structure(mpi_fh);
        rc = PMPI_File_write_at(translated, offset, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_write_at(mpi_fh, offset, buf, count, datatype, status);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: write_at done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_read_at_all(MPI_File mpi_fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
        if(comm != NULL)
        {
            MPI_File translated = comm->translate_structure(mpi_fh);
            MPI_Barrier(comm->get_alias());
            rc = PMPI_File_read_at_all(translated, offset, buf, count, datatype, status);
        }
        else
            rc = PMPI_File_read_at_all(mpi_fh, offset, buf, count, datatype, status);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: read_at_all done (error: %s)\n", rank, size, errstr);
        }
        if(comm != NULL)
        {
            agree_and_eventually_replace(&rc, comm);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_File_write_at_all(MPI_File mpi_fh, MPI_Offset offset, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
        if(comm != NULL)
        {
            MPI_File translated = comm->translate_structure(mpi_fh);
            MPI_Barrier(comm->get_alias());
            rc = PMPI_File_write_at_all(translated, offset, buf, count, datatype, status);
        }
        else
            rc = PMPI_File_write_at_all(mpi_fh, offset, buf, count, datatype, status);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: write_at_all done (error: %s)\n", rank, size, errstr);
        }
        if(comm != NULL)
        {
            agree_and_eventually_replace(&rc, comm);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_File_seek(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        MPI_File translated = comm->translate_structure(mpi_fh);
        rc = PMPI_File_seek(translated, offset, whence);
    }
    else
        rc = PMPI_File_seek(mpi_fh, offset, whence);
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

int MPI_File_get_position(MPI_File mpi_fh, MPI_Offset *offset)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        rc = PMPI_File_get_position(translated, offset);
    }
    else
        rc = PMPI_File_get_position(mpi_fh, offset);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: get_position done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_seek_shared(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    int rc;
    while(1)
    {
        MPI_Offset starting_offset;
        AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
        if(comm != NULL)
        {
            MPI_Barrier(comm->get_alias());
            MPI_File translated = comm->translate_structure(mpi_fh);
            MPI_File_get_position_shared(translated, &starting_offset);
            rc = PMPI_File_seek_shared(translated, offset, whence);
        }
        else
            rc = PMPI_File_seek_shared(mpi_fh, offset, whence);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: seek_shared done (error: %s)\n", rank, size, errstr);
        }
        if(comm != NULL)
        {
            agree_and_eventually_replace(&rc, comm);
            if(rc == MPI_SUCCESS)
                return rc;
            else
            {
                MPI_File translated = comm->translate_structure(mpi_fh);
                PMPI_File_seek_shared(translated, starting_offset, MPI_SEEK_SET);
            }
        }
        else
            return rc;
    }
}

int MPI_File_get_position_shared(MPI_File mpi_fh, MPI_Offset *offset)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        //MPI_Barrier(comm->get_alias());
        rc = PMPI_File_get_position_shared(translated, offset);
    }
    else
        rc = PMPI_File_get_position_shared(mpi_fh, offset);
    
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: get_position_shared done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_read_all(MPI_File mpi_fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
        if(comm != NULL)
        {
            MPI_File translated = comm->translate_structure(mpi_fh);
            MPI_Barrier(comm->get_alias());
            rc = PMPI_File_read_all(translated, buf, count, datatype, status);
        }
        else        
            rc = PMPI_File_read_all(mpi_fh, buf, count, datatype, status);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: read_all done (error: %s)\n", rank, size, errstr);
        }
        if(comm != NULL)
        {
            agree_and_eventually_replace(&rc, comm);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_File_write_all(MPI_File mpi_fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
        if(comm != NULL)
        {
            MPI_File translated = comm->translate_structure(mpi_fh);
            MPI_Barrier(comm->get_alias());
            rc = PMPI_File_write_all(translated, buf, count, datatype, status);
        }
        else
            rc = PMPI_File_write_all(mpi_fh, buf, count, datatype, status);

        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: write_all done (error: %s)\n", rank, size, errstr);
        }
        if(comm != NULL)
        {
            agree_and_eventually_replace(&rc, comm);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_File_set_view(MPI_File mpi_fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, char* datarep, MPI_Info info)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
        if(comm != NULL)
        {
            MPI_File translated = comm->translate_structure(mpi_fh);
            MPI_Barrier(comm->get_alias());
            rc = PMPI_File_set_view(translated, disp, etype, filetype, datarep, info);
        }
        else
            rc = PMPI_File_set_view(mpi_fh, disp, etype, filetype, datarep, info);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: set_view done (error: %s)\n", rank, size, errstr);
        }
        if(comm != NULL)
        {
            agree_and_eventually_replace(&rc, comm);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_File_read(MPI_File mpi_fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        //MPI_Barrier(comm->get_alias());
        rc = PMPI_File_read(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_read(mpi_fh, buf, count, datatype, status);
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

int MPI_File_write(MPI_File mpi_fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        //MPI_Barrier(comm->get_alias());
        rc = PMPI_File_write(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_write(mpi_fh, buf, count, datatype, status);
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

int MPI_File_read_shared(MPI_File mpi_fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc; 
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        //MPI_Barrier(comm->get_alias());
        rc = PMPI_File_read_shared(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_read_shared(mpi_fh, buf, count, datatype, status);

    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: read_shared done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_write_shared(MPI_File mpi_fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        //MPI_Barrier(comm->get_alias());
        rc = PMPI_File_write_shared(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_write_shared(mpi_fh, buf, count, datatype, status);

    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: write_shared done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_read_ordered(MPI_File mpi_fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        MPI_Barrier(comm->get_alias());
        rc = PMPI_File_read_ordered(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_read_ordered(mpi_fh, buf, count, datatype, status);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: read_ordered done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_write_ordered(MPI_File mpi_fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        MPI_Barrier(comm->get_alias());
        rc = PMPI_File_write_ordered(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_write_ordered(mpi_fh, buf, count, datatype, status);
    if (VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: write_ordered done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_sync(MPI_File mpi_fh)
{
    int rc;
    AdvComm * comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        //MPI_Barrier(comm->get_alias());
        rc = PMPI_File_sync(translated);
    }
    else
        rc = PMPI_File_sync(mpi_fh);
    if(VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: file_sync done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_get_size(MPI_File mpi_fh, MPI_Offset * size)
{
    int rc;
    AdvComm * comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        //MPI_Barrier(comm->get_alias());
        rc = PMPI_File_get_size(translated, size);
    }
    else
        rc = PMPI_File_get_size(mpi_fh, size);
    if(VERBOSE)
    {
        int rank, comm_size;
        PMPI_Comm_size(MPI_COMM_WORLD, &comm_size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: file_get_size done (error: %s)\n", rank, comm_size, errstr);
    }
    return rc;
}

int MPI_File_get_type_extent(MPI_File mpi_fh, MPI_Datatype datatype, MPI_Aint * extent)
{
    int rc;
    AdvComm * comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        //MPI_Barrier(comm->get_alias());
        rc = PMPI_File_get_type_extent(translated, datatype, extent);
    }
    else
        rc = PMPI_File_get_type_extent(mpi_fh, datatype, extent);
    if(VERBOSE)
    {
        int rank, size;
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: file_get_type_extent done (error: %s)\n", rank, size, errstr);
    }
    return rc;
}

int MPI_File_set_size(MPI_File mpi_fh, MPI_Offset size)
{
    int rc;
    AdvComm * comm = cur_comms->get_complex_from_file(mpi_fh);
    if(comm != NULL)
    {
        MPI_File translated = comm->translate_structure(mpi_fh);
        //MPI_Barrier(comm->get_alias());
        rc = PMPI_File_set_size(translated, size);
    }
    else
        rc = PMPI_File_set_size(mpi_fh, size);
    if(VERBOSE)
    {
        int rank, comm_size;
        PMPI_Comm_size(MPI_COMM_WORLD, &comm_size);
        PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Error_string(rc, errstr, &len);
        printf("Rank %d / %d: file_set_size done (error: %s)\n", rank, comm_size, errstr);
    }
    return rc;
}