
add_library(legio
        src/coll.cpp 
        src/comm_manipulation.cpp
        src/complex_comm.cpp
        src/fileio.cpp
        src/legio.c
        src/osc.cpp
        src/ptp.cpp
        include/comm_manipulation.h
        include/complex_comm.h
        include/configuration.h
        include/defaults.h
        include/legio.h
        include/structure_handler.h)

target_include_directories(legio PUBLIC ./include)
target_compile_features(legio PUBLIC cxx_std_11)
set_target_properties(legio PROPERTIES CXX_EXTENSIONS OFF)
linkMPI(legio)

target_link_libraries(legio ${MPI_C_LIBRARIES})
if(MPI_COMPILE_FLAGS)
    set_target_properties(legio PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(legio PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

