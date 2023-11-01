#ifndef YAPT_SHARED_GFXTYPES_H
#define YAPT_SHARED_GFXTYPES_H

#include <Renderer/RendererCommonTypes.h>

#include <Renderer/WindowSurfaceDefinition.h>

#ifdef RENDERER_DX12
#include <Renderer/Dx12/PlatformtypesDx12.h>
#elif RENDERER_VK
#include <Renderer/Vk/PlatformtypesVk.h>
#endif

#define YAPT_NULL_HANDLE nullptr
#define YAPT_NULL_INDEX size_t(-1)

#define MAX_NUMBER_OF_ALLOWED_BINDINGPOINT_ALIASES 10
#define HIGHEST_UNIQUE_DESCSET_INDEX 9999
#define BINDINGPOINT_ALIAS_MULTIPLE (HIGHEST_UNIQUE_DESCSET_INDEX + 1)


namespace YAPT
{



	struct GfxApiInitConfig
	{
		size_t pipelineLength;
		YaptRenderSurfaceHandle renderSurfaceHandle; 
	};

	struct AccelerationStructureGeometryDefinition
	{
		size_t vertexCount;
		size_t vertexStrideInBytes;
		size_t vertexBufferOffsetInBytes;
		ResourceFormat vertexFormat;
		size_t indexCount;
		ResourceFormat indexFormat;
		BufferHandle vertexBuffer;
		BufferHandle indexBuffer;

		size_t worldMatrixBufferOffsetInBytes;
		BufferHandle worldMatrixBuffer;
	};

	struct BottomLevelAccelerationStructureDefinition
	{
		AccelerationStructureGeometryDefinition* geometryDefinitions;
		size_t geometryDefinitionCount;
	};


	struct AccelerationStructureInstanceDefinition
	{
		float instanceToWorld[12];
		uint32_t instanceMask;
		uint32_t instanceID;
		size_t hitGroupShaderTableOffset;
		BottomLevelAccelerationStructureHandle blas;
	};

	struct TopLevelAccelerationStructureDefinition
	{
		AccelerationStructureInstanceDefinition* instanceDefinitions;
		size_t instanceDefinitionCount;
	};

	struct ResourceStateDescription
	{
		ResourceUsage resourceUsage;
		AccessFlags accessFlags;
		ShaderStages shaderStagesUsedIn;

		static ResourceStateDescription default() { return { RESOURCE_USAGE_UNKNOWN, ACCESS_FLAGS_READ, SHADERSTAGE_NONE }; }
	};

	struct ShaderModuleDefine
	{
		const char* name;
		const char* value;
	};

	struct ShaderStageCreateInfo 
	{
		ShaderStageBits stage;
		ShaderModuleHandle shaderModule;
		const char* entryPoint;
	};

	struct VertexInputAttribute
	{
		AttributeSemantic shaderInputSlot;
		ResourceFormat format;
		uint32_t perVertexOffset;
	};

	struct VertexBufferDefinition
	{
		VertexInputAttribute* attributes;
		uint32_t numberOfVertexAttributes;
		uint32_t stride;
		uint32_t vertexInputRate; // 0 for per vertex, >0 for per instance
	};

	struct BlendTargetDescription 
	{

		BlendFactor srcColor;
		BlendFactor dstColor;
		BlendOp colorBlendOp;
		BlendFactor srcAlpha;
		BlendFactor dstAlpha;
		BlendOp alphaBlendOp;
		ColorMask colorWriteMask;
		bool blendEnable;
	};

	struct BlendStateDescription
	{
		BlendTargetDescription* blendTargetDescriptions;
		uint32_t numberOfBlendTargets;
		LogicOp logicalOp;
		float blendConstants[4];
		bool enableLogicalOp;
		bool enableIndependentBlend;
	};

	struct RasterizerStateDescription
	{
		PolygonMode polygonMode;
		CullMode cullMode;
		FrontFace frontFace;
		bool depthBiasEnable;
		float depthBiasConstantFactor;
		float depthBiasClamp;
		float depthBiasSlopeFactor;
		float lineWidth;
		bool rasterizerDiscardEnable;
		bool depthClampEnable;
	};

	struct MultisampleStateDescription
	{
		uint64_t sampleMask;
		SampleCount sampleCount;
		float minSampleShading;
		bool enableSampleShading;
		bool enableAlphaToCoverage;
		bool enableAlphaToOne;
	};

	struct StencilState 
	{
		StencilOp    failOp;
		StencilOp    passOp;
		StencilOp    depthFailOp;
		CompareOp    compareOp;
		uint32_t     reference;
	};

	struct DepthStencilStateDescription
	{
		CompareOp depthCompareOp;
		StencilState stencilFront;
		StencilState stencilBack;
		uint32_t     compareMask;
		uint32_t     writeMask;
		float minDepthBounds;
		float maxDepthBounds;
		bool depthTestEnable;
		bool depthWriteEnable;
		bool depthBoundsTestEnable;
		bool stencilTestEnable;
	};

