include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckTypeSize)
include(CheckStructHasMember)
include(CheckPrototypeDefinition)
include(TestBigEndian)

set(PACKAGE ${PROJECT_NAME})
set(VERSION ${PROJECT_VERSION})

set(BINARYDIR ${pam_wrapper_BINARY_DIR})
set(SOURCEDIR ${pam_wrapper_SOURCE_DIR})

function(COMPILER_DUMPVERSION _OUTPUT_VERSION)
    # Remove whitespaces from the argument.
    # This is needed for CC="ccache gcc" cmake ..
    string(REPLACE " " "" _C_COMPILER_ARG "${CMAKE_C_COMPILER_ARG1}")

    execute_process(
        COMMAND
            ${CMAKE_C_COMPILER} ${_C_COMPILER_ARG} -dumpversion
        OUTPUT_VARIABLE _COMPILER_VERSION
    )

    string(REGEX REPLACE "([0-9])\\.([0-9])(\\.[0-9])?" "\\1\\2"
           _COMPILER_VERSION "${_COMPILER_VERSION}")

    set(${_OUTPUT_VERSION} ${_COMPILER_VERSION} PARENT_SCOPE)
endfunction()

# HEADERS
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(security/pam_appl.h HAVE_SECURITY_PAM_APPL_H)
check_include_file(security/pam_modules.h HAVE_SECURITY_PAM_MODULES_H)
check_include_file(security/pam_ext.h HAVE_SECURITY_PAM_EXT_H)

# FUNCTIONS
check_function_exists(strncpy HAVE_STRNCPY)
check_function_exists(vsnprintf HAVE_VSNPRINTF)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(getprogname HAVE_GETPROGNAME)
check_function_exists(getexecname HAVE_GETEXECNAME)

check_prototype_definition(pam_vprompt
    "int pam_vprompt(const pam_handle_t *_pamh, int _style, char **_resp, const char *_fmt, va_list _ap)"
    "-1"
    "stdio.h;sys/types.h;security/pam_appl.h;security/pam_modules.h"
    HAVE_PAM_VPROMPT_CONST)

check_prototype_definition(pam_prompt
    "int pam_prompt(const pam_handle_t *_pamh, int _style, char **_resp, const char *_fmt, ...)"
    "-1"
    "stdio.h;sys/types.h;security/pam_appl.h;security/pam_modules.h"
    HAVE_PAM_PROMPT_CONST)

check_prototype_definition(pam_strerror
    "const char *pam_strerror(const pam_handle_t *_pamh, int _error_number)"
    "NULL"
    "stdio.h;sys/types.h;security/pam_appl.h;security/pam_modules.h"
    HAVE_PAM_STRERROR_CONST)

# LIBRARIES
find_library(PAM_LIBRARY NAMES libpam.so.0 pam)
set(PAM_LIBRARY ${PAM_LIBRARY})
find_library(PAM_MISC_LIBRARY NAMES pam_misc)
if (PAM_MISC_LIBRARY)
	set(HAVE_PAM_MISC TRUE)
endif()

check_library_exists(${PAM_LIBRARY} openpam_set_option "" HAVE_OPENPAM)

# PAM FUNCTIONS
set(CMAKE_REQUIRED_LIBRARIES ${PAM_LIBRARY})
check_function_exists(pam_syslog HAVE_PAM_SYSLOG)
check_function_exists(pam_vsyslog HAVE_PAM_VSYSLOG)
check_function_exists(pam_start_confdir HAVE_PAM_START_CONFDIR)
unset(CMAKE_REQUIRED_LIBRARIES)

# OPTIONS

if (LINUX)
    if (HAVE_SYS_SYSCALL_H)
       list(APPEND CMAKE_REQUIRED_DEFINITIONS "-DHAVE_SYS_SYSCALL_H")
    endif (HAVE_SYS_SYSCALL_H)
    if (HAVE_SYSCALL_H)
        list(APPEND CMAKE_REQUIRED_DEFINITIONS "-DHAVE_SYSCALL_H")
    endif (HAVE_SYSCALL_H)

    set(CMAKE_REQUIRED_DEFINITIONS)
endif (LINUX)

# COMPAT
if (HAVE_OPENPAM_H)
    set(HAVE_OPENPAM    1)
endif ()

check_c_source_compiles("
#include <stdbool.h>
int main(void) {
    bool x;
    bool *p_x = &x;
    __atomic_load(p_x, &x, __ATOMIC_RELAXED);
    return 0;
}" HAVE_GCC_ATOMIC_BUILTINS)

check_c_source_compiles("
__thread int tls;

int main(void) {
    return 0;
}" HAVE_GCC_THREAD_LOCAL_STORAGE)

check_c_source_compiles("
void test_constructor_attribute(void) __attribute__ ((constructor));

void test_constructor_attribute(void)
{
     return;
}

int main(void) {
     return 0;
}" HAVE_CONSTRUCTOR_ATTRIBUTE)

check_c_source_compiles("
void test_destructor_attribute(void) __attribute__ ((destructor));

void test_destructor_attribute(void)
{
    return;
}

int main(void) {
    return 0;
}" HAVE_DESTRUCTOR_ATTRIBUTE)

check_c_source_compiles("
#pragma init (test_constructor)
void test_constructor(void);

void test_constructor(void)
{
     return;
}

int main(void) {
     return 0;
}" HAVE_PRAGMA_INIT)

check_c_source_compiles("
#pragma fini (test_destructor)
void test_destructor(void);

void test_destructor(void)
{
    return;
}

int main(void) {
    return 0;
}" HAVE_PRAGMA_FINI)

check_c_source_compiles("
void log_fn(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

int main(void) {
    return 0;
}" HAVE_FUNCTION_ATTRIBUTE_FORMAT)

# SYSTEM LIBRARIES

check_library_exists(dl dlopen "" HAVE_LIBDL)
if (HAVE_LIBDL)
    find_library(DLFCN_LIBRARY dl)
endif (HAVE_LIBDL)

if (OSX)
    set(HAVE_APPLE 1)
endif (OSX)

# ENDIAN
if (NOT WIN32)
    test_big_endian(WORDS_BIGENDIAN)
endif (NOT WIN32)

set(UIDWRAP_REQUIRED_LIBRARIES ${DLFCN_LIBRARY} CACHE INTERNAL "uidwrap required system libraries")
