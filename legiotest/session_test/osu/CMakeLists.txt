add_executable(osu_bibw "${CMAKE_CURRENT_SOURCE_DIR}/osu_util.h"
                        "${CMAKE_CURRENT_SOURCE_DIR}/osu_util_mpi.h"
                        "${CMAKE_CURRENT_SOURCE_DIR}/osu_util.c"
                        "${CMAKE_CURRENT_SOURCE_DIR}/osu_util_mpi.c"
                        "${CMAKE_CURRENT_SOURCE_DIR}/osu_bibw_sessions.c")

target_link_libraries(osu_bibw PUBLIC legio)

linkMPI(osu_bibw)