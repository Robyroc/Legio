include("${CMAKE_CURRENT_SOURCE_DIR}/include/defaults.cmake")

function(defaultValue VAR VALUE)
    if(NOT DEFINED ${VAR})
        set(${VAR} ${VALUE} PARENT_SCOPE)
    endif()
endfunction(defaultValue)

defaultValue(BROADCAST_FAIL_POLICY ${FAULT_STOP})
defaultValue(SEND_FAIL_POLICY ${FAULT_IGNORE})
defaultValue(NUM_RETRY 3)
defaultValue(RECV_FAIL_POLICY ${FAULT_STOP})
defaultValue(REDUCE_FAIL_POLICY ${FAULT_IGNORE})
defaultValue(GET_FAIL_POLICY ${FAULT_STOP})
defaultValue(PUT_FAIL_POLICY ${FAULT_IGNORE})
defaultValue(GATHER_FAIL_POLICY ${FAULT_IGNORE})
defaultValue(GATHER_APPROACH ${RANK_KEEP})
defaultValue(SCATTER_FAIL_POLICY ${FAULT_STOP})
defaultValue(SCATTER_APPROACH ${RANK_KEEP})

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/configuration.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/configuration.h"
)

add_library(legio
        src/coll.cpp 
        src/comm_manipulation.cpp
        src/complex_comm.cpp
        src/fileio.cpp
        src/general.cpp
        src/legio.c
        src/multicomm.cpp
        src/osc.cpp
        src/ptp.cpp
        include/comm_manipulation.h
        include/complex_comm.h
        include/configuration.h
        include/legio.h
        include/structure_handler.h
        include/multicomm.h)

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
