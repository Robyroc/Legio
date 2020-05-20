add_executable(legio_complex complex_computations.cpp)
target_link_libraries(legio_complex PUBLIC legio)

linkMPI(legio_complex)