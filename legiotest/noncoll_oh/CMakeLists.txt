add_executable(legio_noncoll noncoll_oh.c)
target_link_libraries(legio_noncoll PUBLIC legio)

linkMPI(legio_noncoll)