add_executable(legio_rcomms restartcomms.c)
target_link_libraries(legio_rcomms PUBLIC legio)

linkMPI(legio_rcomms)