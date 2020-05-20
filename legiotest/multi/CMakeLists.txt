add_executable(legio_multi multi.cpp)
target_link_libraries(legio_multi PUBLIC legio)

linkMPI(legio_multi)