include("../../CMakeCommon.txt")
project(Common)

set(HEADERS
	inc/Common/Common.h
	inc/Common/CommonUtilities.h
	inc/Common/Logger.h
	inc/Common/RCObject.h
	inc/Common/RCObjectPtr.h
	inc/Common/TightlyPackedArray.h
	inc/Common/BubbleArray.h
	inc/Common/FileSystemPath.h
	inc/Common/SimpleTaskQueue.h
	inc/Common/MultiProducerPendingList.h
	inc/Common/GrowingMultiProducerPendingList.h
	inc/Common/ArrayIndexAllocator.h
	inc/Common/DeferredFreeArrayIndexAllocator.h
	inc/Common/RangeAllocator.h
	inc/Common/StructureOfArrays.h
	inc/Common/ThreadPool.h
	inc/Common/CommonWindowsUtility.h
	inc/Common/ArenaAllocator.h
	inc/Common/JobSystem.h
	inc/Common/JobSystem.inl

	inc/Math/Math.h
	inc/Math/MathUtility.h
	inc/Math/RandUtility.h
	inc/Math/RandUtility.inl
	inc/Math/GGX.h
	inc/Math/MiscBrdf.h
	inc/Math/Fresnel.h
	inc/Math/AABB.h
	inc/Math/SobolMatrices.h
)

set(SOURCE
	src/Common.cpp
	src/Logger.cpp
	src/RCObject.cpp
	src/FileSystemPath.cpp
	src/ArrayIndexAllocator.cpp
	src/DeferredFreeArrayIndexAllocator.cpp
	src/RangeAllocator.cpp
	src/ThreadPool.cpp
	src/ArenaAllocator.cpp

	src/MathUtility.cpp
	src/RandUtility.cpp

)

add_library(Common STATIC ${HEADERS} ${SOURCE})
target_include_directories(Common PUBLIC inc)
target_include_directories(Common PUBLIC "${YAPT_EXTERNALS_PATH}/glm")
target_include_directories(Common PUBLIC "${YAPT_EXTERNALS_PATH}/json/include/")

target_include_directories(Common PUBLIC "${YAPT_EXTERNALS_PATH}/xxhash")
target_sources(Common PRIVATE "${YAPT_EXTERNALS_PATH}/xxhash/xxhash.c")

generateVSFilters("${HEADERS}" ${CMAKE_CURRENT_SOURCE_DIR})
generateVSFilters("${SOURCE}" ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(Common 
		PUBLIC
			GLM_FORCE_ALIGNED_GENTYPES
	)

