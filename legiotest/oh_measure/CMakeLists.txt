add_executable(legio_oh_measure oh.c)
target_link_libraries(legio_oh_measure PUBLIC legio)

linkMPI(legio_oh_measure)