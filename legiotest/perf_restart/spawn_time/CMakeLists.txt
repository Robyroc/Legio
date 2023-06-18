
add_executable(timer time_to_spawn.cpp)
target_link_libraries(timer PUBLIC legio)

linkMPI(timer)