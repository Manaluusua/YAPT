
include("../../CMakeCommon.txt")

SET(GRAPHICS_API "DirectX12" CACHE STRING "Graphics API to build")

SET_PROPERTY(CACHE GRAPHICS_API PROPERTY STRINGS DirectX12 Vulkan) 
option(ENABLE_GPU_PERF_MARKERS "Enable GPU perf markers" ON)

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

project(Gfx)

set(HEADERS

	inc/Gfx/GfxTypes.h
	inc/Gfx/GfxApi.h
	inc/Gfx/GfxBasicTypes.h
	inc/Gfx/GfxBasicTypesUtility.h
	inc/Gfx/WindowSurfaceDefinition.h
	inc/Gfx/ShaderPipelineReflection.h
	
	inc/Gfx/Utility/ResourceViewPool.h
	inc/Gfx/Utility/ResourceStateTracker.h
	inc/Gfx/Utility/DXCUtility.h

	inc/Gfx/RenderGraph/RenderGraph.h
	inc/Gfx/RenderGraph/RenderGraphNode.h
	inc/Gfx/RenderGraph/RenderGraphResourceRequirements.h
	inc/Gfx/RenderGraph/RenderGraphCommon.h
	inc/Gfx/RenderGraph/RenderNode.h
	inc/Gfx/RenderGraph/ComputeNode.h
	inc/Gfx/RenderGraph/CustomNode.h
	inc/Gfx/RenderGraph/RaytraceNode.h
	inc/Gfx/RenderGraph/SwapChainNode.h
	inc/Gfx/RenderGraph/GenericExecuteNode.h
)

set(SOURCE

	src/Utility/DXCUtility.cpp
	
	src/ShaderPipelineReflection.cpp

	src/RenderGraph/RenderGraph.cpp
	src/RenderGraph/RenderGraphNode.cpp
	src/RenderGraph/RenderGraphResourceRequirements.cpp
	src/RenderGraph/RenderNode.cpp
	src/RenderGraph/ComputeNode.cpp
	src/RenderGraph/CustomNode.cpp
	src/RenderGraph/RaytraceNode.cpp
	src/RenderGraph/SwapChainNode.cpp
	src/RenderGraph/GenericExecuteNode.cpp

)

