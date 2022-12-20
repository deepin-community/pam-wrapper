project(pam_wrapper-modules C)

set(PAM_MODULES pam_matrix pam_get_items pam_set_items pam_chatty)

set(PAM_LIBRARIES pam)
if (HAVE_PAM_MISC)
	list(APPEND PAM_LIBRARIES pam_misc)
endif (HAVE_PAM_MISC)

foreach(_PAM_MODULE ${PAM_MODULES})
	add_library(${_PAM_MODULE} MODULE ${_PAM_MODULE}.c)
    target_include_directories(${_PAM_MODULE}
                               PRIVATE
                                   ${pam_wrapper-headers_SOURCE_DIR}
                                   ${pam_wrapper_BINARY_DIR})
	set_property(TARGET ${_PAM_MODULE} PROPERTY PREFIX "")

	target_link_libraries(${_PAM_MODULE}
				${PAM_LIBRARIES})

	install(
		TARGETS
			${_PAM_MODULE}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/pam_wrapper
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/pam_wrapper)
endforeach()

