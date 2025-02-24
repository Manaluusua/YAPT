include("../../CMakeCommon.txt")

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

#ktx
install_with_vcpkg(Ktx)
find_package(Ktx CONFIG REQUIRED)
target_link_libraries(AssetLoader PRIVATE KTX::ktx)

#tinyGLTF
target_include_directories(AssetLoader PRIVATE "${YAPT_EXTERNALS_PATH}/tinygltf/")

generateVSFilters("${HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR})
generateVSFilters("${SOURCE}" ${CMAKE_CURRENT_SOURCE_DIR})



target_compile_definitions(AssetLoader 
		PRIVATE
			ASSETLOADER_MODULE
	)