	struct SamplerDescription
	{
		Filter magFilter;
		Filter minFilter;
		Filter mipmapMode;
		SamplerAddressMode addressModeU;
		SamplerAddressMode addressModeV;
		SamplerAddressMode addressModeW;
		CompareOp compareOp;
		BorderColor borderColor;
		float mipLodBias;
		float maxAnisotropy;
		float minLod;
		float maxLod;
		bool enableAnisotropy;
		bool enableCompare;
	};

	struct ResourceTransitionBarrierDefinition
	{
		ResourceUsage usageBefore;
		AccessFlags accessBefore;
		ShaderStages shaderStagesBefore;

		ResourceUsage usageAfter;
		AccessFlags accessAfter;
		ShaderStages shaderStagesAfter;
	};

	struct DescriptorSetLayoutBinding
	{
		uint32_t bindingIndex;
		uint32_t descriptorCount;
		DescriptorType type;
		AccessFlags accessFlags;
		ShaderStages shaderStages;
		SamplerHandle* staticSamplers;
	};

	struct PushConstantRange
	{
		size_t offset;
		size_t count;
		ShaderStages shaderStages;
	};

	struct PushContantDefinition
	{
		size_t shaderSpace;
		size_t bindingSlot;
		size_t numberOfRanges;
		PushConstantRange* ranges;
	};

	struct DescriptorSetLayoutDescription
	{
		DescriptorSetLayoutBinding* bindings;
		size_t numberOfBindings;
	};

	struct DescriptorSetUpdate
	{
		uint32_t dstBinding;
		uint32_t dstArrayElement;
		uint32_t descriptorCount;
		TextureViewHandle* texHandles;
		BufferViewHandle* buffHandles;
		SamplerHandle* samplerHandles;
	};

	struct PipelineLayoutDescription
	{
		DescriptorSetLayoutHandle* descSetLayouts;
		size_t numberOfDescSetLayouts;
		PushContantDefinition* pushConstants;
	};

	struct ComputePipelineStateDesc
	{
		ShaderStageCreateInfo shaderStage;
		PipelineLayoutHandle pipelineLayout;
	};

	struct RayHitGroupDescription
	{
		const char* hitGroupName; 
		size_t closestHitShaderIndex;
		size_t anyHitShaderIndex; 
		size_t intersectionShaderIndex;

		size_t configIndex;

		HitGroupType hitGroupType;
	};

	struct RayGenerationDescription
	{
		size_t shaderIndex;
		size_t configIndex;
	};

	struct RayMissDescription
	{
		size_t shaderIndex;
		size_t configIndex;
	};

	struct RayTraceShaderConfig
	{
		size_t maxPayloadSizeInBytes;
		size_t maxAttributeSizeInBytes;
	};


	struct RaytracePipelineStateDesc
	{
		ShaderStageCreateInfo* shaders;
		size_t numberOfShaders;

		PipelineLayoutHandle layout;

		RayTraceShaderConfig* configs;
		size_t numberOfConfigs;

		RayHitGroupDescription* hitGroupDescriptions;
		size_t numberOfHitGroupDescription;
		size_t hitGroupShaderTableConstantsSizeInBytes;

		RayGenerationDescription* rayGenerationDescriptions;
		size_t numberOfRayGenerationDescription;
		size_t rayGenShaderTableConstantsSizeInBytes;

		RayMissDescription* rayMissDescriptions;
		size_t numberOfRayMissDescription;
		size_t missShaderTableConstantsSizeInBytes; 
		
		size_t maxTraceRecursionDepth;
	};

	struct GraphicsPipelineStateDesc
	{
		const ShaderStageCreateInfo* shaderStages;
		uint32_t numberOfShaderStages;
		const VertexBufferDefinition* vertexBufferLayouts;
		uint32_t numberOfVertexBufferLayouts;

		BlendStateDescription* blendStateDescription;
		DepthStencilStateDescription* depthStencilState;
		RasterizerStateDescription* rasterizerStateDescription;
		MultisampleStateDescription* multisampleState;
		
		ViewPort* viewports;
		ScissorRect* scissors;
		size_t numberOfViewportsAndScissors;

		PrimitiveTopology primitivetopology;
		uint32_t tesselationPatchControlPointCount;
		DynamicPipelineStates dynamicPipelineStates;

		PipelineLayoutHandle pipelineLayout;
		RenderPassHandle renderPass;
	};

	enum class GpuUploadStage
	{
		BEFORE_RENDER,
		DURING_RENDER
	};

	struct ShaderTableEntry
	{
		size_t shaderIndexInPso;
		size_t shaderTableIndex;
		uint8_t* shaderTableExtraData;
		size_t extraDataInBytes;
	};

	//forward declarations
	class RenderGraph;
	class ShaderPipelineReflection;
}


#endif