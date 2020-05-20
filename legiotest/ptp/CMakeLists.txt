add_executable(legio_ptp scatter.c)
target_link_libraries(legio_ptp PUBLIC legio)

linkMPI(legio_ptp)