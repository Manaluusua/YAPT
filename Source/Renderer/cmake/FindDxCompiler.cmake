if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (VC_PLATFORM_PATH_SUFFIX x64)
else ()
    set (VC_PLATFORM_PATH_SUFFIX x86)
endif()

set(DXC_PATH "" CACHE PATH "Path to DXC")

find_path (DXC_INCLUDE_PATH dxcapi.h PATHS ${DXC_PATH}/inc)
find_library (DXC_LIB dxcompiler PATHS ${DXC_PATH}/lib/${VC_PLATFORM_PATH_SUFFIX})
find_file (DXC_RUNTIME dxcompiler.dll PATHS ${DXC_PATH}/bin/${VC_PLATFORM_PATH_SUFFIX})




mark_as_advanced (
	DXC_INCLUDE_PATH 
	DXC_LIB
    DXC_RUNTIME
)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (
    DXC 
        REQUIRED_VARS 
			DXC_INCLUDE_PATH 
			DXC_LIB
            DXC_RUNTIME 
)

if (DXC_FOUND)
	add_library (Dxc::Dxc SHARED IMPORTED)
    set_target_properties (Dxc::Dxc
        PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            INTERFACE_INCLUDE_DIRECTORIES "${DXC_INCLUDE_PATH}"
            IMPORTED_IMPLIB "${DXC_LIB}"  
			IMPORTED_LOCATION "${DXC_RUNTIME}"
    )
	
	
	
endif()