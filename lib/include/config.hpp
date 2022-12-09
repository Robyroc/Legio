#pragma once

#define BROADCAST_RESILIENCY 0
#define SEND_RESILIENCY 1
#define NUM_RETRY 3
#define RECV_RESILIENCY 0
#define REDUCE_RESILIENCY 1
#define GET_RESILIENCY 0
#define PUT_RESILIENCY 1
#define GATHER_RESILIENCY 0
#define GATHER_SHIFT 0
#define SCATTER_RESILIENCY 1
#define SCATTER_SHIFT 1

#define LOG_LEVEL 2
#define WITH_RESTART 1
#define CUBE_ALGORITHM 1

namespace legio {

enum class LogLevel { none = 1, errors_only = 2, errors_and_info = 3, full = 4 };

struct BuildOptions
{
    constexpr static bool broadcast_resiliency = static_cast<bool>(BROADCAST_RESILIENCY);
    constexpr static bool send_resiliency = static_cast<bool>(SEND_RESILIENCY);
    constexpr static int num_retry = NUM_RETRY;
    constexpr static bool recv_resiliency = static_cast<bool>(RECV_RESILIENCY);
    constexpr static bool reduce_resiliency = static_cast<bool>(REDUCE_RESILIENCY);
    constexpr static bool get_resiliency = static_cast<bool>(GET_RESILIENCY);
    constexpr static bool put_resiliency = static_cast<bool>(PUT_RESILIENCY);
    constexpr static bool gather_resiliency = static_cast<bool>(GATHER_RESILIENCY);
    constexpr static bool gather_shift = static_cast<bool>(GATHER_SHIFT);
    constexpr static bool scatter_resiliency = static_cast<bool>(SCATTER_RESILIENCY);
    constexpr static bool scatter_shift = static_cast<bool>(SCATTER_SHIFT);

    constexpr static LogLevel log_level = static_cast<LogLevel>(LOG_LEVEL);
    constexpr static bool with_restart = static_cast<bool>(WITH_RESTART);
    constexpr static bool cube_algorithm = static_cast<bool>(CUBE_ALGORITHM);
};

}  // namespace legio
