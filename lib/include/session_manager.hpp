#ifndef SESSION_MANAGER_HPP
#define SESSION_MANAGER_HPP

#include <list>
#include <mutex>
#include <vector>
#include "mpi.h"

namespace legio {

class SessionManager
{
   public:
    SessionManager(SessionManager const&) = delete;
    SessionManager& operator=(SessionManager const&) = delete;
    SessionManager(SessionManager&&) = default;
    SessionManager& operator=(SessionManager&&) = default;
    SessionManager() = default;
    MPI_Comm get_horizon_comm(MPI_Group);

    void add_horizon_comm(MPI_Comm);

    void add_pending_session(MPI_Session session);
    void add_open_session();
    void close_session();
    void initialize();
    const bool is_initialized();

   private:
    std::mutex horizon_lock;
    std::mutex session_lock;
    std::mutex init_lock;
    std::vector<MPI_Session> pending;
    std::list<MPI_Comm> horizon_comms;
    int open_sessions = 0;
    bool initialized = false;
};

}  // namespace legio

#endif