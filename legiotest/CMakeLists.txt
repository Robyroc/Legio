
add_executable(legio_coll coll/gather.c)
target_link_libraries(legio_coll PUBLIC legio)

add_executable(legio_file file/file.c)
target_link_libraries(legio_file PUBLIC legio)

add_executable(legio_osc osc/window.c)
target_link_libraries(legio_osc PUBLIC legio)

add_executable(legio_ptp ptp/scatter.c)
target_link_libraries(legio_ptp PUBLIC legio)

linkMPI(legio_coll)
linkMPI(legio_file)
linkMPI(legio_osc)
linkMPI(legio_ptp)