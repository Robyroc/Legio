#include "comm_manipulation.hpp"
#include <mpi.h>
#include <iostream>
#include <numeric>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include "complex_comm.hpp"
#include "mpi-ext.h"
extern "C" {
#include "legio.h"
#include "restart.h"
}
#include "multicomm.hpp"
//#include "respawn_multicomm.h"
#include "utils.hpp"
//#include <thread>

extern std::shared_timed_mutex failure_mtx;
extern int VERBOSE;
extern char errstr[MPI_MAX_ERROR_STRING];
extern int len;
// std::thread * kalive;
// int temp;

// Far capire al processo respawnato il communicatore
// Necessario
// Quando il processo viene spawnato, sa il suo rank ma non sa i comunicatori
// Sa mpi_comm_world, e mpi comm self
// Assumiamo di salvarci un sottogruppo di cur_comms con solo i dati dei comm che ci interessano
// Dopo il restart duplica mpi_comm_self per creare comm da sostituire (con alternativa
// MPI_COMM_NULL) Dopo fare le chiamate dara' un'errore di MPI_COMM_NULL -> fara' una recv su
// MPI_COMM_WORLD da chiunque
void initialization(int* argc, char*** argv)
{
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    int size, rank;
    std::vector<int> failed;

    if (command_line_option_exists(*argc, *argv, "--respawned"))
    {
        size = std::stoi(get_command_line_option(*argc, *argv, "--size"));
        char* possibly_null_failed = get_command_line_option(*argc, *argv, "--failed-ranks");
        if (possibly_null_failed != 0)
        {
            std::string raw_failed = possibly_null_failed;
            std::stringstream ss(raw_failed);
            while (ss.good())
            {
                std::string substr;
                getline(ss, substr, ',');
                // cur_comms->set_failed_rank(std::stoi(substr));
                failed.push_back(std::stoi(substr));
            }
        }

        rank = std::stoi(get_command_line_option(*argc, *argv, "--rank"));
        // printf("PARSED RANK: %d\n", rank);
        // cur_comms = new RespawnMulticomm(size, rank, failed);
        Multicomm::get_instance().initialize(size, rank, failed);
    }
    else
    {
        PMPI_Comm_size(MPI_COMM_WORLD, &size);
        Multicomm::get_instance().initialize(size);
    }

    char* possibly_null_to_respawn = get_command_line_option(*argc, *argv, "--to-respawn");
    if (possibly_null_to_respawn != 0)
    {
        std::string raw_to_respawn = possibly_null_to_respawn;
        std::stringstream ss(raw_to_respawn);
        while (ss.good())
        {
            std::string substr;

            getline(ss, substr, ',');
            std::cout << substr;
            fflush(stdout);
            Multicomm::get_instance().add_to_respawn_list(std::stoi(substr));
        }
    }

    Multicomm::get_instance().add_comm(MPI_COMM_SELF);
    Multicomm::get_instance().add_comm(MPI_COMM_WORLD);

    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
    if (Multicomm::get_instance().is_respawned())
    {
        char* rank = get_command_line_option(*argc, *argv, "--rank");
        restart(atoi(rank));
    }
}

void finalization() {}

void replace_comm(ComplexComm& cur_complex)
{
    if (!Multicomm::get_instance().get_respawn_list().empty())
        return replace_and_repair_comm(cur_complex);
    MPI_Comm new_comm;
    int old_size, new_size, diff;
    MPIX_Comm_shrink(cur_complex.get_comm(), &new_comm);
    MPI_Comm_size(cur_complex.get_comm(), &old_size);
    MPI_Comm_size(new_comm, &new_size);
    diff = old_size - new_size; /* number of deads */
    if (0 == diff)
        PMPI_Comm_free(&new_comm);
    else
    {
        MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
        cur_complex.replace_comm(new_comm);
    }
}

void replace_and_repair_comm(ComplexComm& cur_complex)
{
    MPI_Comm new_comm, world;
    MPI_Group group;
    int old_size, new_size, failed, ranks[LEGIO_MAX_FAILS], i, rank, current_rank;
    std::set<int> failed_ranks_set;

    ComplexComm& world_complex = Multicomm::get_instance().translate_into_complex(MPI_COMM_WORLD);
    MPIX_Comm_failure_ack(world_complex.get_comm());
    who_failed(world_complex.get_comm(), &failed, ranks);

    if (failed != 0)
    {
        // if (VERBOSE)
        // {
        //     int rank, size;
        //     MPI_Comm_size(MPI_COMM_WORLD, &size);
        //     MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        //     printf("[is_respawned: %d] Detected failure in rank %d / %d.\n", is_respawned(),
        //     rank,
        //            size);
        // }
        for (i = 0; i < failed; i++)
        {
            failed_ranks_set.insert(ranks[i]);
        }
        MPI_Group world_group, not_failed_group, cur_group, to_be_notified_group;
        MPI_Comm not_failed_comm;
        int buf = LEGIO_FAILURE_PING_VALUE, notify_size, cur_rank;
        std::vector<int> not_failed_group_ranks, new_group_ranks;

        // Communicate to all not failed ranks that they must start restart procedure
        PMPI_Comm_group(world_complex.get_comm(), &world_group);
        PMPI_Comm_group(cur_complex.get_comm(), &cur_group);
        PMPI_Group_excl(world_group, failed, ranks, &not_failed_group);
        PMPI_Group_difference(not_failed_group, cur_group, &to_be_notified_group);
        PMPI_Group_size(to_be_notified_group, &notify_size);
        PMPI_Comm_rank(world_complex.get_comm(), &cur_rank);
        if (notify_size > 0)
        {
            MPI_Request* requests = (MPI_Request*)malloc(sizeof(MPI_Request) * notify_size);
            MPI_Status* statuses = (MPI_Status*)malloc(sizeof(MPI_Status) * notify_size);
            for (int i = 0; i < notify_size; i++)
            {
                int target_rank;
                MPI_Group_translate_ranks(to_be_notified_group, 1, &i, world_group, &target_rank);
                if (VERBOSE)
                {
                    int rank, size;
                    MPI_Comm_size(MPI_COMM_WORLD, &size);
                    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
                    printf("Rank %d / %d sending failure notification to not failed rank %d.\n",
                           rank, size, target_rank);
                    fflush(stdout);
                }
                PMPI_Issend(&buf, 1, MPI_INT, target_rank, LEGIO_FAILURE_TAG,
                            world_complex.get_comm(), &(requests[i]));
            }
            MPI_Waitall(notify_size, requests, statuses);
            free(requests);
            free(statuses);
        }

        // PMPI_Group_size(not_failed_group, &not_failed_size);
        // PMPI_Comm_create_group(world_complex->get_comm(), not_failed_group, 1, &not_failed_comm);
        // PMPI_Comm_rank(not_failed_comm, &current_rank);
        failure_mtx.lock();
        repair_failure();
        failure_mtx.unlock();
    }
}

void agree_and_eventually_replace(int* rc, ComplexComm& cur_complex)
{
    int flag = (MPI_SUCCESS == *rc);
    MPIX_Comm_agree(cur_complex.get_comm(), &flag);
    if (!flag && *rc == MPI_SUCCESS)
        *rc = MPIX_ERR_PROC_FAILED;
    if (*rc != MPI_SUCCESS)
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