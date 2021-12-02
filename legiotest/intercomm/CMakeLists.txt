add_executable(legio_intercomm intercomm.cpp)
target_link_libraries(legio_intercomm PUBLIC legio)

linkMPI(legio_intercomm)