add_executable(legio_restart restart.cpp)

target_link_libraries(legio_restart PUBLIC legio)

linkMPI(legio_restart)