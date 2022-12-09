#include <mpi.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "comm_manipulation.hpp"
#include "complex_comm.hpp"
#include "log.hpp"
#include "mpi-ext.h"
#include "multicomm.hpp"

using namespace legio;

int MPI_File_open(MPI_Comm comm, const char* filename, int amode, MPI_Info info, MPI_File* mpi_fh)
{
    int consequent_amode = amode;
    int first_amode = amode;
    if (amode & MPI_MODE_EXCL)
    {
        consequent_amode ^= MPI_MODE_EXCL;
    }
    if (amode & MPI_MODE_DELETE_ON_CLOSE)
    {
        consequent_amode ^= MPI_MODE_DELETE_ON_CLOSE;
        first_amode ^= MPI_MODE_DELETE_ON_CLOSE;
        // Find a way to delete files on close only in this case
    }
    if (amode & MPI_MODE_UNIQUE_OPEN)
    {
        consequent_amode ^= MPI_MODE_UNIQUE_OPEN;
        first_amode ^= MPI_MODE_UNIQUE_OPEN;
        // Find a way to keep unique open functionality while allowing more file handlers for same
        // file
    }
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(comm);
        std::function<int(MPI_Comm, MPI_File*)> func;
        if (flag)
        {
            ComplexComm& translated = Multicomm::get_instance().translate_into_complex(comm);
            MPI_Barrier(translated.get_alias());
            func = [filename, consequent_amode, info](MPI_Comm c, MPI_File* f) -> int {
                int rc = PMPI_File_open(c, filename, consequent_amode, info, f);
                MPI_File_set_errhandler(*f, MPI_ERRORS_RETURN);
                return rc;
            };
            rc = PMPI_File_open(translated.get_comm(), filename, consequent_amode, info, mpi_fh);
        }
        else
            rc = PMPI_File_open(comm, filename, amode, info, mpi_fh);
        legio::report_execution(rc, comm, "File_open");
        if (!flag)
            return rc;
        else if (rc == MPI_SUCCESS)
        {
            bool result = Multicomm::get_instance().add_structure(
                Multicomm::get_instance().translate_into_complex(comm), *mpi_fh, func);
            if (result)
                return rc;
        }
        else
            replace_comm(Multicomm::get_instance().translate_into_complex(comm));
    }
}

int MPI_File_close(MPI_File* mpi_fh)
{
    Multicomm::get_instance().remove_structure(mpi_fh);
    return MPI_SUCCESS;
}

int MPI_File_read_at(MPI_File mpi_fh,
                     MPI_Offset offset,
                     void* buf,
                     int count,
                     MPI_Datatype datatype,
                     MPI_Status* status)
{
    int rc;

    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        rc = PMPI_File_read_at(translated, offset, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_read_at(mpi_fh, offset, buf, count, datatype, status);
    legio::report_execution(rc, MPI_COMM_WORLD, "Read_at");
    return rc;
}

int MPI_File_write_at(MPI_File mpi_fh,
                      MPI_Offset offset,
                      const void* buf,
                      int count,
                      MPI_Datatype datatype,
                      MPI_Status* status)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        rc = PMPI_File_write_at(translated, offset, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_write_at(mpi_fh, offset, buf, count, datatype, status);
    legio::report_execution(rc, MPI_COMM_WORLD, "Write_at");
    return rc;
}

int MPI_File_read_at_all(MPI_File mpi_fh,
                         MPI_Offset offset,
                         void* buf,
                         int count,
                         MPI_Datatype datatype,
                         MPI_Status* status)
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(mpi_fh);
        if (flag)
        {
            MPI_File translated =
                Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                    mpi_fh);
            MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
            rc = PMPI_File_read_at_all(translated, offset, buf, count, datatype, status);
        }
        else
            rc = PMPI_File_read_at_all(mpi_fh, offset, buf, count, datatype, status);
        legio::report_execution(rc, MPI_COMM_WORLD, "Read_at_all");
        if (flag)
        {
            agree_and_eventually_replace(
                &rc, Multicomm::get_instance().get_complex_from_structure(mpi_fh));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_File_write_at_all(MPI_File mpi_fh,
                          MPI_Offset offset,
                          const void* buf,
                          int count,
                          MPI_Datatype datatype,
                          MPI_Status* status)
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(mpi_fh);
        if (flag)
        {
            MPI_File translated =
                Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                    mpi_fh);
            MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
            rc = PMPI_File_write_at_all(translated, offset, buf, count, datatype, status);
        }
        else
            rc = PMPI_File_write_at_all(mpi_fh, offset, buf, count, datatype, status);
        legio::report_execution(rc, MPI_COMM_WORLD, "Write_at_all");
        if (flag)
        {
            agree_and_eventually_replace(
                &rc, Multicomm::get_instance().get_complex_from_structure(mpi_fh));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_File_seek(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        rc = PMPI_File_seek(translated, offset, whence);
    }
    else
        rc = PMPI_File_seek(mpi_fh, offset, whence);
    legio::report_execution(rc, MPI_COMM_WORLD, "File_seek");
    return rc;
}

