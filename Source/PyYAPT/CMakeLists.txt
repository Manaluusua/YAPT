cmake_minimum_required(VERSION 3.7.2)
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
find_package(Python3 COMPONENTS Interpreter Development)

#ensure pip and invoke pip to fetch python dependencies
ensure_pip()
install_with_pip(pybind11)

#configure dependencies

#pybind11
find_pip_package_folder(pybind11)
if(EXISTS ${pybind11_DIR})
	message(STATUS "pybind11 found at ${pybind11_DIR}")
	set(pybind11_DIR "${pybind11_DIR}/share/cmake/pybind11")
	set(PYBIND11_FINDPYTHON ON)
	find_package(pybind11 CONFIG REQUIRED)
else()
	message(FATAL_ERROR "Couldn't find location of pybind11 (${pybind11_DIR})")
endif()

project(PyYAPT)


set(HEADERS
	inc/PyYAPT/PyYAPT.h
	inc/PyYAPT/CommonDefines.h
	inc/PyYAPT/Impl/CPyYAPT.h
	inc/PyYAPT/Impl/BindingsGeneral.h
)

set(SOURCE
	src/CPyYAPT.cpp
)

pybind11_add_module(PyYAPT SHARED ${SOURCE} ${HEADERS})
target_include_directories(PyYAPT PUBLIC inc)
target_link_libraries(PyYAPT PRIVATE Common Renderer Scene)
target_compile_definitions(PyYAPT PRIVATE PYTHON_MODULE)

target_link_libraries(PyYAPT PRIVATE Python3::Python Python3::Module)
target_link_libraries(PyYAPT PUBLIC Renderer Gui Scene AssetLoader)


copy_if_different(PyYAPT $<TARGET_FILE:Python3::Python> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)

#####################PYTHON SOURCES###################################
set(PYTHON_SCRIPTS_FOLDER "python")
file(GLOB_RECURSE PYTHON_SOURCES ${PYTHON_SCRIPTS_FOLDER}/*.*)
add_custom_target( PythonScripts SOURCES ${PYTHON_SOURCES})

add_custom_target(CopyPythonScriptsToBin ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/${PYTHON_SCRIPTS_FOLDER} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PYTHON_SCRIPTS_FOLDER}
	)
#####################PYTHON SOURCES###################################