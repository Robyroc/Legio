set(LEGIO_HDR_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/include")
set(LEGIO_SRC_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/src")

add_executable(algo_test 
                "${LEGIO_HDR_FOLDER}/intercomm_utils.hpp"
                "${LEGIO_SRC_FOLDER}/intercomm_utils.cpp"
                algo.cpp)

target_include_directories(algo_test PUBLIC ${LEGIO_HDR_FOLDER})

linkMPI(algo_test)