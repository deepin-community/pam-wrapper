project(pam_wrapper-headers C)

set(libpamtest_HDRS
  libpamtest.h
)

install(FILES ${libpamtest_HDRS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT headers)
