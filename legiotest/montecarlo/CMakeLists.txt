add_executable(legio_montecarlo montecarlo.c)
target_link_libraries(legio_montecarlo PUBLIC legio)

linkMPI(legio_montecarlo)