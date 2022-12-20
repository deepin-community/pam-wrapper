/* Name of package */
#cmakedefine PACKAGE "${PROJECT_NAME}"

/* Version number of package */
#cmakedefine VERSION "${PROJECT_VERSION}"

#cmakedefine BINARYDIR "${BINARYDIR}"
#cmakedefine SOURCEDIR "${SOURCEDIR}"

/************************** HEADER FILES *************************/

#cmakedefine HAVE_SYS_TYPES_H 1
#cmakedefine HAVE_UNISTD_H 1
#cmakedefine HAVE_SECURITY_PAM_APPL_H 1
#cmakedefine HAVE_SECURITY_PAM_MODULES_H 1
#cmakedefine HAVE_SECURITY_PAM_EXT_H 1
#cmakedefine HAVE_OPENPAM ${HAVE_OPENPAM}

/*************************** FUNCTIONS ***************************/

#cmakedefine HAVE_PAM_VSYSLOG 1
#cmakedefine HAVE_PAM_SYSLOG 1
#cmakedefine HAVE_PAM_START_CONFDIR 1
#cmakedefine HAVE_PAM_MODUTIL_SEARCH_KEY 1

#cmakedefine HAVE_PAM_VPROMPT_CONST 1
#cmakedefine HAVE_PAM_PROMPT_CONST 1
#cmakedefine HAVE_PAM_STRERROR_CONST 1
#cmakedefine HAVE_GETPROGNAME 1
#cmakedefine HAVE_GETEXECNAME 1

/*************************** LIBRARIES ***************************/
#cmakedefine PAM_LIBRARY "${PAM_LIBRARY}"

/**************************** OPTIONS ****************************/

#cmakedefine HAVE_APPLE 1

#cmakedefine HAVE_GCC_THREAD_LOCAL_STORAGE 1
#cmakedefine HAVE_GCC_ATOMIC_BUILTINS 1
#cmakedefine HAVE_CONSTRUCTOR_ATTRIBUTE 1
#cmakedefine HAVE_DESTRUCTOR_ATTRIBUTE 1
#cmakedefine HAVE_FUNCTION_ATTRIBUTE_FORMAT 1

/*************************** ENDIAN *****************************/

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#cmakedefine WORDS_BIGENDIAN 1
