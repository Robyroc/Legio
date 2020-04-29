#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "complex_comm.h"
#include <string.h>

extern ComplexComm *cur_complex;
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
        int rc;
        std::function<int(MPI_Comm, MPI_File *)> func;
        if(comm == MPI_COMM_WORLD)
        {
            MPI_Barrier(MPI_COMM_WORLD);
            func = [filename, consequent_amode, info] (MPI_Comm c, MPI_File* f) -> int
            {
                int rc = PMPI_File_open(c, filename, consequent_amode, info, f);
                MPI_File_set_errhandler(*f, MPI_ERRORS_RETURN);
                return rc;
            };
            rc = PMPI_File_open(cur_complex->get_comm(), filename, consequent_amode, info, mpi_fh);
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
    {
        MPI_Info info;
        char value[MPI_MAX_INFO_VAL];
        char name[MPI_MAX_INFO_VAL];
        MPI_File_get_info(*mpi_fh, &info);
        MPI_Info_get(info, "tbd", MPI_MAX_INFO_VAL, value, &flag);
        MPI_Info_get(info, "filename", MPI_MAX_INFO_VAL, name, &flag);
        if(!strcmp(value, "1"))
            MPI_File_delete(name, MPI_INFO_NULL);
        cur_complex->remove_structure(*mpi_fh);
    }
    return MPI_SUCCESS;
}

int MPI_File_read_at(MPI_File mpi_fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc; //, flag;
    //cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);
    /*
    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);
    */
    rc = PMPI_File_read_at(translated, offset, buf, count, datatype, status);
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
    int rc; //, flag;
    //cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);
    /*
    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);
    */
    rc = PMPI_File_write_at(translated, offset, buf, count, datatype, status);
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
        int rc, flag;
        cur_complex->check_global(mpi_fh, &flag);
        MPI_File translated = cur_complex->translate_structure(mpi_fh);
        
        if(flag)
            MPI_Barrier(MPI_COMM_WORLD);
        
        rc = PMPI_File_read_at_all(translated, offset, buf, count, datatype, status);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: read_at_all done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, cur_complex);
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
        int rc, flag;
        cur_complex->check_global(mpi_fh, &flag);
        MPI_File translated = cur_complex->translate_structure(mpi_fh);
        
        if(flag)
            MPI_Barrier(MPI_COMM_WORLD);
        
        rc = PMPI_File_write_at_all(translated, offset, buf, count, datatype, status);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: write_at_all done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, cur_complex);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
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

int MPI_File_get_position(MPI_File mpi_fh, MPI_Offset *offset)
{
    int rc; //, flag;
    //cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);
    /*
    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);
    */
    rc = PMPI_File_get_position(translated, offset);
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
    int rc, flag;
    while(1)
    {
        MPI_Offset starting_offset;
        cur_complex->check_global(mpi_fh, &flag);
        MPI_File translated = cur_complex->translate_structure(mpi_fh);
        if(flag)
        {
            MPI_File_get_position_shared(translated, &starting_offset);
            MPI_Barrier(MPI_COMM_WORLD);
        }
        rc = PMPI_File_seek_shared(translated, offset, whence);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: seek_shared done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, cur_complex);
            if(rc == MPI_SUCCESS)
                return rc;
            else
            {
                translated = cur_complex->translate_structure(mpi_fh);
                PMPI_File_seek_shared(translated, starting_offset, MPI_SEEK_SET);
            }
        }
        else
            return rc;
    }
}

int MPI_File_get_position_shared(MPI_File mpi_fh, MPI_Offset *offset)
{
    int rc; //, flag;
    //cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);
    /*
    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);
    */
    rc = PMPI_File_get_position_shared(translated, offset);
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
        int rc, flag;
        cur_complex->check_global(mpi_fh, &flag);
        MPI_File translated = cur_complex->translate_structure(mpi_fh);
        
        if(flag)
            MPI_Barrier(MPI_COMM_WORLD);
        
        rc = PMPI_File_read_all(translated, buf, count, datatype, status);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: read_all done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, cur_complex);
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
        int rc, flag;
        cur_complex->check_global(mpi_fh, &flag);
        MPI_File translated = cur_complex->translate_structure(mpi_fh);
        
        if(flag)
            MPI_Barrier(MPI_COMM_WORLD);
        
        rc = PMPI_File_write_all(translated, buf, count, datatype, status);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: write_all done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, cur_complex);
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
        int rc, flag;
        cur_complex->check_global(mpi_fh, &flag);
        MPI_File translated = cur_complex->translate_structure(mpi_fh);
        
        if(flag)
            MPI_Barrier(MPI_COMM_WORLD);
        
        rc = PMPI_File_set_view(translated, disp, etype, filetype, datarep, info);
        if (VERBOSE)
        {
            int rank, size;
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            PMPI_Comm_rank(MPI_COMM_WORLD, &rank);
            MPI_Error_string(rc, errstr, &len);
            printf("Rank %d / %d: set_view done (error: %s)\n", rank, size, errstr);
        }
        if(flag)
        {
            agree_and_eventually_replace(&rc, cur_complex);
            if(rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
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

int MPI_File_write(MPI_File mpi_fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
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

int MPI_File_read_shared(MPI_File mpi_fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc; //, flag;
    //cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);
    /*
    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);
    */
   
    rc = PMPI_File_read_shared(translated, buf, count, datatype, status);
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
    int rc; //, flag;
    //cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);
    /*
    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);
    */
    rc = PMPI_File_write_shared(translated, buf, count, datatype, status);
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
    int rc, flag;
    cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);
    
    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);
   
    rc = PMPI_File_read_ordered(translated, buf, count, datatype, status);
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
    int rc, flag;
    cur_complex->check_global(mpi_fh, &flag);
    MPI_File translated = cur_complex->translate_structure(mpi_fh);

    if(flag)
        MPI_Barrier(MPI_COMM_WORLD);

    rc = PMPI_File_write_ordered(translated, buf, count, datatype, status);
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