add_executable(vs_shrink algo.cpp)

target_link_libraries(vs_shrink PUBLIC legio)

#target_include_directories(vs_shrink PUBLIC ${LEGIO_HDR_FOLDER})

linkMPI(vs_shrink)