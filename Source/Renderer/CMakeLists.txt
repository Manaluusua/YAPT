cmake_minimum_required(VERSION 3.7.2)


include("../../CMakeCommon.txt")

SET(GRAPHICS_API "DirectX12" CACHE STRING "Graphics API to build")

SET_PROPERTY(CACHE GRAPHICS_API PROPERTY STRINGS DirectX12 Vulkan) 

if(${GRAPHICS_API} MATCHES "DirectX12")
	set(BUILD_DX12_BACKEND TRUE)
elseif(${GRAPHICS_API} MATCHES "Vulkan")
	set(BUILD_VK_BACKEND TRUE)
else()
message(FATAL_ERROR "No Graphics API selected")
endif()





list(APPEND CMAKE_MODULE_PATH 
	"${CMAKE_CURRENT_LIST_DIR}/cmake/"
)


project(Renderer)


set(HEADERS
	inc/Renderer/Renderer.h
	inc/Renderer/WindowSurfaceDefinition.h
	inc/Renderer/RendererCommonTypes.h
	inc/Renderer/RendererCommonTypesUtility.h
	inc/Renderer/CommonDefines.h
	inc/Renderer/Mesh.h
	inc/Renderer/Buffer.h
	inc/Renderer/Texture.h
	inc/Renderer/Material.h
	inc/Renderer/ResourceAllocationPool.h
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
	inc/Renderer/Shared/ResourceAllocationPoolImpl.h
	inc/Renderer/Shared/GfxTypes.h
	inc/Renderer/Shared/GfxApi.h
	inc/Renderer/Shared/CRenderer.h
	inc/Renderer/Shared/ShaderPipelineReflection.h
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
	inc/Renderer/Shared/Utility/ResourceViewPool.h
	inc/Renderer/Shared/Utility/DescriptorSetUtility.h
	inc/Renderer/Shared/Utility/PostProcessUtility.h
	inc/Renderer/Shared/Utility/GpuBufferHelper.h
	inc/Renderer/Shared/Utility/CommonGpuBufferStructs.h
	inc/Renderer/Shared/Utility/BindlessResourceUtility.h
	inc/Renderer/Shared/Utility/AccelerationStructureHelper.h
	inc/Renderer/Shared/Utility/ShaderTableHelper.h
	inc/Renderer/Shared/Utility/MultipleScatterLUT.h
	inc/Renderer/Shared/Utility/MultipleScatterLUTUtility.h
	inc/Renderer/Shared/Utility/DXCUtility.h

	inc/Renderer/Shared/RenderPipeline/RenderPipeline.h
	inc/Renderer/Shared/RenderPipeline/RenderPipelineBase.h
	inc/Renderer/Shared/RenderPipeline/RenderPipelineManager.h
	
	inc/Renderer/Shared/RenderPipeline/RenderStage.h
	inc/Renderer/Shared/RenderPipeline/RenderStageUtilities.h
	
	inc/Renderer/Shared/RenderPipeline/PathTracerPipeline.h	
	inc/Renderer/Shared/RenderPipeline/DebugDrawPipeline.h	
	
	inc/Renderer/Shared/RenderPipeline/TonemapStage.h
	inc/Renderer/Shared/RenderPipeline/RaytraceStage.h
	inc/Renderer/Shared/RenderPipeline/SwapChainStage.h
	inc/Renderer/Shared/RenderPipeline/MeshDebugStage.h
	
	inc/Renderer/Shared/RenderGraph/RenderGraph.h
	inc/Renderer/Shared/RenderGraph/RenderGraphNode.h
	inc/Renderer/Shared/RenderGraph/RenderGraphResourceRequirements.h
	inc/Renderer/Shared/RenderGraph/RenderGraphCommon.h
	inc/Renderer/Shared/RenderGraph/RenderNode.h
	inc/Renderer/Shared/RenderGraph/ComputeNode.h
	inc/Renderer/Shared/RenderGraph/CustomNode.h
	inc/Renderer/Shared/RenderGraph/RaytraceNode.h
	inc/Renderer/Shared/RenderGraph/SwapChainNode.h
	inc/Renderer/Shared/RenderGraph/GenericExecuteNode.h
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
	src/Shared/ResourceAllocationPoolImpl.cpp
	src/Shared/CRenderer.cpp
	src/Shared/ShaderPipelineReflection.cpp
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
	src/Shared/Utility/DXCUtility.cpp

	src/Shared/RenderPipeline/RenderPipelineBase.cpp
	src/Shared/RenderPipeline/RenderStage.cpp
	src/Shared/RenderPipeline/RenderStageUtilities.cpp
	src/Shared/RenderPipeline/PathTracerPipeline.cpp
	src/Shared/RenderPipeline/DebugDrawPipeline.cpp
	src/Shared/RenderPipeline/TonemapStage.cpp
	src/Shared/RenderPipeline/RaytraceStage.cpp
	src/Shared/RenderPipeline/SwapChainStage.cpp
	src/Shared/RenderPipeline/MeshDebugStage.cpp
	src/Shared/RenderPipeline/RenderPipelineManager.cpp
	
	src/Shared/RenderGraph/RenderGraph.cpp
	src/Shared/RenderGraph/RenderGraphNode.cpp
	src/Shared/RenderGraph/RenderGraphResourceRequirements.cpp
	src/Shared/RenderGraph/RenderNode.cpp
	src/Shared/RenderGraph/ComputeNode.cpp
	src/Shared/RenderGraph/CustomNode.cpp
	src/Shared/RenderGraph/RaytraceNode.cpp
	src/Shared/RenderGraph/SwapChainNode.cpp
	src/Shared/RenderGraph/GenericExecuteNode.cpp
)

