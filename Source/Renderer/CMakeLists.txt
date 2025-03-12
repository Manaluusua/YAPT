
include("../../CMakeCommon.txt")

list(APPEND CMAKE_MODULE_PATH 
	"${CMAKE_CURRENT_LIST_DIR}/cmake/"
)


project(Renderer)


set(HEADERS
	inc/Renderer/Renderer.h
	inc/Renderer/CommonDefines.h
	inc/Renderer/Mesh.h
	inc/Renderer/Buffer.h
	inc/Renderer/Texture.h
	inc/Renderer/Material.h
	inc/Renderer/RenderObject.h
	inc/Renderer/MeshUtility.h
	inc/Renderer/RendererConfiguration.h
	inc/Renderer/RendererCacheProvider.h
	
	
	inc/Renderer/Shared/CRendererConfiguration.h
	inc/Renderer/Shared/RenderObjectManager.h
	inc/Renderer/Shared/RenderObjectProxy.h
	
	inc/Renderer/Shared/MeshManager.h
	inc/Renderer/Shared/MeshProxy.h
	inc/Renderer/Shared/MeshInternal.h
	
	inc/Renderer/Shared/MaterialManager.h
	inc/Renderer/Shared/MaterialProxy.h
	inc/Renderer/Shared/MaterialInternal.h
	
	inc/Renderer/Shared/BufferImpl.h
	inc/Renderer/Shared/TextureImpl.h
	inc/Renderer/Shared/CRenderer.h
	inc/Renderer/Shared/BindlessTextureManager.h
	inc/Renderer/Shared/BindlessBufferManager.h
	
	inc/Renderer/Shared/RendererVarsList.h
	inc/Renderer/Shared/RenderView.h

	inc/Renderer/Shared/Utility/RenderUtility.h
	inc/Renderer/Shared/Utility/PipelineStateDescriptionUtility.h
	inc/Renderer/Shared/Utility/CoreRenderResourcesUtility.h
	inc/Renderer/Shared/Utility/RenderResourcesPool.h
	inc/Renderer/Shared/Utility/RenderAPIAbstractionUtility.h
	inc/Renderer/Shared/Utility/ShaderLoader.h
	inc/Renderer/Shared/Utility/PipelineLayoutHelper.h
	
	inc/Renderer/Shared/Utility/DescriptorSetUtility.h
	inc/Renderer/Shared/Utility/PostProcessUtility.h
	inc/Renderer/Shared/Utility/GpuBufferHelper.h
	inc/Renderer/Shared/Utility/CommonGpuBufferStructs.h
	inc/Renderer/Shared/Utility/BindlessResourceUtility.h
	inc/Renderer/Shared/Utility/AccelerationStructureHelper.h
	inc/Renderer/Shared/Utility/ShaderTableHelper.h
	inc/Renderer/Shared/Utility/MultipleScatterLUT.h
	inc/Renderer/Shared/Utility/MultipleScatterLUTUtility.h

	inc/Renderer/Shared/RenderPipeline/RenderPipeline.h
	inc/Renderer/Shared/RenderPipeline/RenderPipelineBase.h
	inc/Renderer/Shared/RenderPipeline/RenderPipelineManager.h
	
	inc/Renderer/Shared/RenderPipeline/RenderStage.h
	
	inc/Renderer/Shared/RenderPipeline/PathTracerPipeline.h	
	inc/Renderer/Shared/RenderPipeline/DebugDrawPipeline.h	
	
	inc/Renderer/Shared/RenderPipeline/TonemapStage.h
	inc/Renderer/Shared/RenderPipeline/RaytraceStage.h
	inc/Renderer/Shared/RenderPipeline/SwapChainStage.h
	inc/Renderer/Shared/RenderPipeline/MeshDebugStage.h
)

set(SOURCE
	src/Material.cpp
	src/RenderObject.cpp
	src/MeshUtility.cpp
	
	src/Shared/CRendererConfiguration.cpp
	src/Shared/RenderObjectManager.cpp
	src/Shared/RenderObjectProxy.cpp
	
	src/Shared/MeshManager.cpp
	src/Shared/MeshProxy.cpp
	src/Shared/MeshInternal.cpp
	
	src/Shared/MaterialManager.cpp
	src/Shared/MaterialProxy.cpp
	src/Shared/MaterialInternal.cpp
	
	src/Shared/BufferImpl.cpp
	src/Shared/TextureImpl.cpp
	src/Shared/CRenderer.cpp
	src/Shared/BindlessTextureManager.cpp
	src/Shared/BindlessBufferManager.cpp
	src/Shared/RenderView.cpp

	src/Shared/Utility/RenderUtility.cpp
	src/Shared/Utility/PipelineStateDescriptionUtility.cpp
	src/Shared/Utility/CoreRenderResourcesUtility.cpp
	src/Shared/Utility/RenderResourcesPool.cpp
	src/Shared/Utility/ShaderLoader.cpp
	src/Shared/Utility/PipelineLayoutHelper.cpp
	src/Shared/Utility/DescriptorSetUtility.cpp
	src/Shared/Utility/PostProcessUtility.cpp
	src/Shared/Utility/BindlessResourceUtility.cpp
	src/Shared/Utility/AccelerationStructureHelper.cpp
	src/Shared/Utility/ShaderTableHelper.cpp
	src/Shared/Utility/MultipleScatterLUT.cpp
	
	src/Shared/RenderPipeline/RenderPipelineBase.cpp
	src/Shared/RenderPipeline/RenderStage.cpp
	src/Shared/RenderPipeline/PathTracerPipeline.cpp
	src/Shared/RenderPipeline/DebugDrawPipeline.cpp
	src/Shared/RenderPipeline/TonemapStage.cpp
	src/Shared/RenderPipeline/RaytraceStage.cpp
	src/Shared/RenderPipeline/SwapChainStage.cpp
	src/Shared/RenderPipeline/MeshDebugStage.cpp
	src/Shared/RenderPipeline/RenderPipelineManager.cpp
	

)

add_library(Renderer SHARED)

#copy shader
copy_folder(Renderer ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

generateVSFilters("${HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR})
generateVSFilters("${SOURCE}" ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(Renderer PRIVATE ${SOURCE} ${HEADERS})

target_include_directories(Renderer PUBLIC inc)
target_link_libraries(Renderer PUBLIC Gfx)

target_compile_definitions(Renderer 
		PRIVATE
			RENDERER_MODULE
	)
	
#####################SHADER SOURCES###################################
set(SHADER_FOLDER "shaders")
#shader sources (TODO: check if there is a better way to have shaders show up in IDE. It seems that if they are just added as sources to renderer, FXC is actually invoked on them :/
file(GLOB_RECURSE SHADERS_SOURCES ${SHADER_FOLDER}/*.*)
add_custom_target( Shaders SOURCES ${SHADERS_SOURCES})
add_custom_target(CopyShadersToBin ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
	)
#####################SHADER SOURCES###################################
