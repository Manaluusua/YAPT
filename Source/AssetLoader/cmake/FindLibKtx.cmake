if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set (VC_PLATFORM_PATH_SUFFIX x64)
else ()
    set (VC_PLATFORM_PATH_SUFFIX x86)
endif()

set(LibKtx_INCLUDEDIR "" CACHE PATH "Path to libKtx Headers")
set(LibKtx_LIBRARYDIR "" CACHE PATH "Path to libKtx binaries")

find_path(LibKtx_HEADERS ktx.h PATHS ${LibKtx_INCLUDEDIR})

find_library (LibKtx_Lib ktx PATHS ${LibKtx_LIBRARYDIR})
find_file (LibKtx_Dll ktx.dll PATHS ${LibKtx_LIBRARYDIR})

mark_as_advanced (
	LibKtx_HEADERS
	
    LibKtx_Lib 
    LibKtx_Dll

)

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args (
    LibKtx 
        REQUIRED_VARS 
			LibKtx_HEADERS
		
            LibKtx_Lib 
            LibKtx_Dll 
)

if (LibKtx_FOUND)

    add_library (LibKtx::libKtx SHARED IMPORTED)
    set_target_properties (LibKtx::libKtx
        PROPERTIES
            IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
            INTERFACE_INCLUDE_DIRECTORIES "${LibKtx_HEADERS}"
            IMPORTED_IMPLIB "${LibKtx_Lib}" 
			libKtx_RUNTIME "${LibKtx_Dll}"
    )
	
endif()