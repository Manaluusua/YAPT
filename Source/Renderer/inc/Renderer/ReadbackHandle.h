#pragma once

#include <Common/RCObject.h>
#include <Common/RCObjectPtr.h>
#include <Gfx/GfxTypes.h>
namespace YAPT
{
	enum class ReadbackTarget
	{
		FinalColor = 0
	};

	enum class ReadbackState
	{
		Created,
		Pending,
		Ready,
		Failed,
		Freed
	};
	
	struct ReadbackData
	{
		const void* data;
		size_t widthOrSizeInBytes;
		size_t height;
		size_t depthOrSlices;
		//layout of the mapped texture data, 0 for buffers
		size_t rowPitchInBytes;
		size_t depthPitchInBytes;
		ResourceDimension dimensions;
		ResourceFormat format;
	};

	class ReadbackObject : public RCObject
	{
	public:
		virtual ReadbackState getState() = 0;
		virtual const ReadbackData* getData() = 0;
	};

	using ReadbackHandle = RCObjectPtr<ReadbackObject>;

}