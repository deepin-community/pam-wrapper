project(python2-pamtest C)

if (NOT PYTHON2_LIBRARY)
    unset(PYTHON_EXECUTABLE CACHE)
    unset(PYTHON_INCLUDE_DIR CACHE)
    unset(PYTHON_LIBRARY CACHE)
    unset(PYTHON_SITELIB CACHE)
    unset(PYTHONLIBS_FOUND CACHE)
    unset(PYTHONLIBS_VERSION_STRING CACHE)

    find_package(PythonLibs 2)
    find_package(PythonInterp 2)
    find_package(PythonSiteLibs 2)

    if (PYTHONLIBS_FOUND)
        set(PYTHON2_EXECUTABLE ${PYTHON_EXECUTABLE} CACHE FILEPATH "PYTHON2_EXECUTABLE")
        set(PYTHON2_INCLUDE_DIR ${PYTHON_INCLUDE_DIR})
        set(PYTHON2_LIBRARY ${PYTHON_LIBRARY})
        set(PYTHON2_SITELIB ${PYTHON_SITELIB})
    endif()

    unset(PYTHON_EXECUTABLE CACHE)
    unset(PYTHON_INCLUDE_DIR CACHE)
    unset(PYTHON_LIBRARY CACHE)
    unset(PYTHON_SITELIB CACHE)
    unset(PYTHONLIBS_FOUND CACHE)
    unset(PYTHONLIBS_VERSION_STRING CACHE)
endif()

if (PYTHON2_INCLUDE_DIR AND PYTHON2_LIBRARY AND PYTHON2_SITELIB)
    include_directories(${CMAKE_BINARY_DIR})
    include_directories(${pam_wrapper-headers_DIR})
    include_directories(${PYTHON2_INCLUDE_DIR})

    python_add_module(python2-pamtest ${pypamtest_SOURCE_DIR}/pypamtest.c)
    target_link_libraries(python2-pamtest pamtest::pamtest ${PYTHON2_LIBRARY})
    target_compile_options(python2-pamtest
                           PRIVATE ${DEFAULT_C_COMPILE_FLAGS})
    set_target_properties(python2-pamtest PROPERTIES OUTPUT_NAME "pypamtest")

    install(TARGETS
                python2-pamtest
            DESTINATION
                ${CMAKE_INSTALL_PREFIX}/${PYTHON2_SITELIB})
endif()
