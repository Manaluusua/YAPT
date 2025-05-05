cmake_minimum_required(VERSION 3.30.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

##for now dump everything ot Bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Bin")

set(YAPT_SRC_DIR
    Source
)
set(YAPT_SOURCE
    ${YAPT_SRC_DIR}/main.cpp
)


set(YAPT_EXTERNALS_DIR
	External
)

get_filename_component(YAPT_EXTERNALS_PATH ${YAPT_EXTERNALS_DIR} ABSOLUTE)


project(YAPT)

add_executable(YAPT ${YAPT_SOURCE})

##compiler defines
if(MSVC)
    add_compile_options("/MP" "/WX" "/W3" "/GR-" "/fp:fast")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
else()
message(FATAL_ERROR "not implemented")
	
endif()

##Platform defines
if(WIN32)
	add_definitions(-DYAPT_WINDOWS)
endif(WIN32)

#make sure vcpkg is installed & bootstrapped
find_program(VCPKG_EXECUTABLE vcpkg)
if(NOT ${VCPKG_EXECUTABLE})
    include(FetchContent)
	FetchContent_Declare(
		vcpkg
		GIT_REPOSITORY https://github.com/microsoft/vcpkg.git
		GIT_TAG master
		GIT_PROGRESS TRUE
	)
	FetchContent_MakeAvailable(vcpkg)
	if(EXISTS ${vcpkg_SOURCE_DIR})
		execute_process(
			COMMAND cmd /c ${vcpkg_SOURCE_DIR}/bootstrap-vcpkg.bat
			WORKING_DIRECTORY ${vcpkg_SOURCE_DIR}
			RESULT_VARIABLE result
		)
		
		execute_process(
			COMMAND ${vcpkg_SOURCE_DIR}/./vcpkg integrate install
			WORKING_DIRECTORY ${vcpkg_SOURCE_DIR}
			RESULT_VARIABLE result
		)
		
		
	endif()
	
	#set(CMAKE_TOOLCHAIN_FILE "${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
	include(${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake)
else()
    message(STATUS "vcpkg found: ${VCPKG_EXECUTABLE}")
endif()


##add modules
add_subdirectory(${YAPT_SRC_DIR}/Common)
add_subdirectory(${YAPT_SRC_DIR}/Gfx)
add_subdirectory(${YAPT_SRC_DIR}/Renderer)
add_subdirectory(${YAPT_SRC_DIR}/Scene)
add_subdirectory(${YAPT_SRC_DIR}/AssetLoader)
add_subdirectory(${YAPT_SRC_DIR}/Python)

target_link_libraries(YAPT PUBLIC Common Python)

set_target_properties(YAPT PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)

