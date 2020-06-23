#include <mpi.h>
#include <mpi-ext.h>
#include <stdio.h>
#include <signal.h>
#include "comm_manipulation.h"
#include "configuration.h"
#include "adv_comm.h"
#include <string.h>
#include "multicomm.h"
#include "operations.h"
#include "no_comm.h"

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
        AdvComm* translated = cur_comms->translate_into_adv(comm);

        LocalOnly operation([filename, consequent_amode, info, mpi_fh] (MPI_Comm comm_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_File_open(comm_t, filename, consequent_amode, info, mpi_fh);
            if(rc != MPI_SUCCESS)
                replace_comm(adv, comm_t);
            return rc;
        }, false);

        std::function<int(MPI_Comm, MPI_File *)> func = [filename, consequent_amode, info] (MPI_Comm c, MPI_File* f) -> int {
            int rc = PMPI_File_open(c, filename, consequent_amode, info, f);
            MPI_File_set_errhandler(*f, MPI_ERRORS_RETURN);
            return rc;
        };

        MPI_Barrier(translated->get_alias());
        rc = translated->perform_operation(operation);

        print_info("file_open", comm, rc);

        if(!flag)
            return rc;
        else if(rc == MPI_SUCCESS)
        {
            MPI_File_set_errhandler(*mpi_fh, MPI_ERRORS_RETURN);
            bool result = cur_comms->add_file(translated, *mpi_fh, func);
            if(result)
                return rc;
        }
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
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([offset, buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_read_at(file_t, offset, buf, count, datatype, status);
    }, false);

    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
        rc = func(mpi_fh, NULL);
    
    print_info("read_at", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_write_at(MPI_File mpi_fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc; 
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([offset, buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_write_at(file_t, offset, buf, count, datatype, status);
    }, false);
    
    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
        rc = func(mpi_fh, NULL);
    
    print_info("write_at", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_read_at_all(MPI_File mpi_fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

        FileOp func([offset, buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_File_read_at_all(file_t, offset, buf, count, datatype, status);
            agree_and_eventually_replace(&rc, adv, file_t);
            return rc;
        }, false);

        if(comm != NULL)
        {
            MPI_Barrier(comm->get_alias());
            rc = comm->perform_operation(func, mpi_fh);
        }
        else
        {
            AdvComm* temp = new NoComm(MPI_COMM_SELF);
            rc = func(mpi_fh, temp);
            delete temp;
        }
            
        
        print_info("read_at_all", MPI_COMM_WORLD, rc);

        if(comm == NULL || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_File_write_at_all(MPI_File mpi_fh, MPI_Offset offset, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

        FileOp func([offset, buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_File_write_at_all(file_t, offset, buf, count, datatype, status);
            agree_and_eventually_replace(&rc, adv, file_t);
            return rc;
        }, false);

        if(comm != NULL)
        {
            MPI_Barrier(comm->get_alias());
            rc = comm->perform_operation(func, mpi_fh);
        }
        else
        {
            AdvComm* temp = new NoComm(MPI_COMM_SELF);
            rc = func(mpi_fh, temp);
            delete temp;
        }
        
        print_info("write_at_all", MPI_COMM_WORLD, rc);

        if(comm == NULL || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_File_seek(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    int rc;
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([offset, whence] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_seek(file_t, offset, whence);
    }, false);

    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
        func(mpi_fh, NULL);
    
    print_info("seek", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_get_position(MPI_File mpi_fh, MPI_Offset *offset)
{
    int rc;
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([offset] (MPI_File file_t, AdvComm*) -> int {
        return PMPI_File_get_position(file_t, offset);
    }, false);

    if(comm != NULL)
        rc = comm->perform_operation(func, mpi_fh);
    else
        rc = func(mpi_fh, NULL);
    
    print_info("get_position", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_seek_shared(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    int rc;
    while(1)
    {
        AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

        FileOp func([offset, whence] (MPI_File file_t, AdvComm* adv) -> int {
            int rc;
            MPI_Offset starting_offset;
            MPI_File_get_position_shared(file_t, &starting_offset);
            rc = PMPI_File_seek_shared(file_t, offset, whence);
            if(rc != MPI_SUCCESS)
                PMPI_File_seek_shared(file_t, starting_offset, MPI_SEEK_SET);
            agree_and_eventually_replace(&rc, adv, file_t);
            return rc;
        }, true);

        if(comm != NULL)
        {
            MPI_Barrier(comm->get_alias());
            rc = comm->perform_operation(func, mpi_fh);
        }
        else
        {
            AdvComm* temp = new NoComm(MPI_COMM_SELF);
            rc = func(mpi_fh, temp);
            delete temp;
        }
        
        print_info("seek_shared", MPI_COMM_WORLD, rc);

        if(comm == NULL || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_File_get_position_shared(MPI_File mpi_fh, MPI_Offset *offset)
{
    int rc;
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([offset] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_get_position_shared(file_t, offset);
    }, true);

    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
        rc = func(mpi_fh, NULL);
    
    print_info("get_position_shared", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_read_all(MPI_File mpi_fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

        FileOp func([buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_File_read_all(file_t, buf, count, datatype, status);
            agree_and_eventually_replace(&rc, adv, file_t);
            return rc;
        }, true);

        if(comm != NULL)
        {
            MPI_Barrier(comm->get_alias());
            rc = comm->perform_operation(func, mpi_fh);
        }
        else
        {
            AdvComm* temp = new NoComm(MPI_COMM_SELF);
            rc = func(mpi_fh, temp);
            delete temp;
        }
        
        print_info("read_all", MPI_COMM_WORLD, rc);

        if(comm == NULL || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_File_write_all(MPI_File mpi_fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

        FileOp func([buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_File_write_all(file_t, buf, count, datatype, status);
            agree_and_eventually_replace(&rc, adv, file_t);
            return rc;
        }, true);

        if(comm != NULL)
        {
            MPI_Barrier(comm->get_alias());
            rc = comm->perform_operation(func, mpi_fh);
        }
        else
        {
            AdvComm* temp = new NoComm(MPI_COMM_SELF);
            rc = func(mpi_fh, temp);
            delete temp;
        }

        print_info("write_all", MPI_COMM_WORLD, rc);

        if(comm == NULL || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_File_set_view(MPI_File mpi_fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, char* datarep, MPI_Info info)
{
    while(1)
    {
        int rc;
        AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

        FileOp func([disp, etype, filetype, datarep, info] (MPI_File file_t, AdvComm* adv) -> int {
            int rc;
            rc = PMPI_File_set_view(file_t, disp, etype, filetype, datarep, info);
            agree_and_eventually_replace(&rc, adv, file_t);
            return rc;
        }, false);

        if(comm != NULL)
        {
            MPI_Barrier(comm->get_alias());
            rc = comm->perform_operation(func, mpi_fh);
        }
        else
        {
            AdvComm* temp = new NoComm(MPI_COMM_SELF);
            rc = func(mpi_fh, temp);
            delete temp;
        }
        
        print_info("set_view", MPI_COMM_WORLD, rc);

        if(comm == NULL || rc == MPI_SUCCESS)
            return rc;
    }
}

int MPI_File_read(MPI_File mpi_fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_read(file_t, buf, count, datatype, status);
    }, false);

    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
        rc = func(mpi_fh, NULL);
    
    print_info("read", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_write(MPI_File mpi_fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_write(file_t, buf, count, datatype, status);
    }, false);

    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
        rc = func(mpi_fh, NULL);
    
    print_info("write", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_read_shared(MPI_File mpi_fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc; 
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_read_shared(file_t, buf, count, datatype, status);
    }, true);
    
    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
        rc = func(mpi_fh, NULL);

    print_info("read_shared", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_write_shared(MPI_File mpi_fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_write_shared(file_t, buf, count, datatype, status);
    }, true);

    if(comm != NULL)
    {
        //MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
        rc = func(mpi_fh, NULL);

    print_info("write shared", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_read_ordered(MPI_File mpi_fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_read_ordered(file_t, buf, count, datatype, status);
    }, true);

    if(comm != NULL)
    {
        MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
    {
        AdvComm* temp = new NoComm(MPI_COMM_SELF);
        rc = func(mpi_fh, temp);
        delete temp;
    }
    
    print_info("read_ordered", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_write_ordered(MPI_File mpi_fh, const void* buf, int count, MPI_Datatype datatype, MPI_Status *status)
{
    int rc;
    AdvComm* comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([buf, count, datatype, status] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_write_ordered(file_t, buf, count, datatype, status);
    }, true);

    if(comm != NULL)
    {
        MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
    {
        AdvComm* temp = new NoComm(MPI_COMM_SELF);
        rc = func(mpi_fh, temp);
        delete temp;
    }
    
    print_info("write_ordered", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_sync(MPI_File mpi_fh)
{
    int rc;
    AdvComm * comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([](MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_sync(file_t);
    }, false);
    if(comm != NULL)
    {
        MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
    {
        AdvComm* temp = new NoComm(MPI_COMM_SELF);
        rc = func(mpi_fh, temp);
        delete temp;
    }
    
    print_info("file_sync", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_get_size(MPI_File mpi_fh, MPI_Offset * size)
{
    int rc;
    AdvComm * comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([size] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_get_size(file_t, size);
    }, false);

    if(comm != NULL)
    {
        MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
    {
        AdvComm* temp = new NoComm(MPI_COMM_SELF);
        rc = func(mpi_fh, temp);
        delete temp;
    }
    
    print_info("file_get_size", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_get_type_extent(MPI_File mpi_fh, MPI_Datatype datatype, MPI_Aint * extent)
{
    int rc;
    AdvComm * comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([datatype, extent] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_get_type_extent(file_t, datatype, extent);
    }, false);

    if(comm != NULL)
    {
        MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
    {
        AdvComm* temp = new NoComm(MPI_COMM_SELF);
        rc = func(mpi_fh, temp);
        delete temp;
    }
    
    print_info("file_get_type_extent", MPI_COMM_WORLD, rc);

    return rc;
}

int MPI_File_set_size(MPI_File mpi_fh, MPI_Offset size)
{
    int rc;
    AdvComm * comm = cur_comms->get_adv_from_file(mpi_fh);

    FileOp func([size] (MPI_File file_t, AdvComm* adv) -> int {
        return PMPI_File_set_size(file_t, size);
    }, false);

    if(comm != NULL)
    {
        MPI_Barrier(comm->get_alias());
        rc = comm->perform_operation(func, mpi_fh);
    }
    else
    {
        AdvComm* temp = new NoComm(MPI_COMM_SELF);
        rc = func(mpi_fh, temp);
        delete temp;
    }
    
    print_info("file_set_size", MPI_COMM_WORLD, rc);

    return rc;
}