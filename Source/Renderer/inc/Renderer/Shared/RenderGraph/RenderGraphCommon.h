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

	struct ClearValue
	{
		ClearValue()
		{}

		ClearValue(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
			:type(_ClearValueType::UINT),
			value(x, y, z, w)
		{}
		ClearValue(float x, float y, float z, float w)
			:type(_ClearValueType::FLOAT),
			value(x, y, z, w)
		{}

		ClearValue(float d, uint8_t s)
			:type(_ClearValueType::DEPTH_STENCIL),
			value(d, s)
		{}
		
		enum class _ClearValueType
		{
			FLOAT,
			UINT,
			DEPTH_STENCIL
		} type;

		union _ClearValue
		{
			_ClearValue()
			{}

			_ClearValue(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
				:uvec(x, y, z, w)
			{}
			_ClearValue(float x, float y, float z, float w)
				:fvec(x, y, z, w)
			{}

			_ClearValue(float d, uint8_t s)
			{
				depthStencil.depth = d;
				depthStencil.stencil = s;
			}

			glm::uvec4 uvec;
			glm::vec4 fvec;
			struct
			{
				float depth;
				uint8_t stencil;
			} depthStencil;
		} value;

		void set(float d, uint8_t s)
		{
			type = _ClearValueType::DEPTH_STENCIL;
			value.depthStencil.depth = d;
			value.depthStencil.stencil = s;
		}

		void set(unsigned int x, unsigned int y, unsigned int z, unsigned int w)
		{
			type = _ClearValueType::UINT;
			value.uvec = glm::uvec4(x, y, z, w);
		}

		void set(float x, float y, float z, float w)
		{
			type = _ClearValueType::FLOAT;
			value.fvec = glm::vec4(x, y, z, w);
		}


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