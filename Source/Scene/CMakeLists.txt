cmake_minimum_required(VERSION 3.7.2)


include("../../CMakeCommon.txt")



project(Scene)


set(HEADERS
	inc/Scene/CommonDefines.h
	inc/Scene/Camera.h
	inc/Scene/Camera.inl
	inc/Scene/Scene.h
	inc/Scene/Transform.h
	inc/Scene/Transform.inl
	inc/Scene/Model.h
	
	inc/Scene/Impl/CModel.h
	inc/Scene/Impl/CScene.h
)

set(SOURCE
	src/CScene.cpp
	src/CModel.cpp
)

add_library(Scene SHARED)

generateVSFilters("${HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR})
generateVSFilters("${SOURCE}" ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(Scene PRIVATE ${SOURCE} ${HEADERS})

target_include_directories(Scene PUBLIC inc)
target_link_libraries(Scene PUBLIC Common Renderer)

target_compile_definitions(Scene 
		PRIVATE
			SCENE_MODULE
	)
