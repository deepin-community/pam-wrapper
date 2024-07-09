project(pypamtest C)

add_subdirectory(python3)

if (WITH_WNO_CAST_FUNCTION_TYPE)
    set_source_files_properties(pypamtest.c
                                DIRECTORY python3
                                PROPERTIES COMPILE_OPTIONS "-Wno-cast-function-type")
endif()
