cmake_minimum_required(VERSION 3.10)

project(Legio)

set(LEGIO_VERSION_MAJOR 0)
set(LEGIO_VERSION_MINOR 1)

set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(MPICC $ENV{MPICC})
if("a${MPICC}" STREQUAL "a")
    execute_process(
        COMMAND bash -c "cd .. && make ulfm"
        OUTPUT_VARIABLE outVar
    )
    set(MPICC ${outVar})
endif()
#set(CMAKE_BUILD_TYPE Release)

if(NOT "a${MPICC}" STREQUAL "a")
    string(REPLACE "\n" "" MPI_C_COMPILER ${MPICC})
    set(CMAKE_C_COMPILER ${MPI_C_COMPILER})
    string(REPLACE "mpicc" "mpicxx" MPI_CXX_COMPILER ${MPI_C_COMPILER})
    set(CMAKE_CXX_COMPILER ${MPI_CXX_COMPILER})
    
    message("[legio] MPICC has been passed: ${MPICC}")
    message("[legio] MPICXX found: ${MPI_CXX_COMPILER}")
else()
    message("[legio] MPICC was not passed, searching for MPI")
    find_package(MPI REQUIRED)
    if(${MPI_C_FOUND})
        message("[legio] Found MPICC: ${MPI_C_COMPILER}")
    else()
        message(FATAL_ERROR "[fenix] MPI not found! Try ../make ulfm")
    endif()
endif()

function(linkMPI TOLINK)
    if("a${MPICC}" STREQUAL "a")
        target_link_libraries(${TOLINK} PUBLIC MPI::MPI_C)
    endif()
endfunction(linkMPI)

add_subdirectory(lib)
add_subdirectory(legiotest)

