add_executable(tiny_mc tiny_mc.c)
target_link_libraries(tiny_mc PUBLIC m)
target_link_libraries(tiny_mc PUBLIC legio)

add_executable(tiny_mc_adj tiny_mc_adj.c)
target_link_libraries(tiny_mc_adj PUBLIC m)
target_link_libraries(tiny_mc_adj PUBLIC legio)

linkMPI(tiny_mc)
linkMPI(tiny_mc_adj)