int MPI_File_get_position(MPI_File mpi_fh, MPI_Offset* offset)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        rc = PMPI_File_get_position(translated, offset);
    }
    else
        rc = PMPI_File_get_position(mpi_fh, offset);
    legio::report_execution(rc, MPI_COMM_WORLD, "File_get_position");
    return rc;
}

int MPI_File_seek_shared(MPI_File mpi_fh, MPI_Offset offset, int whence)
{
    int rc;
    while (1)
    {
        MPI_Offset starting_offset;
        bool flag = Multicomm::get_instance().part_of(mpi_fh);
        if (flag)
        {
            MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
            MPI_File translated =
                Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                    mpi_fh);
            MPI_File_get_position_shared(translated, &starting_offset);
            rc = PMPI_File_seek_shared(translated, offset, whence);
        }
        else
            rc = PMPI_File_seek_shared(mpi_fh, offset, whence);
        legio::report_execution(rc, MPI_COMM_WORLD, "File_seek_shared");
        if (flag)
        {
            agree_and_eventually_replace(
                &rc, Multicomm::get_instance().get_complex_from_structure(mpi_fh));
            if (rc == MPI_SUCCESS)
                return rc;
            else
            {
                MPI_File translated = Multicomm::get_instance()
                                          .get_complex_from_structure(mpi_fh)
                                          .translate_structure(mpi_fh);
                PMPI_File_seek_shared(translated, starting_offset, MPI_SEEK_SET);
            }
        }
        else
            return rc;
    }
}

int MPI_File_get_position_shared(MPI_File mpi_fh, MPI_Offset* offset)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_get_position_shared(translated, offset);
    }
    else
        rc = PMPI_File_get_position_shared(mpi_fh, offset);

    legio::report_execution(rc, MPI_COMM_WORLD, "File_get_position_shared");
    return rc;
}

int MPI_File_read_all(MPI_File mpi_fh,
                      void* buf,
                      int count,
                      MPI_Datatype datatype,
                      MPI_Status* status)
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(mpi_fh);
        if (flag)
        {
            MPI_File translated =
                Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                    mpi_fh);
            MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
            rc = PMPI_File_read_all(translated, buf, count, datatype, status);
        }
        else
            rc = PMPI_File_read_all(mpi_fh, buf, count, datatype, status);
        legio::report_execution(rc, MPI_COMM_WORLD, "File_read_all");
        if (flag)
        {
            agree_and_eventually_replace(
                &rc, Multicomm::get_instance().get_complex_from_structure(mpi_fh));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_File_write_all(MPI_File mpi_fh,
                       const void* buf,
                       int count,
                       MPI_Datatype datatype,
                       MPI_Status* status)
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(mpi_fh);
        if (flag)
        {
            MPI_File translated =
                Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                    mpi_fh);
            MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
            rc = PMPI_File_write_all(translated, buf, count, datatype, status);
        }
        else
            rc = PMPI_File_write_all(mpi_fh, buf, count, datatype, status);

        legio::report_execution(rc, MPI_COMM_WORLD, "File_write_all");
        if (flag)
        {
            agree_and_eventually_replace(
                &rc, Multicomm::get_instance().get_complex_from_structure(mpi_fh));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_File_set_view(MPI_File mpi_fh,
                      MPI_Offset disp,
                      MPI_Datatype etype,
                      MPI_Datatype filetype,
                      char* datarep,
                      MPI_Info info)
{
    while (1)
    {
        int rc;
        bool flag = Multicomm::get_instance().part_of(mpi_fh);
        if (flag)
        {
            MPI_File translated =
                Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                    mpi_fh);
            MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
            rc = PMPI_File_set_view(translated, disp, etype, filetype, datarep, info);
        }
        else
            rc = PMPI_File_set_view(mpi_fh, disp, etype, filetype, datarep, info);
        legio::report_execution(rc, MPI_COMM_WORLD, "File_set_view");
        if (flag)
        {
            agree_and_eventually_replace(
                &rc, Multicomm::get_instance().get_complex_from_structure(mpi_fh));
            if (rc == MPI_SUCCESS)
                return rc;
        }
        else
            return rc;
    }
}

