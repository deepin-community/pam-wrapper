# Required cmake version
cmake_minimum_required(VERSION 3.5.0)
cmake_policy(SET CMP0048 NEW)

# Specify search path for CMake modules to be loaded by include()
# and find_package()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# Add defaults for cmake
# Those need to be set before the project() call.
include(DefineCMakeDefaults)
include(DefineCompilerFlags)

project(pam_wrapper VERSION 1.1.3 LANGUAGES C)

# global needed variables
set(APPLICATION_NAME ${PROJECT_NAME})

# SOVERSION scheme: MAJOR.MINOR.PATCH
#   If there was an incompatible interface change:
#     Increment MAJOR. Set MINOR and PATCH to 0
#   If there was a compatible interface change:
#     Increment MINOR. Set PATCH to 0
#   If the source code was changed, but there were no interface changes:
#     Increment PATCH.
set(LIBRARY_VERSION_MAJOR 0)
set(LIBRARY_VERSION_MINOR 0)
set(LIBRARY_VERSION_PATCH 6)
set(LIBRARY_VERSION "${LIBRARY_VERSION_MAJOR}.${LIBRARY_VERSION_MINOR}.${LIBRARY_VERSION_PATCH}")
set(LIBRARY_SOVERSION ${LIBRARY_VERSION_MAJOR})

set(PAMTEST_LIBRARY_VERSION_MAJOR 0)
set(PAMTEST_LIBRARY_VERSION_MINOR 0)
set(PAMTEST_LIBRARY_VERSION_PATCH 5)
set(PAMTEST_LIBRARY_VERSION "${LIBRARY_VERSION_MAJOR}.${LIBRARY_VERSION_MINOR}.${LIBRARY_VERSION_PATCH}")
set(PAMTEST_LIBRARY_SOVERSION ${LIBRARY_VERSION_MAJOR})

# add definitions
include(DefinePlatformDefaults)
include(GNUInstallDirs)
include(DefineOptions.cmake)
include(CPackConfig.cmake)
include(CompilerChecks.cmake)

# Find out if we have threading available
set(CMAKE_THREAD_PREFER_PTHREADS ON)
find_package(Threads)

# config.h checks
include(ConfigureChecks.cmake)
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# check subdirectories
add_subdirectory(include)
add_subdirectory(src)

if (UNIT_TESTING)
    find_package(CMocka 1.1.0 REQUIRED)
    include(AddCMockaTest)
    add_subdirectory(tests)
endif (UNIT_TESTING)

# Link compile database for clangd
execute_process(COMMAND cmake -E create_symlink
                "${CMAKE_BINARY_DIR}/compile_commands.json"
                "${CMAKE_SOURCE_DIR}/compile_commands.json")
# pkg-config file
get_filename_component(PAM_WRAPPER_LIB ${PAM_WRAPPER_LOCATION} NAME)

configure_file(pam_wrapper.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/pam_wrapper.pc @ONLY)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/pam_wrapper.pc
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  COMPONENT
    pkgconfig
)

configure_file(libpamtest.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/libpamtest.pc @ONLY)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libpamtest.pc
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  COMPONENT
    pkgconfig
)

# cmake config files
include(CMakePackageConfigHelpers)

write_basic_package_version_file(pam_wrapper-config-version.cmake
                                 COMPATIBILITY
                                     AnyNewerVersion)
configure_file(pam_wrapper-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/pam_wrapper-config.cmake @ONLY)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/pam_wrapper-config-version.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/pam_wrapper-config.cmake
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/pam_wrapper
    COMPONENT
        devel
)

write_basic_package_version_file(pamtest-config-version.cmake
                                 COMPATIBILITY
                                     AnyNewerVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/pamtest-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pamtest
        COMPONENT devel)

add_subdirectory(doc)
