#include "comm_manipulation.h"
#include <mpi.h>
#include <mpi-ext.h>
#include "complex_comm.h"
#include "respawn_multicomm.h"
#include "multicomm.h"
#include "utils.cpp"
#include "restart.h"
#include "legio.c"
#include <sstream>
#include <numeric>
#include <shared_mutex>
#include <thread>
#include <iostream>
//#include <thread>

extern Multicomm *cur_comms;
extern std::shared_timed_mutex failure_mtx;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;
//std::thread * kalive;
//int temp;

// Far capire al processo respawnato il communicatore
// Necessario 
// Quando il processo viene spawnato, sa il suo rank ma non sa i comunicatori
// Sa mpi_comm_world, e mpi comm self
// Assumiamo di salvarci un sottogruppo di cur_comms con solo i dati dei comm che ci interessano
// Dopo il restart duplica mpi_comm_self per creare comm da sostituire (con alternativa MPI_COMM_NULL)
// Dopo fare le chiamate dara' un'errore di MPI_COMM_NULL -> fara' una recv su MPI_COMM_WORLD da chiunque
void initialization(int* argc, char *** argv)
{
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    int size, rank;
    std::vector<int> failed;
        
    if (command_line_option_exists(*argc, *argv, "--respawned")) {

        size = std::stoi(get_command_line_option(*argc, *argv, "--size"));
        char* possibly_null_failed = get_command_line_option(*argc, *argv, "--failed-ranks");
        if (possibly_null_failed != 0) {
            std::string raw_failed = possibly_null_failed;
            std::stringstream ss( raw_failed );
            while( ss.good() )
            {
                std::string substr;
                getline( ss, substr, ',' );
                cur_comms->set_failed_rank(std::stoi(substr));
            }
        }

        rank = std::stoi(get_command_line_option(*argc, *argv, "--rank"));
        printf("PARSED RANK: %d\n", rank);
        cur_comms = new RespawnMulticomm(size, rank, failed);
        cur_comms->respawned = true;
    }
    else {
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        cur_comms = new Multicomm(size);
        cur_comms->respawned = false;
    }

    char* possibly_null_to_respawn = get_command_line_option(*argc, *argv, "--to-respawn");
    if (possibly_null_to_respawn != 0) {

        std::string raw_to_respawn = possibly_null_to_respawn;
        std::stringstream ss( raw_to_respawn );
        while( ss.good() )
        {
            std::string substr;
            
            getline( ss, substr, ',' );
                            std::cout << substr; fflush(stdout);
            cur_comms->to_respawn.push_back(std::stoi(substr));
        }
    }
    else {
        cur_comms->to_respawn = {};
    }

    cur_comms->add_comm(
        MPI_COMM_SELF,
        MPI_COMM_NULL,
        [](MPI_Comm a, MPI_Comm* dest) -> int
        {
            *dest = MPI_COMM_SELF;
            MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
            return MPI_SUCCESS;
        });
    cur_comms->add_comm(
        MPI_COMM_WORLD,
        MPI_COMM_NULL,
        [](MPI_Comm a, MPI_Comm* dest) -> int 
        {
            *dest = MPI_COMM_WORLD;
            MPI_Comm_set_errhandler(*dest, MPI_ERRORS_RETURN);
            return MPI_SUCCESS;
        }
    );

    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
    if (cur_comms->respawned) {
        char* rank = get_command_line_option(*argc, *argv, "--rank");
        restart(atoi(rank));
    }
}

void finalization()
{
    delete cur_comms;
}

void replace_comm(ComplexComm* cur_complex)
{
    MPI_Comm new_comm, world;
    MPI_Group group;
    int old_size, new_size, failed, ranks[LEGIO_MAX_FAILS], i, rank, current_rank;

    ComplexComm *world_complex = cur_comms->translate_into_complex(MPI_COMM_WORLD);
    MPIX_Comm_failure_ack(cur_complex->get_comm());
    who_failed(cur_complex->get_comm(), &failed, ranks);

    if(0 == failed) {
        PMPI_Comm_free(&new_comm);
    }
    else
    {    
        if (VERBOSE)
        {
            int rank, size;
            MPI_Comm_size(MPI_COMM_WORLD, &size);
            MPI_Comm_rank(MPI_COMM_WORLD, &rank);
            printf("[is_respawned: %d] Detected failure in rank %d / %d.\n", is_respawned(), rank, size);
        }
        // TODO(low-priority): skip communication with thread if only not-to-restart process
        MPI_Group world_group, not_failed_group;
        MPI_Comm not_failed_comm;
        int buf = LEGIO_FAILURE_PING_VALUE, not_failed_size;
        std::vector<int> not_failed_group_ranks, new_group_ranks;

        // Communicate to all not failed ranks that they must start restart procedure
        PMPI_Comm_group(world_complex->get_comm(), &world_group);
        PMPI_Group_excl(world_group, failed, ranks, &not_failed_group);
        PMPI_Group_size(not_failed_group, &not_failed_size);
        PMPI_Comm_create_group(world_complex->get_comm(), not_failed_group, 1, &not_failed_comm);
        PMPI_Comm_rank(not_failed_comm, &current_rank);
        for (int i = 0; i < not_failed_size; i++) {
            printf("NOT FAILED SIZE %d\nCURRENT RANK %d\n", not_failed_size, current_rank); fflush(stdout);
            if (i == current_rank)
                continue;
            if (VERBOSE)
                {
                    int rank, size;
                    MPI_Comm_size(MPI_COMM_WORLD, &size);
                    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                    printf("Rank %d / %d sending failure notification to not failed rank (new index %d) .\n", rank, size, i); fflush(stdout);
                }
            PMPI_Send(&buf, 1, MPI_INT, i, LEGIO_FAILURE_TAG, not_failed_comm);
        }
        failure_mtx.lock();
        repair_failure();
        failure_mtx.unlock();
    }
}

void agree_and_eventually_replace(int* rc, ComplexComm* cur_complex)
{
    int flag = (MPI_SUCCESS==*rc);
    MPIX_Comm_agree(cur_complex->get_comm(), &flag);
    if(!flag && *rc == MPI_SUCCESS)
        *rc = MPIX_ERR_PROC_FAILED;
    if(*rc != MPI_SUCCESS)
        replace_comm(cur_complex);
}
/*
void receiver()
{
    MPI_Recv(&temp, 1, MPI_INT, 0, 75, MPI_COMM_SELF, MPI_STATUS_IGNORE);
}

void sender()
{
    int i = 0;
    MPI_Send(&i, 1, MPI_INT, 0, 75, MPI_COMM_SELF);
}

void kalive_thread()
{
    kalive = new std::thread(receiver);
}

void kill_kalive_thread()
{
    sender();
}
*/