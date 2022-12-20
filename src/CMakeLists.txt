project(libpam_wrapper C)

###########################################################
### pam_wrapper
###########################################################

add_library(pam_wrapper SHARED pam_wrapper.c)
target_include_directories(pam_wrapper
                           PRIVATE
                               ${pam_wrapper-headers_SOURCE_DIR}
                               ${pam_wrapper_BINARY_DIR})

set(PAM_WRAPPER_LIBRARIES
	${PAMWRAP_REQUIRED_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT})
if (HAVE_OPENPAM)
	list(APPEND PAM_WRAPPER_LIBRARIES pam)
endif (HAVE_OPENPAM)

target_link_libraries(pam_wrapper ${PAM_WRAPPER_LIBRARIES} ${DLFCN_LIBRARY})

target_compile_options(pam_wrapper
                       PRIVATE ${DEFAULT_C_COMPILE_FLAGS} -D_GNU_SOURCE=1)

set_target_properties(
  pam_wrapper
    PROPERTIES
      VERSION
        ${LIBRARY_VERSION}
      SOVERSION
        ${LIBRARY_SOVERSION}
)

install(TARGETS pam_wrapper
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

###########################################################
### libpamtest
###########################################################

set(pamtest_SOURCES
    libpamtest.c
)

set(PAM_LIBRARIES pam)
if (HAVE_PAM_MISC)
	list(APPEND PAM_LIBRARIES pam_misc)
endif (HAVE_PAM_MISC)

add_library(pamtest SHARED ${pamtest_SOURCES})
target_include_directories(pamtest
                           PRIVATE
                               ${pam_wrapper_BINARY_DIR}
                           PUBLIC
                               $<BUILD_INTERFACE:${pam_wrapper-headers_SOURCE_DIR}>
                               $<INSTALL_INTERFACE:include>)
add_library(pamtest::pamtest ALIAS pamtest)

target_link_libraries(pamtest ${PAM_LIBRARIES})
target_compile_options(pamtest
                       PRIVATE ${DEFAULT_C_COMPILE_FLAGS})

set_target_properties(pamtest
    PROPERTIES
        VERSION ${LIBRARY_VERSION}
        SOVERSION ${LIBRARY_SOVERSION})

install(TARGETS pamtest
        EXPORT pamtest-config
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(EXPORT pamtest-config
        NAMESPACE pamtest::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/pamtest)

add_subdirectory(modules)
add_subdirectory(python)

# This needs to be at the end
set(PAM_WRAPPER_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}pam_wrapper${CMAKE_SHARED_LIBRARY_SUFFIX}" PARENT_SCOPE)
