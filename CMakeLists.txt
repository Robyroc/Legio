cmake_minimum_required(VERSION 3.10)

project(Legio)

set(CMAKE_CXX_FLAGS -std=c++11)

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
    set(MPI_C_COMPILER ${MPICC})
    set(CMAKE_C_COMPILER ${MPI_C_COMPILER})

    message("[legio] MPICC has been passed: ${MPICC}")
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

