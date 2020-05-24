include(FindGit)
find_package(Git)

if(NOT Git_FOUND)
    message(FATAL_ERROR "GIT not found!")
endif()

include(ExternalProject)

ExternalProject_Add(
    legio

    PREFIX          legio
    GIT_REPOSITORY  https://github.com/Robyroc/Legio
    GIT_TAG         master
    GIT_SHALLOW     ON

    BUILD_ALWAYS    OFF
    INSTALL_DIR     ${CMAKE_CURRENT_BINARY_DIR}/ext/legio

    BUILD_COMMAND   ${CMAKE_COMMAND} --build <BINARY_DIR>
)