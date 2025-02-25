
include("../../CMakeCommon.txt")
include("./CMakePythonUtilityFunctions.txt")

function(copy_if_different _target _source _destination)

	add_custom_command (
		TARGET ${_target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E 
			copy_if_different 
				${_source}
				${_destination}
	)

endfunction()


#find python
set (Python3_FIND_ABI "OFF" "ANY" "ANY" "ANY")
find_package(Python3 COMPONENTS Interpreter Development)

#ensure pip and invoke pip to fetch python dependencies
ensure_pip()
install_with_pip(pybind11)
install_with_pip(pyside6)
install_with_pip(pygltflib)


#pybind11
find_pip_package_folder(pybind11)
if(EXISTS ${pybind11_DIR})
	message(STATUS "pybind11 found at ${pybind11_DIR}")
	set(pybind11_DIR "${pybind11_DIR}/share/cmake/pybind11")
	find_package(pybind11 CONFIG REQUIRED)
else()
	message(FATAL_ERROR "Couldn't find location of pybind11 (${pybind11_DIR})")
endif()


set(HEADERS
	inc/Python/Python.h
	inc/Python/CommonDefines.h
	inc/Python/Impl/CPython.h
)

set(SOURCE
	src/Python.cpp
)

file(GLOB_RECURSE HEADERS_BINDINGS incBindings/*.h)
file(GLOB_RECURSE SOURCE_BINDINGS srcBindings/*.cpp)


#Python module (interpreter)
project(Python)
add_library(Python SHARED)
target_sources(Python PRIVATE ${SOURCE} ${HEADERS})
target_include_directories(Python PUBLIC inc)
target_compile_definitions(Python PRIVATE PYTHON_MODULE)
target_link_libraries(Python PUBLIC Common pybind11::embed)
if(NOT YAPT_FORCE_DEBUG_PYTHON)
	set_target_properties(Python PROPERTIES MAP_IMPORTED_CONFIG_DEBUG Release)
	set_target_properties(Python PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

#Python module (bindings)
project(py_yapt)
pybind11_add_module(py_yapt MODULE ${HEADERS_BINDINGS} ${SOURCE_BINDINGS})
target_include_directories(py_yapt PRIVATE incBindings)
target_link_libraries(py_yapt PUBLIC Common Renderer Scene AssetLoader)
install(TARGETS Python py_yapt DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
set_target_properties(py_yapt PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>
)

if(YAPT_FORCE_DEBUG_PYTHON)
	target_compile_definitions (Python PRIVATE $<$<CONFIG:Debug>:Py_DEBUG>)
	target_compile_definitions (py_yapt PRIVATE $<$<CONFIG:Debug>:Py_DEBUG>)
	set(PYTHON_MODULE_DEBUG_POSTFIX "_d")
endif()

#enable RTTI for bindings
if (MSVC)
    target_compile_options(py_yapt PRIVATE /GR) 
else()
    target_compile_options(py_yapt PRIVATE -frtti)
endif()

#####################PYTHON SOURCES###################################
set(PYTHON_SCRIPTS_FOLDER "pythonScripts")
file(GLOB_RECURSE PYTHON_SOURCES ${PYTHON_SCRIPTS_FOLDER}/*.*)
add_custom_target( PythonScripts SOURCES ${PYTHON_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/${PYTHON_SCRIPTS_FOLDER}" FILES ${PYTHON_SOURCES})
add_custom_target(CopyPythonScriptsToBin ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${PYTHON_SCRIPTS_FOLDER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PYTHON_SCRIPTS_FOLDER}
	)
#####################PYTHON SOURCES###################################