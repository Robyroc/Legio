#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "multicomm.hpp"
#include "restart_manager.hpp"
#include "session_manager.hpp"

namespace legio {
class Context
{
   public:
    Context(Context const&) = delete;
    Context& operator=(Context const&) = delete;
    Context(Context&&) = default;
    Context& operator=(Context&&) = default;
    static Context& get()
    {
        static Context instance;
        return instance;
    }
    SessionManager s_manager;
    Multicomm m_comm;
    RestartManager r_manager;

   private:
    Context() = default;
};

}  // namespace legio

#endif