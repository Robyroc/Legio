#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include "config.hpp"
#include "multicomm.hpp"
#include "restart_manager.hpp"
#if WITH_SESSION
#include "session_manager.hpp"
#endif

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
#if WITH_SESSION
    SessionManager s_manager;
#endif
    Multicomm m_comm;
    RestartManager r_manager;

   private:
    Context() = default;
};

}  // namespace legio

#endif