
add_subdirectory(coll)

add_subdirectory(complex)

add_subdirectory(file)

add_subdirectory(multi)

add_subdirectory(osc)

add_subdirectory(ptp)

add_subdirectory(oh_measure)

add_subdirectory(montecarlo)

add_subdirectory(intercomm)

add_subdirectory(request)

add_subdirectory(noncoll_oh)

add_subdirectory(restart_single_failure)

add_subdirectory(restart_double_failure_no_respawn)

add_subdirectory(restart_double_failure_both_respawn)

add_subdirectory(restart_single_failure_different_threads)

add_subdirectory(noncoll_test)

add_subdirectory(vs_shrink)

add_subdirectory(algo_test)

if(WITH_SESSION_TESTS)
    add_subdirectory(session_test)
endif()

add_subdirectory(tiny_mc)