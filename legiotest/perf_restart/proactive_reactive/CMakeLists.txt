set(LEGIO_HDR_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/include")

add_executable(proactive pro_react.cpp)
target_link_libraries(proactive PUBLIC legio)

linkMPI(timer)