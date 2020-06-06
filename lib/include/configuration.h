#ifndef CONFIGURATION_H
#define CONFIGURATION_H

    #define NUM_RETRY 3

    #if 2 == 1
        #define HANDLE_BCAST_FAIL(C) ({\
            return MPI_SUCCESS;})
    #else   
        #define HANDLE_BCAST_FAIL(C) printf("##### Broadcast failed, stopping a node\n"); raise(SIGINT)
    #endif

    #if 1 == 1
        #define HANDLE_SEND_FAIL(C) ({\
            return MPI_SUCCESS;})
    #else
        #define HANDLE_SEND_FAIL(C) printf("##### Send failed, stopping a node\n"); raise(SIGINT)
    #endif

    #if 2 == 1
        #define HANDLE_RECV_FAIL(C) ({\
            return MPI_SUCCESS;})
    #else
        #define HANDLE_RECV_FAIL(C) printf("##### Receive failed, stopping a node\n"); raise(SIGINT)
    #endif

    #if 1 == 1
        #define HANDLE_REDUCE_FAIL(C) ({\
            return MPI_SUCCESS;})
    #else   
        #define HANDLE_REDUCE_FAIL(C) printf("##### Reduce failed, stopping a node\n"); raise(SIGINT)
    #endif

    #if 2 == 1
        #define HANDLE_GET_FAIL(C) ({\
            return MPI_SUCCESS;})
    #else
        #define HANDLE_GET_FAIL(C) printf("##### Get failed, stopping a node\n"); raise(SIGINT)
    #endif

    #if 1 == 1
        #define HANDLE_PUT_FAIL(C) ({\
            return MPI_SUCCESS;})
    #else
        #define HANDLE_PUT_FAIL(C) printf("##### Put failed, stopping a node\n"); raise(SIGINT)
    #endif

    #if 1 == 1
        #define HANDLE_GATHER_FAIL(C) ({\
            return MPI_SUCCESS;})
    #else
        #define HANDLE_GATHER_FAIL(C) printf("##### Gather failed, stopping a node\n"); raise(SIGINT)
    #endif

    #if 2 == 1
        #define HANDLE_SCATTER_FAIL(C) ({\
            return MPI_SUCCESS;})
    #else
        #define HANDLE_SCATTER_FAIL(C) printf("##### Scatter failed, stopping a node\n"); raise(SIGINT)
    #endif


    #if 4 == 3
        #define PERFORM_GATHER(A, B, C, D, E, F, G, H, I, J, K) rc = PMPI_Gather(A, B, C, D, E, F, G, H)

    #else
        #define PERFORM_GATHER(SENDBUF, SENDCOUNT, SENDTYPE, RECVBUF, RECVCOUNT, RECVTYPE, ROOT, COMM, TOTALSIZE, FAKERANK, FAKECOMM) ({\
            int type_size, cur_rank;\
            MPI_Win win;\
            MPI_Type_size(RECVTYPE, &type_size);\
            MPI_Comm_rank(COMM, &cur_rank);\
            if(cur_rank == ROOT)\
                MPI_Win_create(RECVBUF, TOTALSIZE * RECVCOUNT * type_size, type_size, MPI_INFO_NULL, FAKECOMM, &win);\
            else\
                MPI_Win_create(RECVBUF, 0, type_size, MPI_INFO_NULL, FAKECOMM, &win);\
            MPI_Win_fence(0, win);\
            MPI_Put(SENDBUF, SENDCOUNT, SENDTYPE, ROOT, FAKERANK * RECVCOUNT, RECVCOUNT, RECVTYPE, win);\
            MPI_Win_fence(0, win);\
            MPI_Win_free(&win);\
            rc = MPI_SUCCESS;})
    #endif

    #if 4 == 3
        #define PERFORM_SCATTER(A, B, C, D, E, F, G, H, I, J, K) rc = PMPI_Scatter(A, B, C, D, E, F, G, H)

    #else
        #define PERFORM_SCATTER(SENDBUF, SENDCOUNT, SENDTYPE, RECVBUF, RECVCOUNT, RECVTYPE, ROOT, COMM, TOTALSIZE, FAKERANK, FAKECOMM) ({\
            int type_size, cur_rank;\
            MPI_Win win;\
            MPI_Type_size(RECVTYPE, &type_size);\
            MPI_Comm_rank(COMM, &cur_rank);\
            if(cur_rank == ROOT)\
                MPI_Win_create((void *) SENDBUF, TOTALSIZE * SENDCOUNT * type_size, type_size, MPI_INFO_NULL, FAKECOMM, &win);\
            else\
                MPI_Win_create((void *) SENDBUF, 0, type_size, MPI_INFO_NULL, FAKECOMM, &win);\
            MPI_Win_fence(0, win);\
            MPI_Get(RECVBUF, RECVCOUNT, RECVTYPE, ROOT, FAKERANK * SENDCOUNT, SENDCOUNT, SENDTYPE, win);\
            MPI_Win_fence(0, win);\
            MPI_Win_free(&win);\
            rc = MPI_SUCCESS;})
    #endif



#endif
