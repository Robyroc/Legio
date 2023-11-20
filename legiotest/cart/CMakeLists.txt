add_executable(legio_cart cart.c)
target_link_libraries(legio_cart PUBLIC legio)

linkMPI(legio_cart)