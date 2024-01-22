
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/configs.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/include/config.hpp"
)

### SOURCE FILES ###

set(LIBRARY_HDR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(LIBRARY_HEADERS
    "${CMAKE_CURRENT_BINARY_DIR}/include/config.hpp"
    "${LIBRARY_HDR_PATH}/cart_topology.hpp"
    "${LIBRARY_HDR_PATH}/comm_manipulation.hpp"
    "${LIBRARY_HDR_PATH}/complex_comm.hpp"
    "${LIBRARY_HDR_PATH}/context.hpp"
    "${LIBRARY_HDR_PATH}/intercomm_utils.hpp"
    "${LIBRARY_HDR_PATH}/legio.h"
    "${LIBRARY_HDR_PATH}/log.hpp"
    "${LIBRARY_HDR_PATH}/multicomm.hpp"
    "${LIBRARY_HDR_PATH}/request_handler.hpp"
    "${LIBRARY_HDR_PATH}/restart_manager.hpp"
    "${LIBRARY_HDR_PATH}/restart_routines.hpp"
    "${LIBRARY_HDR_PATH}/restart.h"
    "${LIBRARY_HDR_PATH}/session_manager.hpp"
    "${LIBRARY_HDR_PATH}/struct_selector.hpp"
    "${LIBRARY_HDR_PATH}/structure_handler.hpp"
    "${LIBRARY_HDR_PATH}/supported_comm.hpp"
    "${LIBRARY_HDR_PATH}/utils.hpp"
)
set(LIBRARY_INTERFACE
    "${LIBRARY_HDR_PATH}/legio.h"
    "${LIBRARY_HDR_PATH}/restart.h"
)

set(LIBRARY_SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(LIBRARY_SOURCES
    "${LIBRARY_SRC_PATH}/async.cpp"
    "${LIBRARY_SRC_PATH}/cart_topology.cpp"
    "${LIBRARY_SRC_PATH}/coll.cpp"
    "${LIBRARY_SRC_PATH}/comm_manipulation.cpp"
    "${LIBRARY_SRC_PATH}/complex_comm.cpp"
    "${LIBRARY_SRC_PATH}/fileio.cpp"
    "${LIBRARY_SRC_PATH}/general.cpp"
    "${LIBRARY_SRC_PATH}/intercomm_utils.cpp"
    "${LIBRARY_SRC_PATH}/legio.cpp"
    "${LIBRARY_SRC_PATH}/log.cpp"
    "${LIBRARY_SRC_PATH}/multicomm.cpp"
    "${LIBRARY_SRC_PATH}/osc.cpp"
    "${LIBRARY_SRC_PATH}/ptp.cpp"
    "${LIBRARY_SRC_PATH}/request_handler.cpp"
    "${LIBRARY_SRC_PATH}/restart_manager.cpp"
    "${LIBRARY_SRC_PATH}/restart_routines.cpp"
    "${LIBRARY_SRC_PATH}/restart.cpp"
    "${LIBRARY_SRC_PATH}/session.cpp"
    "${LIBRARY_SRC_PATH}/supported_comm.cpp"
    "${LIBRARY_SRC_PATH}/topo.cpp"
    "${LIBRARY_SRC_PATH}/utils.cpp"
)
if(${WITH_SESSION})
set(LIBRARY_SOURCES ${LIBRARY_SOURCES} "${LIBRARY_SRC_PATH}/session_manager.cpp")
endif()


add_library(legio ${LIBRARY_HEADERS} ${LIBRARY_SOURCES})
add_library(legio::legio ALIAS legio)
target_link_libraries(legio pthread)

target_include_directories(legio PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(legio PUBLIC cxx_std_17)
set_target_properties(legio PROPERTIES CXX_EXTENSIONS ON)
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