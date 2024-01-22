add_executable(legio_cart_rank rank.c)
target_link_libraries(legio_cart_rank PUBLIC legio)

linkMPI(legio_cart_rank)

add_executable(legio_cart cart.cpp)
target_link_libraries(legio_cart PUBLIC legio)

linkMPI(legio_cart)