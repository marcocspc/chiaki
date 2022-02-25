# Provides ILClient::ILClient

#/opt/vc/lib/libbcm_host.so
find_path(ILClient_LIB_DIR libbcm_host.so
	PATHS /opt/vc/include
	PATHS /opt/vc/lib
	NO_DEFAULT_PATH)
	
message(${ILClient_LIB_DIR})


#/opt/vc/lib/libvcilcs.a
set(_required_libs
	${ILClient_LIB_DIR}/libbcm_host.so
	${ILClient_LIB_DIR}/libvcilcs.a
	${ILClient_LIB_DIR}/libvchiq_arm.so
	${ILClient_LIB_DIR}/libvcos.so)

unset(_libvars)
foreach(_lib ${_required_libs})
	get_filename_component(_libname "${_lib}" NAME_WE)
	set(_libvar "ILClient_${_libname}_LIBRARY")
	list(APPEND _libvars "${_libvar}")
	if(EXISTS "${_lib}")
		set("${_libvar}" "${_lib}")
	else()
		set("${_libvar}" "${_libvar}-NOTFOUND")
	endif()
endforeach()

find_path(ILClient_INCLUDE_DIR bcm_host.h
	PATHS /opt/vc/include
	PATHS /usr/include
	NO_DEFAULT_PATH)

# /opt/vc/src/hello_pi/libs/ilclient/ilclient.c
find_path(ILClient_SOURCE_DIR ilclient.c
	PATHS /opt/vc/src/hello_pi/libs/ilclient)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ILClient
	FOUND_VAR ILClient_FOUND
	REQUIRED_VARS
		${_libvars}
		ILClient_INCLUDE_DIR
		ILClient_SOURCE_DIR)

if(ILClient_FOUND)
	if(NOT TARGET ILClient::ILClient)
		# see /opt/vc/src/hello_pi/libs/ilclient/Makefile
		add_library(ilclient STATIC
			"${ILClient_SOURCE_DIR}/ilclient.c"
			"${ILClient_SOURCE_DIR}/ilcore.c")
		target_include_directories(ilclient PUBLIC
			/home/pi/dev/missing_ilclient/opt_vc/include
			"${ILClient_INCLUDE_DIR}"
			"${ILClient_SOURCE_DIR}")
		target_compile_definitions(ilclient PUBLIC
			HAVE_LIBOPENMAX=2
			OMX
			OMX_SKIP64BIT
			USE_EXTERNAL_OMX
			HAVE_LIBBCM_HOST
			USE_EXTERNAL_LIBBCM_HOST
			USE_VCHIQ_ARM)
		target_link_libraries(ilclient PUBLIC ${_required_libs})
		add_library(ILClient::ILClient ALIAS ilclient)
	endif()
endif()
