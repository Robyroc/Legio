include("${CMAKE_CURRENT_SOURCE_DIR}/include/defaults.cmake")

function(defaultValue VAR VALUE)
    if(NOT DEFINED ${VAR})
        set(${VAR} ${VALUE} PARENT_SCOPE)
    endif()
endfunction(defaultValue)

defaultValue(BROADCAST_FAIL_POLICY ${FAULT_STOP})
defaultValue(SEND_FAIL_POLICY ${FAULT_IGNORE})
defaultValue(NUM_RETRY 3)
defaultValue(RECV_FAIL_POLICY ${FAULT_STOP})
defaultValue(REDUCE_FAIL_POLICY ${FAULT_IGNORE})
defaultValue(GET_FAIL_POLICY ${FAULT_STOP})
defaultValue(PUT_FAIL_POLICY ${FAULT_IGNORE})
defaultValue(GATHER_FAIL_POLICY ${FAULT_IGNORE})
defaultValue(GATHER_APPROACH ${RANK_KEEP})
defaultValue(SCATTER_FAIL_POLICY ${FAULT_STOP})
defaultValue(SCATTER_APPROACH ${RANK_KEEP})

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/configuration.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/configuration.h"
)

### SOURCE FILES ###

set(LIBRARY_HDR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(LIBRARY_HEADERS
    "${LIBRARY_HDR_PATH}/comm_manipulation.h"
    "${LIBRARY_HDR_PATH}/single_comm.h"
    "${LIBRARY_HDR_PATH}/configuration.h"
    "${LIBRARY_HDR_PATH}/legio.h"
    "${LIBRARY_HDR_PATH}/structure_handler.h"
    "${LIBRARY_HDR_PATH}/multicomm.h"
    "${LIBRARY_HDR_PATH}/adv_comm.h"
    "${LIBRARY_HDR_PATH}/operations.h"
    "${LIBRARY_HDR_PATH}/hierar_comm.h"
    "${LIBRARY_HDR_PATH}/no_comm.h"
)
set(LIBRARY_INTERFACE "${LIBRARY_HDR_PATH}/legio.h")

set(LIBRARY_SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LIBRARY_SOURCES
    "${LIBRARY_SRC_PATH}/coll.cpp" 
    "${LIBRARY_SRC_PATH}/comm_manipulation.cpp"
    "${LIBRARY_SRC_PATH}/single_comm.cpp"
    "${LIBRARY_SRC_PATH}/fileio.cpp"
    "${LIBRARY_SRC_PATH}/general.cpp"
    "${LIBRARY_SRC_PATH}/legio.c"
    "${LIBRARY_SRC_PATH}/multicomm.cpp"
    "${LIBRARY_SRC_PATH}/osc.cpp"
    "${LIBRARY_SRC_PATH}/ptp.cpp"
    "${LIBRARY_SRC_PATH}/hierar_comm.cpp"
)


add_library(legio ${LIBRARY_HEADERS} ${LIBRARY_SOURCES})
add_library(legio::legio ALIAS legio)

target_include_directories(legio PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(legio PUBLIC cxx_std_11)
set_target_properties(legio PROPERTIES CXX_EXTENSIONS OFF)
set_target_properties(legio PROPERTIES PUBLIC_HEADER "${LIBRARY_INTERFACE}")

linkMPI(legio)

linkMPI(legio)

if(MPI_COMPILE_FLAGS)
    set_target_properties(legio PROPERTIES COMPILE_FLAGS "${MPI_COMPILE_FLAGS}")
endif()

if(MPI_LINK_FLAGS)
    set_target_properties(legio PROPERTIES LINK_FLAGS "${MPI_LINK_FLAGS}")
endif()

include(GNUInstallDirs)

install(TARGETS legio
        EXPORT legioTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/legio
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/legio
)

include(CMakePackageConfigHelpers)
#configure_package_config_file(config/config.in
#    ${CMAKE_CURRENT_BINARY_DIR}/legioConfig.cmake
#    INSTALL_DESTINATION cmake
#)

set(CONFIG_INSTALL_PATH "${CMAKE_INSTALL_LIBDIR}/cmake")

write_basic_package_version_file(
    legioConfigVersion.cmake
    VERSION "1.0"
    COMPATIBILITY ExactVersion
)

install(
    EXPORT legioTargets
    FILE legioTargets.cmake
    NAMESPACE legio::
    DESTINATION ${CONFIG_INSTALL_PATH}
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config/config.in legioConfig.cmake @ONLY)

install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/legioConfig.cmake
          ${CMAKE_CURRENT_BINARY_DIR}/legioConfigVersion.cmake
    DESTINATION ${CONFIG_INSTALL_PATH}
)