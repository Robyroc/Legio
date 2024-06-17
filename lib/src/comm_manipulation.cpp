#include "comm_manipulation.hpp"
#include <numeric>
#include <shared_mutex>
#include <sstream>
#include <thread>
#include "complex_comm.hpp"
#include "context.hpp"
#include "log.hpp"
#include "mpi.h"
#include "restart_routines.hpp"
#include "utils.hpp"
extern "C" {
#include "legio.h"
}

#include "mpi-ext.h"

extern std::shared_timed_mutex failure_mtx;
using namespace legio;

// Far capire al processo respawnato il communicatore
// Necessario
// Quando il processo viene spawnato, sa il suo rank ma non sa i comunicatori
// Sa mpi_comm_world, e mpi comm self
// Assumiamo di salvarci un sottogruppo di cur_comms con solo i dati dei comm che ci interessano
// Dopo il restart duplica mpi_comm_self per creare comm da sostituire (con alternativa
// MPI_COMM_NULL) Dopo fare le chiamate dara' un'errore di MPI_COMM_NULL -> fara' una recv su
// MPI_COMM_WORLD da chiunque
void legio::initialization(int* argc, char*** argv)
{
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);
    MPI_Comm_set_errhandler(MPI_COMM_SELF, MPI_ERRORS_RETURN);
    int size, rank;
    std::vector<int> failed;

    if constexpr (BuildOptions::with_restart)
    {
        if (command_line_option_exists(argc, argv, "--respawned"))
        {
            size = std::stoi(get_command_line_option(argc, argv, "--size"));
            char* possibly_null_failed = get_command_line_option(argc, argv, "--failed-ranks");
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

            rank = std::stoi(get_command_line_option(argc, argv, "--rank"));
            // printf("PARSED RANK: %d\n", rank);
            // cur_comms = new RespawnMulticomm(size, rank, failed);
            Context::get().r_manager.initialize(size, rank, failed);
        }
        else
        {
            PMPI_Comm_size(MPI_COMM_WORLD, &size);
            Context::get().r_manager.initialize(size);
        }

        char* possibly_null_to_respawn = get_command_line_option(argc, argv, "--to-respawn");
        if (possibly_null_to_respawn != 0)
        {
            std::string raw_to_respawn = possibly_null_to_respawn;
            std::stringstream ss(raw_to_respawn);
            while (ss.good())
            {
                std::string substr;

                getline(ss, substr, ',');
                legio::log(substr.c_str(), LogLevel::full);
                Context::get().r_manager.add_to_respawn_list(std::stoi(substr));
            }
        }
    }
    else
    {
#if WITH_SESSION
        MPI_Comm temp;
        MPI_Session temp_session;
        MPI_Group temp_group;
        PMPI_Session_init(MPI_INFO_NULL, MPI_ERRORS_RETURN, &temp_session);
        PMPI_Group_from_session_pset(temp_session, "mpi://WORLD", &temp_group);
        PMPI_Comm_create_from_group(temp_group, "Legio_horizon_construction", MPI_INFO_NULL,
                                    MPI_ERRORS_RETURN, &temp);
        PMPI_Group_free(&temp_group);
        Context::get().s_manager.add_pending_session(temp_session);
        Context::get().s_manager.add_open_session();
        Context::get().s_manager.initialize();
        Context::get().s_manager.add_horizon_comm(temp);
#endif
    }

    Context::get().m_comm.add_comm(MPI_COMM_SELF);
    Context::get().m_comm.add_comm(MPI_COMM_WORLD);

    if constexpr (BuildOptions::with_restart)
    {
        if (Context::get().r_manager.is_respawned())
        {
            int rank = Context::get().r_manager.get_own_rank();
            restart(rank);
        }
    }
}

void legio::finalization()
{
#if WITH_SESSION
    Context::get().s_manager.close_session();
#endif
}

void legio::replace_comm(ComplexComm& cur_complex)
{
    if constexpr (BuildOptions::with_restart)
        if (!Context::get().r_manager.get_respawn_list().empty())
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
        MPIX_Comm_failure_ack(cur_complex.get_alias());
        MPI_Comm_set_errhandler(new_comm, MPI_ERRORS_RETURN);
        cur_complex.replace_comm(new_comm);
    }
}

void legio::replace_and_repair_comm(ComplexComm& cur_complex)
{
    MPI_Comm new_comm, world;
    MPI_Group group;
    int old_size, new_size, failed, ranks[LEGIO_MAX_FAILS], i, rank, current_rank;
    std::set<int> failed_ranks_set;

    ComplexComm& world_complex = Context::get().m_comm.translate_into_complex(MPI_COMM_WORLD);
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

void legio::agree_and_eventually_replace(int* rc, ComplexComm& cur_complex)
{
    int flag = (MPI_SUCCESS == *rc);
    MPIX_Comm_agree(cur_complex.get_comm(), &flag);
    if (!flag && *rc == MPI_SUCCESS)
        *rc = MPIX_ERR_PROC_FAILED;
    if (*rc != MPI_SUCCESS)
        replace_comm(cur_complex);
}

int legio::translate_ranks(const int rank, ComplexComm& comm)
{
    if constexpr (BuildOptions::with_restart)
        return Context::get().r_manager.translate_ranks(rank, comm);
    else
    {
        MPI_Group tr_group;
        int source = rank, dest_rank;
        MPI_Comm_group(comm.get_comm(), &tr_group);
        MPI_Group_translate_ranks(comm.get_group(), 1, &source, tr_group, &dest_rank);
        return dest_rank;
    }
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