add_library(Renderer SHARED)
if(${BUILD_DX12_BACKEND})
	list(APPEND HEADERS 
		inc/Renderer/Dx12/RendererDx12.h
		inc/Renderer/Dx12/d3dx12.h
		inc/Renderer/Dx12/RendererDx12.h
		inc/Renderer/Dx12/Dx12CommonIncludes.h
		inc/Renderer/Dx12/ResourceManagerDx12.h
		inc/Renderer/Dx12/DescriptorHeapDx12.h
		inc/Renderer/Dx12/DescriptorHeapAllocatorDx12.h
		inc/Renderer/Dx12/DescriptorSetPoolDx12.h
		inc/Renderer/Dx12/SwapChainDx12.h
		inc/Renderer/Dx12/ShaderUtilityDx12.h
		inc/Renderer/Dx12/ResourceAllocationPoolDx12.h
		inc/Renderer/Dx12/YaptToDx12Conversions.h
		inc/Renderer/Dx12/FenceHelperDx12.h
		inc/Renderer/Dx12/PlatformTypesDx12.h
		inc/Renderer/Dx12/AccelerationStructureBuilderDx12.h
		inc/Renderer/Dx12/CommandListPoolerDx12.h
		inc/Renderer/Dx12/RenderGraphDx12.h
		inc/Renderer/Dx12/RenderNodeDx12.h
		inc/Renderer/Dx12/ComputeNodeDx12.h
		inc/Renderer/Dx12/RaytraceNodeDx12.h
		inc/Renderer/Dx12/Dx12MiscUtils.h
		inc/Renderer/Dx12/DescriptorSetLayoutDx12.h
		inc/Renderer/Dx12/PipelineLayoutDx12.h
		inc/Renderer/Dx12/SubmissionThreadDx12.h
		inc/Renderer/Dx12/UploadHeapDx12.h
		inc/Renderer/Dx12/UploadHelperDx12.h
		inc/Renderer/Dx12/PipelineStateUtilityDx12.h
		inc/Renderer/Dx12/ComputePipelineStateDx12.h
		inc/Renderer/Dx12/GraphicsPipelineStateDx12.h
		inc/Renderer/Dx12/RaytracePipelineStateDx12.h
		inc/Renderer/Dx12/ShaderPipelineReflectionDx12.h
		inc/Renderer/Dx12/ShaderTableDx12.h
		)
	
	list(APPEND SOURCE
		
		src/Dx12/RendererDx12.cpp
		src/Dx12/ResourceManagerDx12.cpp
		src/Dx12/DescriptorHeapDx12.cpp
		src/Dx12/DescriptorHeapAllocatorDx12.cpp
		src/Dx12/DescriptorSetPoolDx12.cpp
		src/Dx12/SwapChainDx12.cpp
		src/Dx12/ShaderUtilityDx12.cpp
		src/Dx12/ResourceAllocationPoolDx12.cpp
		src/Dx12/FenceHelperDx12.cpp
		src/Dx12/AccelerationStructureBuilderDx12.cpp
		src/Dx12/GfxApiDx12.cpp
		src/Dx12/CommandListPoolerDx12.cpp
		src/Dx12/RenderGraphDx12.cpp
		src/Dx12/RenderNodeDx12.cpp
		src/Dx12/ComputeNodeDx12.cpp
		src/Dx12/RaytraceNodeDx12.cpp
		src/Dx12/Dx12MiscUtils.cpp
		src/Dx12/DescriptorSetLayoutDx12.cpp
		src/Dx12/PipelineLayoutDx12.cpp
		src/Dx12/SubmissionThreadDx12.cpp
		src/Dx12/UploadHeapDx12.cpp
		src/Dx12/UploadHelperDx12.cpp
		src/Dx12/PipelineStateUtilityDx12.cpp
		src/Dx12/ComputePipelineStateDx12.cpp
		src/Dx12/GraphicsPipelineStateDx12.cpp
		src/Dx12/RaytracePipelineStateDx12.cpp
		src/Dx12/ShaderPipelineReflectionDx12.cpp
		src/Dx12/ShaderTableDx12.cpp
		)
	
	set(WINDOWS10KIT_PATH "" CACHE PATH "Path to DirectX dependencies")
	set(WINDOWS10KIT_VERSION ${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION} CACHE PATH "Windowskit version")
	find_package(Dx12 REQUIRED)
	
	target_link_libraries(Renderer PRIVATE d3d12::d3d12 d3d12::dxgi d3d12::dxc)
	
	#copy dxil and dxcompiler
	copy_if_different(Renderer $<TARGET_FILE:d3d12::dxc> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
	copy_if_different(Renderer $<TARGET_PROPERTY:d3d12::dxc,DXIL_RUNTIME> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
	
	target_compile_definitions(Renderer 
		PRIVATE
			RENDERER_DX12
	)
	
endif()

if(${BUILD_VK_BACKEND})
	
	find_package(Vulkan REQUIRED)
	find_package(DxCompiler REQUIRED)
	target_link_libraries(Renderer PRIVATE Vulkan::Vulkan Dxc::Dxc)
	
	copy_if_different(Renderer $<TARGET_FILE:Dxc::Dxc> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
	
	#spirv-reflect
	target_include_directories(Renderer PRIVATE "${YAPT_EXTERNALS_PATH}/SPIRV-Reflect/")
	target_sources(Renderer PRIVATE "${YAPT_EXTERNALS_PATH}/SPIRV-Reflect/spirv_reflect.c")

	
	list(APPEND HEADERS 
		inc/Renderer/Vk/RendererVk.h
		inc/Renderer/Vk/PlatformTypesVk.h
		inc/Renderer/Vk/CommonVk.h
		inc/Renderer/Vk/SwapChainVk.h
		inc/Renderer/Vk/ResourceManagerVk.h
		inc/Renderer/Vk/ResourceAllocationPoolVk.h
		inc/Renderer/Vk/ResourceUtilityVk.h
		inc/Renderer/Vk/YaptToVkConversions.h
		inc/Renderer/Vk/ResourceHandlesVk.h
		inc/Renderer/Vk/UploadHeapVk.h
		inc/Renderer/Vk/UploadHelperVk.h
		inc/Renderer/Vk/SubmissionThreadVk.h
		inc/Renderer/Vk/SyncUtilities.h
		inc/Renderer/Vk/CommandBufferPoolVk.h
		inc/Renderer/Vk/ShaderModuleVk.h
		inc/Renderer/Vk/ShaderPipelineReflectionVk.h
		inc/Renderer/Vk/RenderGraphVk.h
		inc/Renderer/Vk/RenderNodeVk.h
		inc/Renderer/Vk/ComputeNodeVk.h
		inc/Renderer/Vk/RaytraceNodeVk.h
		inc/Renderer/Vk/ComputePipelineStateVk.h
		inc/Renderer/Vk/GraphicsPipelineStateVk.h
		inc/Renderer/Vk/RaytracePipelineStateVk.h
		inc/Renderer/Vk/DescriptorSetLayoutVk.h
		inc/Renderer/Vk/DescriptorSetPoolVk.h
		inc/Renderer/Vk/DescriptorSetVk.h
		)
	list(APPEND SOURCE
		src/Vk/RendererVk.cpp
		src/Vk/GfxApiVk.cpp
		src/Vk/SwapChainVk.cpp
		src/Vk/ResourceManagerVk.cpp
		src/Vk/ResourceAllocationPoolVk.cpp
		src/Vk/ResourceHandlesVk.cpp
		src/Vk/UploadHeapVk.cpp
		src/Vk/UploadHelperVk.cpp
		src/Vk/SubmissionThreadVk.cpp
		src/Vk/SyncUtilities.cpp
		src/Vk/CommandBufferPoolVk.cpp
		src/Vk/ShaderModuleVk.cpp
		src/Vk/ShaderPipelineReflectionVk.cpp
		src/Vk/RenderGraphVk.cpp
		src/Vk/RenderNodeVk.cpp
		src/Vk/ComputeNodeVk.cpp
		src/Vk/RaytraceNodeVk.cpp
		src/Vk/ComputePipelineStateVk.cpp
		src/Vk/GraphicsPipelineStateVk.cpp
		src/Vk/RaytracePipelineStateVk.cpp
		src/Vk/DescriptorSetLayoutVk.cpp
		src/Vk/DescriptorSetPoolVk.cpp
		src/Vk/DescriptorSetVk.cpp
		)
		
	target_compile_definitions(Renderer 
		PRIVATE
			RENDERER_VK
	)
	
	
	
endif()


#copy shader
copy_folder(Renderer ${CMAKE_CURRENT_SOURCE_DIR}/shaders ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders)

generateVSFilters("${HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR})
generateVSFilters("${SOURCE}" ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(Renderer PRIVATE ${SOURCE} ${HEADERS})

target_include_directories(Renderer PUBLIC inc)
target_link_libraries(Renderer PUBLIC Common)

target_compile_definitions(Renderer 
		PRIVATE
			RENDERER_MODULE
	)
	
#####################SHADER SOURCES###################################
set(SHADER_FOLDER "shaders")
#shader sources (TODO: check if there is a better way to have shaders show up in IDE. It seems that if they are just added as sources to renderer, FXC is actually invoked on them :/

file(GLOB_RECURSE SHADERS_SOURCES ${SHADER_FOLDER}/*.*)


add_custom_target( Shaders SOURCES ${SHADERS_SOURCES})
	
#####################SHADER SOURCES###################################
