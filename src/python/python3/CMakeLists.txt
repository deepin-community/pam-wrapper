project(python3-pamtest C)

find_package(Python 3.10 COMPONENTS Development Interpreter)
# Allow to specify sitearch dir on the commandline
if (NOT PYTHON_INSTALL_SITEARCH)
    set(PYTHON_INSTALL_SITEARCH ${Python_SITEARCH})
endif()

if (Python_Development_FOUND)
    add_library(python3-pamtest
                MODULE ${pypamtest_SOURCE_DIR}/pypamtest.c)
    target_compile_options(python3-pamtest
                           PRIVATE ${DEFAULT_C_COMPILE_FLAGS})
    target_include_directories(python3-pamtest
                               PRIVATE
                                   ${pam_wrapper_BINARY_DIR}
                                   ${pam_wrapper-headers_DIR})
    target_link_libraries(python3-pamtest PRIVATE pamtest::pamtest Python::Python)
    set_property(TARGET python3-pamtest PROPERTY PREFIX "")
    set_property(TARGET python3-pamtest PROPERTY OUTPUT_NAME "pypamtest")

    install(TARGETS
                python3-pamtest
            DESTINATION
                ${PYTHON_INSTALL_SITEARCH})
endif()
