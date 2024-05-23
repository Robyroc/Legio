#include <signal.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include "context.hpp"

extern "C" {
    void raise_sigint() {
        raise(SIGINT);
    }

    // MPI_INIT
    void my_MPI_Init(MPI_Fint comm_w, MPI_Fint comm_s, int *ierr){
        MPI_Comm c_comm_w = MPI_Comm_f2c(comm_w);
        MPI_Comm c_comm_s = MPI_Comm_f2c(comm_s);
        MPI_Comm_set_errhandler(c_comm_w, MPI_ERRORS_RETURN);
        MPI_Comm_set_errhandler(c_comm_s, MPI_ERRORS_RETURN);

        Context::get().m_comm.add_comm(MPI_COMM_SELF);
        Context::get().m_comm.add_comm(MPI_COMM_WORLD);
    }

    // MPI_COMM_RANK 
    // checked
    void my_MPI_Comm_rank(MPI_Fint comm, int *rank, int *ierr){
        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Comm_rank(c_comm, rank);
    }

    // MPI_BARRIER
    // checked
    void my_MPI_Barrier(MPI_Fint comm, int *ierr){
        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Barrier(c_comm);
    }

    // MPI_ABORT
    void my_MPI_Abort(MPI_Fint comm, int errorcode, int *ierr){
        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Abort(c_comm, errorcode);
    }

    // MPI_COMM_SIZE
    void my_MPI_Comm_size(MPI_Fint comm, int *size, int *ierr){
        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Comm_size(c_comm, size);
    }

    // MPI_WAITALL
    void my_MPI_Waitall(int count, MPI_Fint *array_of_requests, MPI_Fint *array_of_statuses, int *ierr) {
        // if the statuses are the output of the function you might want to 
        // define them as c type inside the function and then convert back to fortran
        // after the mpi call
        // just like the my_mpi_Wait function
        printf("WAIT ");
        MPI_Request *cmpi_requests = (MPI_Request *)malloc(count * sizeof(MPI_Request));
        MPI_Status *cmpi_statuses = (MPI_Status *)malloc(count * sizeof(MPI_Status));
        MPI_Status c_status;
        
        // Convert Fortran MPI handles to C MPI handles
        for (int i = 0; i < count; ++i) {
            cmpi_requests[i] = MPI_Request_f2c(array_of_requests[i]);
            MPI_Status_f2c(&array_of_statuses[i], &c_status);
            cmpi_statuses[i] = c_status;
        }
        
        // Call MPI_Waitall
        int c = MPI_Waitall(count, cmpi_requests, cmpi_statuses);
        printf("%d, ", c);

        // convert c requests back to fortran
        for (int i = 0; i < count; ++i) {
            MPI_Status_c2f(&cmpi_statuses[i], &array_of_statuses[i]);
            array_of_requests[i] = MPI_Request_c2f(cmpi_requests[i]);
        }
        
        // Free allocated memory
        free(cmpi_requests);
        free(cmpi_statuses);
    }


    // MPI_ISEND
    void my_MPI_Isend(void *buf, int count, MPI_Fint datatype, int *dest,
                    int tag, MPI_Fint comm, MPI_Fint *request, int *ierr){
        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Datatype c_datatype = MPI_Type_f2c(datatype);
        MPI_Request c_request;
        *ierr = MPI_Isend(buf, count, c_datatype, *dest, tag, c_comm, &c_request);
        *request = MPI_Request_c2f(c_request);
        
    }

    // MPI_IRECV
    void my_MPI_Irecv(void *buf, int count, MPI_Fint datatype, int *source, 
                    int tag, MPI_Fint comm, MPI_Fint *request, int *ierr){
        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Datatype c_datatype = MPI_Type_f2c(datatype);
        MPI_Request c_request;
        *ierr = MPI_Irecv(buf, count, c_datatype, *source, tag, c_comm, &c_request);
        *request = MPI_Request_c2f(c_request);
    }

    // MPI_WAIT
    void my_MPI_Wait(MPI_Fint request, MPI_Fint status, int *ierr){
        MPI_Request c_request = MPI_Request_f2c(request);
        MPI_Status c_status;
        MPI_Status_f2c(&status, &c_status);
        MPI_Wait(&c_request, &c_status);
    }

    // MPI_GATHER
    void my_MPI_Gather(void *sendbuf, int sendcount, MPI_Fint sendtype,
                void *recvbuf, int recvcount, MPI_Fint recvtype,
                int root, MPI_Fint comm, int *ierr){

        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Datatype c_send_datatype = MPI_Type_f2c(sendtype);
        MPI_Datatype c_rec_datatype = MPI_Type_f2c(recvtype);

        MPI_Gather(sendbuf, sendcount, c_send_datatype, recvbuf, 
                    recvcount, c_rec_datatype, root, c_comm);
        
    }

    // MPI_ALLGATHER
    void my_MPI_Allgather(void *sendbuf, int sendcount, MPI_Fint sendtype,
                    void *recvbuf, int recvcount, MPI_Fint recvtype,
                    MPI_Fint comm, int *ierr){

        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Datatype c_send_datatype = MPI_Type_f2c(sendtype);
        MPI_Datatype c_rec_datatype = MPI_Type_f2c(recvtype);
        
        *ierr = MPI_Allgather(sendbuf, sendcount, c_send_datatype, recvbuf, 
                    recvcount, c_rec_datatype, c_comm);
        }

    // MPI_REDUCE
    void my_MPI_Reduce(void *sendbuf, void *recvbuf, int count, 
                    MPI_Fint datatype, MPI_Fint op, int root, MPI_Fint comm, int *ierr){
        
        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Datatype c_datatype = MPI_Type_f2c(datatype);
        MPI_Op c_op = MPI_Op_f2c(op);

        *ierr = MPI_Reduce(sendbuf, recvbuf, count, c_datatype, c_op, root, c_comm);
    }

    // MPI_ALLREDUCE

    void my_MPI_Allreduce(void *sendbuf, void *recvbuf, int count, 
                    MPI_Fint datatype, MPI_Fint op, MPI_Fint comm, int *ierr){

        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Datatype c_datatype = MPI_Type_f2c(datatype);
        MPI_Op c_op = MPI_Op_f2c(op);

        * ierr = MPI_Allreduce(sendbuf, recvbuf, count, c_datatype, c_op, c_comm);
    }


    void my_MPI_Sendrecv(void* sendbuf, int sendcount, MPI_Fint sendtype,
                        int *dest, int sendtag, void* recvbuf, int recvcount,
                        MPI_Fint recvtype, int *source, int recvtag,
                        MPI_Fint comm, MPI_Fint status){

        MPI_Comm c_comm = MPI_Comm_f2c(comm);
        MPI_Datatype c_sendtype = MPI_Type_f2c(sendtype);
        MPI_Datatype c_recvtype = MPI_Type_f2c(recvtype);

        MPI_Status c_status;
        MPI_Status_f2c(&status, &c_status);

        MPI_Sendrecv(sendbuf, sendcount, c_sendtype, *dest, sendtag,
                        recvbuf, recvcount, c_recvtype, *source, recvtag,
                        c_comm, &c_status);

    }

}