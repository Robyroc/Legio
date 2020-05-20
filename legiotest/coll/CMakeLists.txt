add_executable(legio_coll gather.c)
target_link_libraries(legio_coll PUBLIC legio)

linkMPI(legio_coll)