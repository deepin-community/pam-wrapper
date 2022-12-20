project(python3-pamtest C)

if (NOT PYTHON3_LIBRARY)
    unset(PYTHON_EXECUTABLE CACHE)
    unset(PYTHON_INCLUDE_DIR CACHE)
    unset(PYTHON_LIBRARY CACHE)
    unset(PYTHON_SITELIB CACHE)
    unset(PYTHONLIBS_FOUND CACHE)
    unset(PYTHONLIBS_VERSION_STRING CACHE)

    find_package(PythonLibs 3)
    find_package(PythonInterp 3)
    find_package(PythonSiteLibs 3)

    if (PYTHONLIBS_FOUND)
        set(PYTHON3_LIBRARY ${PYTHON_LIBRARY})
        set(PYTHON3_INCLUDE_DIR ${PYTHON_INCLUDE_DIR})
        set(PYTHON3_SITELIB ${PYTHON_SITELIB})
        set(PYTHON3_EXECUTABLE ${PYTHON_EXECUTABLE} CACHE FILEPATH "PYTHON3_EXECUTABLE")
    endif()

    unset(PYTHON_EXECUTABLE CACHE)
    unset(PYTHON_INCLUDE_DIR CACHE)
    unset(PYTHON_LIBRARY CACHE)
    unset(PYTHON_SITELIB CACHE)
    unset(PYTHONLIBS_FOUND CACHE)
    unset(PYTHONLIBS_VERSION_STRING CACHE)
endif()

if (PYTHON3_INCLUDE_DIR AND PYTHON3_LIBRARY AND PYTHON3_SITELIB)
    include_directories(${CMAKE_BINARY_DIR})
    include_directories(${pam_wrapper-headers_DIR})
    include_directories(${PYTHON3_INCLUDE_DIR})

    python_add_module(python3-pamtest ${pypamtest_SOURCE_DIR}/pypamtest.c)
    target_link_libraries(python3-pamtest pamtest::pamtest ${PYTHON3_LIBRARY})
    target_compile_options(python3-pamtest
                           PRIVATE ${DEFAULT_C_COMPILE_FLAGS})
    set_target_properties(python3-pamtest PROPERTIES OUTPUT_NAME "pypamtest")

    install(TARGETS
                python3-pamtest
            DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${PYTHON3_SITELIB})
endif()
