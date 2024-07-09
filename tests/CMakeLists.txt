project(tests C)

if (TARGET cmocka::cmocka)
    set(CMOCKA_LIBRARY cmocka::cmocka)
endif()

set(PAM_MATRIX_PATH "${CMAKE_BINARY_DIR}/src/modules/pam_matrix.so")

configure_file(services/matrix.in ${CMAKE_CURRENT_BINARY_DIR}/services/matrix @ONLY)

# Some tests use a passdb as argument for pam_matrix
set(PASSDB_RO_PATH ${CMAKE_CURRENT_BINARY_DIR}/passdb_ro)
configure_file(passdb_ro ${PASSDB_RO_PATH} @ONLY)
configure_file(services/matrix_opt.in ${CMAKE_CURRENT_BINARY_DIR}/services/matrix_opt @ONLY)

set(PASSDB_PY_PATH ${CMAKE_CURRENT_BINARY_DIR}/passdb_py)
configure_file(passdb_py ${PASSDB_PY_PATH} @ONLY)
configure_file(services/matrix_py.in ${CMAKE_CURRENT_BINARY_DIR}/services/matrix_py @ONLY)

configure_file(services/pwrap_get_set.in ${CMAKE_CURRENT_BINARY_DIR}/services/pwrap_get_set @ONLY)

set(PAM_CHATTY_PATH "${CMAKE_BINARY_DIR}/src/modules/pam_chatty.so")
configure_file(services/chatty.in ${CMAKE_CURRENT_BINARY_DIR}/services/chatty @ONLY)

function(ADD_CMOCKA_TEST_ENVIRONMENT _TEST_NAME)
    if (CMAKE_BUILD_TYPE)
        string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
        if (CMAKE_BUILD_TYPE_LOWER STREQUAL "addresssanitizer")
            find_library(ASAN_LIBRARY
                         NAMES asan)
            if (NOT ASAN_LIBRARY)
                foreach(version RANGE 10 1)
                    if (NOT ASAN_LIBRARY)
                        find_library(ASAN_LIBRARY libasan.so.${version})
                    endif()
                endforeach()
            endif()
        endif()
    endif()

    if (ASAN_LIBRARY)
        list(APPEND PRELOAD_LIBRARIES ${ASAN_LIBRARY})
    endif()
    list(APPEND PRELOAD_LIBRARIES ${PAM_WRAPPER_LOCATION})

    if (OSX)
        set(TORTURE_ENVIRONMENT "DYLD_FORCE_FLAT_NAMESPACE=1;DYLD_INSERT_LIBRARIES=${RESOLV_WRAPPER_LOCATION}:${SOCKET_WRAPPER_LIBRARY}")
    else ()
        string(REPLACE ";" ":" _TMP_ENV "${PRELOAD_LIBRARIES}")
        set(TORTURE_ENVIRONMENT "LD_PRELOAD=${_TMP_ENV}")
    endif()

    list(APPEND TORTURE_ENVIRONMENT PAM_WRAPPER=1)
    list(APPEND TORTURE_ENVIRONMENT PAM_WRAPPER_SERVICE_DIR=${CMAKE_CURRENT_BINARY_DIR}/services)

    foreach(_arg ${ARGN})
        list(APPEND TORTURE_ENVIRONMENT ${_arg})
    endforeach()

    set_property(TEST
                    ${_TEST_NAME}
                PROPERTY
                    ENVIRONMENT "${TORTURE_ENVIRONMENT}")
endfunction()

set(PAM_LIBRARIES pam)
if (HAVE_PAM_MISC)
	list(APPEND PAM_LIBRARIES pam_misc)
endif (HAVE_PAM_MISC)

set(TEST_LIBRARIES
    pamtest
    ${CMOCKA_LIBRARY}
    ${PAM_LIBRARIES}
)

add_cmocka_test(test_pam_wrapper
                SOURCES test_pam_wrapper.c
                COMPILE_OPTIONS ${DEFAULT_C_COMPILE_FLAGS}
                LINK_LIBRARIES ${TEST_LIBRARIES})
target_include_directories(test_pam_wrapper
                           PRIVATE
                               ${CMOCKA_INCLUDE_DIR}
                               ${pam_wrapper_BINARY_DIR})
add_cmocka_test_environment(test_pam_wrapper)

# Do not run python tests with AddressSanitizer
set(RUN_PYTHON_TESTS ON)
if (CMAKE_BUILD_TYPE)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
    if (CMAKE_BUILD_TYPE_LOWER STREQUAL "addresssanitizer")
        set(RUN_PYTHON_TESTS OFF)
    endif()
endif()

if (RUN_PYTHON_TESTS)
    if (PYTHON3_EXECUTABLE)
        add_test(NAME
                    py3pamtest_test
                 COMMAND
                    ${PYTHON3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/pypamtest_test.py)

        add_cmocka_test_environment(py3pamtest_test)
    endif()
endif()
