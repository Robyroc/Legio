add_executable(legio_request request.cpp)
target_link_libraries(legio_request PUBLIC legio)

linkMPI(legio_request)