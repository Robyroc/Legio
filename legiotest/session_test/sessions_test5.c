#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

#include "legio.h"

void print_error(const char* msg, int rc)
{
    char err_str[MPI_MAX_ERROR_STRING];
    int resultlen = sizeof(err_str) - 1;

    MPI_Error_string(rc, err_str, &resultlen);
    fprintf(stderr, "%s return err code  = %d (%s)\n", msg, rc, err_str);
}

void my_session_errhandler(MPI_Session* foo, int* bar, ...)
{
    fprintf(stderr, "errhandler called with error %d\n", *bar);
}

int main(int argc, char* argv[])
{
    MPI_Session session, session1;
    MPI_Errhandler errhandler;
    MPI_Group group, group1, group2, group3;
    MPI_Comm comm_world, comm_self, comm_world1, comm_self1;
    MPI_Info info;
    int rc, npsets, npsets1, one = 1, sum, sum1, i;

    rc = MPI_Session_create_errhandler(my_session_errhandler, &errhandler);
    if (MPI_SUCCESS != rc)
    {
        print_error("Error handler creation failed", rc);
        return -1;
    }

    rc = MPI_Info_create(&info);
    if (MPI_SUCCESS != rc)
    {
        print_error("Info creation failed", rc);
        return -1;
    }

    rc = MPI_Info_set(info, "mpi_thread_support_level", "MPI_THREAD_MULTIPLE");
    if (MPI_SUCCESS != rc)
    {
        print_error("Info key/val set failed", rc);
        return -1;
    }

    rc = MPI_Session_init(info, errhandler, &session);
    if (MPI_SUCCESS != rc)
    {
        print_error("Session initialization failed", rc);
        return -1;
    }

    rc = MPI_Session_init(info, errhandler, &session1);
    if (MPI_SUCCESS != rc)
    {
        print_error("Session1 initialization failed", rc);
        return -1;
    }

    rc = MPI_Session_get_num_psets(session, MPI_INFO_NULL, &npsets);
    for (i = 0; i < npsets; ++i)
    {
        int psetlen = 0;
        char name[256];
        MPI_Session_get_nth_pset(session, MPI_INFO_NULL, i, &psetlen, NULL);
        MPI_Session_get_nth_pset(session, MPI_INFO_NULL, i, &psetlen, name);
    }

    rc = MPI_Session_get_num_psets(session1, MPI_INFO_NULL, &npsets1);
    for (i = 0; i < npsets1; ++i)
    {
        int psetlen = 0;
        char name[256];
        MPI_Session_get_nth_pset(session1, MPI_INFO_NULL, i, &psetlen, NULL);
        MPI_Session_get_nth_pset(session1, MPI_INFO_NULL, i, &psetlen, name);
    }

    rc = MPI_Group_from_session_pset(session, "mpi://WORLD", &group);
    if (MPI_SUCCESS != rc)
    {
        print_error("Could not get a group for mpi://WORLD. ", rc);
        return -1;
    }

    rc = MPI_Group_from_session_pset(session1, "mpi://WORLD", &group1);
    if (MPI_SUCCESS != rc)
    {
        print_error("Could not get a group1 for mpi://WORLD. ", rc);
        return -1;
    }

    // rc = MPIX_Horizon_from_group(group);

    MPI_Comm_create_from_group(group, "my_world", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm_world);
    MPI_Group_free(&group);

    MPI_Comm_create_from_group(group1, "my_world", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm_world1);
    MPI_Group_free(&group1);

    MPI_Allreduce(&one, &sum, 1, MPI_INT, MPI_SUM, comm_world);

    rc = MPI_Group_from_session_pset(session, "mpi://SELF", &group2);
    if (MPI_SUCCESS != rc)
    {
        print_error("Could not get a group for mpi://SELF. ", rc);
        return -1;
    }

    rc = MPI_Group_from_session_pset(session1, "mpi://SELF", &group3);
    if (MPI_SUCCESS != rc)
    {
        print_error("Could not get a group1 for mpi://SELF. ", rc);
        return -1;
    }

    MPI_Comm_create_from_group(group2, "myself", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm_self);
    MPI_Group_free(&group2);
    MPI_Allreduce(&one, &sum, 1, MPI_INT, MPI_SUM, comm_self);

    MPI_Comm_create_from_group(group3, "myself", MPI_INFO_NULL, MPI_ERRORS_RETURN, &comm_self1);
    MPI_Group_free(&group3);

    MPI_Errhandler_free(&errhandler);
    MPI_Info_free(&info);

    MPI_Comm_free(&comm_world);
    MPI_Comm_free(&comm_self);
    MPI_Session_finalize(&session);

    MPI_Allreduce(&one, &sum1, 1, MPI_INT, MPI_SUM, comm_world1);

    MPI_Allreduce(&one, &sum1, 1, MPI_INT, MPI_SUM, comm_self1);

    MPI_Comm_free(&comm_world1);
    MPI_Comm_free(&comm_self1);
    MPI_Session_finalize(&session1);

    return 0;
}
