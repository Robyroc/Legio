add_executable(legio_osc window.c)
target_link_libraries(legio_osc PUBLIC legio)

linkMPI(legio_osc)