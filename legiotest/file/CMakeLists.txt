add_executable(legio_file file.c)
target_link_libraries(legio_file PUBLIC legio)

linkMPI(legio_file)