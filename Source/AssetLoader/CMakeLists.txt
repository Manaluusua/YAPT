cmake_minimum_required(VERSION 3.7.2)



include("../../CMakeCommon.txt")

list(APPEND CMAKE_MODULE_PATH 
	"${CMAKE_CURRENT_LIST_DIR}/cmake/"
)


project(AssetLoader)


set(HEADERS
	inc/AssetLoader/CommonDefines.h
	inc/AssetLoader/AssetLoader.h

	
	inc/AssetLoader/Impl/CAssetLoader.h
	inc/AssetLoader/Impl/CTextureHandler.h
	inc/AssetLoader/Impl/CSceneLoader.h
	inc/AssetLoader/Impl/CDefaultRendererCacheProvider.h
)

set(SOURCE
	src/CAssetLoader.cpp
	src/CTextureHandler.cpp
	src/CSceneLoader.cpp
	src/CDefaultRendererCacheProvider.cpp
)

add_library(AssetLoader SHARED)
target_include_directories(AssetLoader PUBLIC inc)
target_link_libraries(AssetLoader PUBLIC Common Renderer)
target_sources(AssetLoader PRIVATE ${SOURCE} ${HEADERS})

#libktx
find_package(LibKtx REQUIRED)
target_link_libraries(AssetLoader PRIVATE LibKtx::libKtx )
copy_if_different(AssetLoader $<TARGET_PROPERTY:LibKtx::libKtx,libKtx_RUNTIME> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)

#tinyGLTF
target_include_directories(AssetLoader PRIVATE "${YAPT_EXTERNALS_PATH}/tinygltf/")

generateVSFilters("${HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR})
generateVSFilters("${SOURCE}" ${CMAKE_CURRENT_SOURCE_DIR})



target_compile_definitions(AssetLoader 
		PRIVATE
			ASSETLOADER_MODULE
	)
