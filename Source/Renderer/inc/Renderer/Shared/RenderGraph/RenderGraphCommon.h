#ifndef YAPT_SHARED_RENDERGRAPHCOMMON_H
#define YAPT_SHARED_RENDERGRAPHCOMMON_H

#include <Renderer/Shared/GfxTypes.h>
#include <Math/Math.h>
#include <Common/CommonUtilities.h>
#include <Common/Logger.h>
namespace YAPT
{

	constexpr size_t INVALID_NODESLOT_INDEX = size_t(-1);
	constexpr size_t INVALID_RENDERGRAPH_RESOURCEID = size_t(-1);
	class RenderGraphNode;

	typedef size_t RenderGraphResourceId;

	enum class RenderNodeClearFrequency
	{
		NONE,
		ALWAYS
	};

	


	struct RenderGraphResourceDescription
	{
		ResourceDimension resourceDimensions;
		ResourceFormat resourceFormat;
		ResourceUsage resourceUsage;
		AccessFlags accessFlags;
		ShaderStages shaderStages;
		uint32_t mipCount;
		uint32_t arraySliceCount;
	};

	
	struct RenderGraphNodeEdge
	{
		RenderGraphNode* fromNode;
		size_t fromSlot;

		RenderGraphNode* toNode;
		size_t toSlot;

		uint32_t toArrayOffset;
		uint32_t toMipOffset;
	};

	inline bool tryToMergeRenderGraphResourceDescriptions(const RenderGraphResourceDescription& desc1, const RenderGraphResourceDescription& desc2, RenderGraphResourceDescription& descOut)
	{
		if (desc1.resourceDimensions != desc2.resourceDimensions)
		{
			return false;
		}

		if (desc1.resourceFormat != desc2.resourceFormat)
		{
			return false;
		}

		descOut.resourceDimensions = desc1.resourceDimensions;
		descOut.resourceFormat = desc1.resourceFormat;
		descOut.accessFlags = desc1.accessFlags | desc2.accessFlags;
		descOut.arraySliceCount = glm::max(desc1.arraySliceCount, desc2.arraySliceCount);
		descOut.mipCount = glm::max(desc1.mipCount, desc2.mipCount);
		descOut.resourceUsage = desc1.resourceUsage | desc2.resourceUsage;
		descOut.shaderStages = desc1.shaderStages | desc2.shaderStages;
		return true;
	}
}

#endif