add_library(Gfx STATIC)
if(${BUILD_DX12_BACKEND})
	list(APPEND HEADERS 
		inc/Gfx/Dx12/RendererDx12.h
		inc/Gfx/Dx12/d3dx12.h
		inc/Gfx/Dx12/RendererDx12.h
		inc/Gfx/Dx12/Dx12CommonIncludes.h
		inc/Gfx/Dx12/ResourceManagerDx12.h
		inc/Gfx/Dx12/DescriptorHeapDx12.h
		inc/Gfx/Dx12/DescriptorHeapAllocatorDx12.h
		inc/Gfx/Dx12/DescriptorSetPoolDx12.h
		inc/Gfx/Dx12/SwapChainDx12.h
		inc/Gfx/Dx12/ShaderUtilityDx12.h
		inc/Gfx/Dx12/YaptToDx12Conversions.h
		inc/Gfx/Dx12/FenceHelperDx12.h
		inc/Gfx/Dx12/PlatformTypesDx12.h
		inc/Gfx/Dx12/AccelerationStructureBuilderDx12.h
		inc/Gfx/Dx12/CommandListPoolerDx12.h
		inc/Gfx/Dx12/RenderGraphDx12.h
		inc/Gfx/Dx12/RenderNodeDx12.h
		inc/Gfx/Dx12/ComputeNodeDx12.h
		inc/Gfx/Dx12/RaytraceNodeDx12.h
		inc/Gfx/Dx12/Dx12MiscUtils.h
		inc/Gfx/Dx12/DescriptorSetLayoutDx12.h
		inc/Gfx/Dx12/PipelineLayoutDx12.h
		inc/Gfx/Dx12/SubmissionThreadDx12.h
		inc/Gfx/Dx12/UploadHeapDx12.h
		inc/Gfx/Dx12/UploadHelperDx12.h
		inc/Gfx/Dx12/PipelineStateUtilityDx12.h
		inc/Gfx/Dx12/ComputePipelineStateDx12.h
		inc/Gfx/Dx12/GraphicsPipelineStateDx12.h
		inc/Gfx/Dx12/RaytracePipelineStateDx12.h
		inc/Gfx/Dx12/ShaderPipelineReflectionDx12.h
		inc/Gfx/Dx12/ShaderTableDx12.h
		)
	
	list(APPEND SOURCE
		
		src/Dx12/RendererDx12.cpp
		src/Dx12/ResourceManagerDx12.cpp
		src/Dx12/DescriptorHeapDx12.cpp
		src/Dx12/DescriptorHeapAllocatorDx12.cpp
		src/Dx12/DescriptorSetPoolDx12.cpp
		src/Dx12/SwapChainDx12.cpp
		src/Dx12/ShaderUtilityDx12.cpp
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
	
	target_link_libraries(Gfx PRIVATE d3d12::d3d12 d3d12::dxgi d3d12::dxc)
	
	#add pix perf markers support
	if(${ENABLE_GPU_PERF_MARKERS})
		set(PIX_VERSION 1.0.240308001)
		set(PIX_INSTALL_PATH "")
		install_with_nuget(WinPixEventRuntime ${PIX_VERSION} PIX_INSTALL_PATH)

		target_include_directories(Gfx PRIVATE "${PIX_INSTALL_PATH}/Include/")
		target_link_libraries(Gfx PRIVATE "${PIX_INSTALL_PATH}/bin/${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE}/WinPixEventRuntime.lib")
		add_custom_command(TARGET Gfx POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PIX_INSTALL_PATH}/bin/${CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE}/WinPixEventRuntime.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/WinPixEventRuntime.dll")
		
		target_compile_definitions(Gfx PRIVATE USE_PIX)
	endif()
	
	#dx12 memory allocator
	add_library(Dx12MemoryAllocator STATIC)
	target_sources(Dx12MemoryAllocator PUBLIC "${YAPT_EXTERNALS_PATH}/D3D12MemoryAllocator/src/D3D12MemAlloc.cpp" "${YAPT_EXTERNALS_PATH}/D3D12MemoryAllocator/include/D3D12MemAlloc.h")
	target_include_directories(Dx12MemoryAllocator PUBLIC "${YAPT_EXTERNALS_PATH}/D3D12MemoryAllocator/include/")
	target_link_libraries(Gfx PUBLIC Dx12MemoryAllocator)
	
	
	#copy dxil and dxcompiler
	add_custom_command(TARGET Gfx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        $<TARGET_FILE:d3d12::dxc> "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/dxcompiler.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        $<TARGET_PROPERTY:d3d12::dxc,DXIL_RUNTIME> "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/dxil.dll")
	
	target_compile_definitions(Gfx 
		PUBLIC
			RENDERER_DX12
	)
	
endif()

if(${BUILD_VK_BACKEND})
	
	find_package(Vulkan REQUIRED COMPONENTS dxc)
	target_link_libraries(Gfx PUBLIC Vulkan::Vulkan Vulkan::dxc_lib) #for now linked as public since we expose all the internal types where some need these includes. Change to private when the actual types are encapsulated
	
	get_filename_component(DXC_BIN_DIR "${Vulkan_dxc_EXECUTABLE}" DIRECTORY)

	
	add_custom_command(TARGET Gfx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different 
        ${DXC_BIN_DIR}/dxcompiler.dll "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/dxcompiler.dll")
	
	#spirv-reflect
	target_include_directories(Gfx PRIVATE "${YAPT_EXTERNALS_PATH}/SPIRV-Reflect/")
	target_sources(Gfx PRIVATE "${YAPT_EXTERNALS_PATH}/SPIRV-Reflect/spirv_reflect.c")

	

	
	list(APPEND HEADERS 
		inc/Gfx/Vk/RendererVk.h
		inc/Gfx/Vk/PlatformTypesVk.h
		inc/Gfx/Vk/CommonVk.h
		inc/Gfx/Vk/SwapChainVk.h
		inc/Gfx/Vk/ResourceManagerVk.h
		inc/Gfx/Vk/ResourceUtilityVk.h
		inc/Gfx/Vk/YaptToVkConversions.h
		inc/Gfx/Vk/ResourceHandlesVk.h
		inc/Gfx/Vk/UploadHeapVk.h
		inc/Gfx/Vk/UploadHelperVk.h
		inc/Gfx/Vk/SubmissionThreadVk.h
		inc/Gfx/Vk/SyncUtilities.h
		inc/Gfx/Vk/CommandBufferPoolVk.h
		inc/Gfx/Vk/ShaderModuleVk.h
		inc/Gfx/Vk/ShaderPipelineReflectionVk.h
		inc/Gfx/Vk/RenderGraphVk.h
		inc/Gfx/Vk/RenderNodeVk.h
		inc/Gfx/Vk/ComputeNodeVk.h
		inc/Gfx/Vk/RaytraceNodeVk.h
		inc/Gfx/Vk/ComputePipelineStateVk.h
		inc/Gfx/Vk/GraphicsPipelineStateVk.h
		inc/Gfx/Vk/RaytracePipelineStateVk.h
		inc/Gfx/Vk/DescriptorSetLayoutVk.h
		inc/Gfx/Vk/DescriptorSetPoolVk.h
		inc/Gfx/Vk/DescriptorSetVk.h
		inc/Gfx/Vk/QueueTransitionHelperVk.h
		inc/Gfx/Vk/ShaderTableVk.h
		inc/Gfx/Vk/AccelerationStructureBuilderVk.h
		inc/Gfx/Vk/AccelerationStructuresVk.h
		inc/Gfx/Vk/VkExtensions.h
		inc/Gfx/Vk/SimpleBufferAllocationUtility.h
		)
	list(APPEND SOURCE
		src/Vk/RendererVk.cpp
		src/Vk/GfxApiVk.cpp
		src/Vk/SwapChainVk.cpp
		src/Vk/ResourceManagerVk.cpp
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
		src/Vk/QueueTransitionHelperVk.cpp
		src/Vk/ShaderTableVk.cpp
		src/Vk/AccelerationStructureBuilderVk.cpp
		src/Vk/AccelerationStructuresVk.cpp
		src/Vk/SimpleBufferAllocationUtility.cpp
		)
		
	#VMA
	target_include_directories(Gfx PUBLIC "${YAPT_EXTERNALS_PATH}/VulkanMemoryAllocator/include/")
	target_sources(Gfx PRIVATE "${YAPT_EXTERNALS_PATH}/VulkanMemoryAllocator/include/vk_mem_alloc.h")
		
	target_compile_definitions(Gfx 
		PUBLIC
			RENDERER_VK
	)
	
	
	
endif()

generateVSFilters("${HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR})
generateVSFilters("${SOURCE}" ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(Gfx PRIVATE ${SOURCE} ${HEADERS})
target_link_libraries(Gfx PUBLIC Common)
target_include_directories(Gfx PUBLIC inc)

