#ifndef CONFIGURATION_H
#define CONFIGURATION_H

    #include "defaults.h"

    #define BROADCAST_FAIL_POLICY       FAULT_STOP
    #define SEND_FAIL_POLICY            FAULT_IGNORE
    #define NUM_RETRY                   3
    #define RECV_FAIL_POLICY            FAULT_STOP
    #define REDUCE_FAIL_POLICY          FAULT_IGNORE
    #define GET_FAIL_POLICY             FAULT_STOP
    #define PUT_FAIL_POLICY             FAULT_IGNORE






    #if BROADCAST_FAIL_POLICY == FAULT_IGNORE
        #define HANDLE_BCAST_FAIL(C) ({\
            rc = MPI_SUCCESS;\
            goto bcast_handling;})
    #else   
        #define HANDLE_BCAST_FAIL(C) raise(SIGINT)
    #endif

    #if SEND_FAIL_POLICY == FAULT_IGNORE
        #define HANDLE_SEND_FAIL(C) ({\
            rc = MPI_SUCCESS;\
            goto send_handling;})
    #else
        #define HANDLE_SEND_FAIL(C) raise(SIGINT)
    #endif

    #if RECV_FAIL_POLICY == FAULT_IGNORE
        #define HANDLE_RECV_FAIL(C) ({\
            rc = MPI_SUCCESS;\
            goto recv_handling;})
    #else
        #define HANDLE_RECV_FAIL(C) raise(SIGINT)
    #endif

    #if REDUCE_FAIL_POLICY == FAULT_IGNORE
        #define HANDLE_REDUCE_FAIL(C) ({\
            rc = MPI_SUCCESS;\
            goto reduce_handling;})
    #else   
        #define HANDLE_REDUCE_FAIL(C) raise(SIGINT)
    #endif

    #if GET_FAIL_POLICY == FAULT_IGNORE
        #define HANDLE_GET_FAIL(C) ({\
            rc = MPI_SUCCESS;\
            goto get_handling;})
    #else
        #define HANDLE_GET_FAIL(C) raise(SIGINT)
    #endif

    #if PUT_FAIL_POLICY == FAULT_IGNORE
        #define HANDLE_PUT_FAIL(C) ({\
            rc = MPI_SUCCESS;\
            goto put_handling;})
    #else
        #define HANDLE_PUT_FAIL(C) raise(SIGINT)
    #endif

#endif