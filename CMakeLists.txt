cmake_minimum_required(VERSION 3.7.2)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(YAPT_BUILD_EXAMPLES "Build YAPT examples?" ON)

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


##add modules
add_subdirectory(${YAPT_SRC_DIR}/Common)
add_subdirectory(${YAPT_SRC_DIR}/Gfx)
add_subdirectory(${YAPT_SRC_DIR}/Renderer)
add_subdirectory(${YAPT_SRC_DIR}/Scene)
add_subdirectory(${YAPT_SRC_DIR}/Gui)
add_subdirectory(${YAPT_SRC_DIR}/AssetLoader)


target_link_libraries(YAPT PUBLIC Renderer Gui Scene AssetLoader)

if(YAPT_BUILD_EXAMPLES)
	add_subdirectory(${YAPT_SRC_DIR}/Examples)
endif(YAPT_BUILD_EXAMPLES)

set_target_properties(YAPT PROPERTIES 
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/Bin)

