install(FILES
            pam_wrapper.1
        DESTINATION
            ${CMAKE_INSTALL_MANDIR}/man1)

install(FILES
            pam_matrix.8
        DESTINATION
            ${CMAKE_INSTALL_MANDIR}/man8)

install(FILES
            pam_chatty.8
        DESTINATION
            ${CMAKE_INSTALL_MANDIR}/man8)

install(FILES
            pam_get_items.8
        DESTINATION
            ${CMAKE_INSTALL_MANDIR}/man8)

install(FILES
            pam_set_items.8
        DESTINATION
            ${CMAKE_INSTALL_MANDIR}/man8)

#
# Build the documentation
#
include(UseDoxygen OPTIONAL)

