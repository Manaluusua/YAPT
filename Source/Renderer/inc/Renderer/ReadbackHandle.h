#pragma once

#include <Common/RCObject.h>
#include <Common/RCObjectPtr.h>
namespace YAPT
{
	enum class ReadbackTarget
	{
		FinalColor = 0,
		COUNT
	};

	enum class ReadbackState
	{
		Pending,
		Ready,
		Unused
	};
	class ReadbackObject : public RCObject
	{
	public:
		virtual ReadbackState getState() = 0;
	};

	using ReadbackHandle = RCObjectPtr<ReadbackObject>;

}