int MPI_File_read(MPI_File mpi_fh, void* buf, int count, MPI_Datatype datatype, MPI_Status* status)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_read(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_read(mpi_fh, buf, count, datatype, status);
    legio::report_execution(rc, MPI_COMM_WORLD, "File_read");
    return rc;
}

int MPI_File_write(MPI_File mpi_fh,
                   const void* buf,
                   int count,
                   MPI_Datatype datatype,
                   MPI_Status* status)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_write(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_write(mpi_fh, buf, count, datatype, status);
    legio::report_execution(rc, MPI_COMM_WORLD, "File_write");
    return rc;
}

int MPI_File_read_shared(MPI_File mpi_fh,
                         void* buf,
                         int count,
                         MPI_Datatype datatype,
                         MPI_Status* status)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_read_shared(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_read_shared(mpi_fh, buf, count, datatype, status);

    legio::report_execution(rc, MPI_COMM_WORLD, "File_read_shared");
    return rc;
}

int MPI_File_write_shared(MPI_File mpi_fh,
                          const void* buf,
                          int count,
                          MPI_Datatype datatype,
                          MPI_Status* status)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_write_shared(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_write_shared(mpi_fh, buf, count, datatype, status);

    legio::report_execution(rc, MPI_COMM_WORLD, "File_write_shared");
    return rc;
}

int MPI_File_read_ordered(MPI_File mpi_fh,
                          void* buf,
                          int count,
                          MPI_Datatype datatype,
                          MPI_Status* status)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_read_ordered(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_read_ordered(mpi_fh, buf, count, datatype, status);
    legio::report_execution(rc, MPI_COMM_WORLD, "File_read_ordered");
    return rc;
}

int MPI_File_write_ordered(MPI_File mpi_fh,
                           const void* buf,
                           int count,
                           MPI_Datatype datatype,
                           MPI_Status* status)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_write_ordered(translated, buf, count, datatype, status);
    }
    else
        rc = PMPI_File_write_ordered(mpi_fh, buf, count, datatype, status);
    legio::report_execution(rc, MPI_COMM_WORLD, "File_write_ordered");
    return rc;
}

int MPI_File_sync(MPI_File mpi_fh)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_sync(translated);
    }
    else
        rc = PMPI_File_sync(mpi_fh);
    legio::report_execution(rc, MPI_COMM_WORLD, "File_sync");
    return rc;
}

int MPI_File_get_size(MPI_File mpi_fh, MPI_Offset* size)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_get_size(translated, size);
    }
    else
        rc = PMPI_File_get_size(mpi_fh, size);
    legio::report_execution(rc, MPI_COMM_WORLD, "File_get_size");
    return rc;
}

int MPI_File_get_type_extent(MPI_File mpi_fh, MPI_Datatype datatype, MPI_Aint* extent)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_get_type_extent(translated, datatype, extent);
    }
    else
        rc = PMPI_File_get_type_extent(mpi_fh, datatype, extent);
    legio::report_execution(rc, MPI_COMM_WORLD, "File_get_type_extent");
    return rc;
}

int MPI_File_set_size(MPI_File mpi_fh, MPI_Offset size)
{
    int rc;
    bool flag = Multicomm::get_instance().part_of(mpi_fh);
    if (flag)
    {
        MPI_File translated =
            Multicomm::get_instance().get_complex_from_structure(mpi_fh).translate_structure(
                mpi_fh);
        // MPI_Barrier(Multicomm::get_instance().get_complex_from_structure(mpi_fh).get_alias());
        rc = PMPI_File_set_size(translated, size);
    }
    else
        rc = PMPI_File_set_size(mpi_fh, size);
    legio::report_execution(rc, MPI_COMM_WORLD, "File_set_size");
    return rc;
}