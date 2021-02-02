cmake_minimum_required(VERSION 3.7.2)

#utility to copy runtime deps
function(copy_if_different _target _source _destination)

	add_custom_command (
		TARGET ${_target} POST_BUILD COMMAND ${CMAKE_COMMAND} -E 
			copy_if_different 
				${_source}
				${_destination}
	)

endfunction()



set(QT_PATH "" CACHE PATH "Path to Qt")

list(APPEND CMAKE_PREFIX_PATH ${QT_PATH})



project(Gui)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

set(HEADERS
	inc/Gui/CommonDefines.h
	inc/Gui/Gui.h
	inc/Gui/GuiController.h
	inc/Gui/MainWindow.h
	inc/Gui/CameraController.h
	inc/Gui/RenderVarEntry.h
	inc/Gui/RenderVarGroup.h
	inc/Gui/RenderVarsDialog.h
	inc/Gui/MutableNumberElement.h
	inc/Gui/RenderVarWrapper.h
)

set(SOURCE
	src/Gui.cpp
	src/GuiController.cpp
	src/MainWindow.cpp
	src/CameraController.cpp
	src/RenderVarEntry.cpp
	src/RenderVarGroup.cpp
	src/RenderVarsDialog.cpp
	src/RenderVarWrapper.cpp
)



add_library(Gui SHARED)

target_sources(Gui PRIVATE ${SOURCE} ${HEADERS})
target_include_directories(Gui PUBLIC inc)

find_package(Qt5Core CONFIG REQUIRED)
find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5Gui CONFIG REQUIRED)


copy_if_different(Gui $<TARGET_FILE:Qt5::Core> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
copy_if_different(Gui $<TARGET_FILE:Qt5::Widgets>  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
copy_if_different(Gui $<TARGET_FILE:Qt5::Gui>  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)

target_link_libraries(Gui PUBLIC Qt5::Core Qt5::Widgets Qt5::Gui)
target_link_libraries(Gui PRIVATE Common Renderer Scene)

target_compile_definitions(Gui PRIVATE GUI_MODULE)
