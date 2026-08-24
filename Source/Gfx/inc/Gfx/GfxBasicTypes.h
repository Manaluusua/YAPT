#ifndef YAPT_GFXBASICTYPES_H
#define YAPT_GFXBASICTYPES_H

#include  <cstdint>

#define YAPT_TEXTURE_VIEW_DESC_ALL_MIPS uint32_t(-1)
#define YAPT_TEXTURE_VIEW_DESC_ALL_SLICES uint32_t(-1)
#define YAPT_BUFFER_WHOLE_RESOURCE size_t(-1)

namespace YAPT
{
#define YAPTBIT(bit) 1 << bit
#define YAPT_AREBITSSET(val, bits) (((val) & (bits)) != 0)
	typedef uint32_t Flags;


	enum ResourceUsageBits
	{
		//common/idle
		RESOURCE_USAGE_UNKNOWN = 0,
		//copy usages
		RESOURCE_USAGE_COPY_DESTINATION = YAPTBIT(0),
		RESOURCE_USAGE_COPY_SOURCE = YAPTBIT(1),
		//buffer only usages
		RESOURCE_USAGE_UNIFORM_BUFFER = YAPTBIT(2),
		RESOURCE_USAGE_UNIFORM_TEXEL_BUFFER = YAPTBIT(3),
		RESOURCE_USAGE_STORAGE_BUFFER = YAPTBIT(4),
		RESOURCE_USAGE_STORAGE_TEXEL_BUFFER = YAPTBIT(5),
		RESOURCE_USAGE_VERTEX_BUFFER = YAPTBIT(6),
		RESOURCE_USAGE_INDEX_BUFFER = YAPTBIT(7),
		RESOURCE_USAGE_SHADERTABLE_BUFFER = YAPTBIT(8),
		RESOURCE_USAGE_ACCELERATION_STRUCTURE_BUFFER = YAPTBIT(9),
		RESOURCE_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT = YAPTBIT(10),

		//texture only usages
		RESOURCE_USAGE_SAMPLED_TEXTURE = YAPTBIT(11),
		RESOURCE_USAGE_STORAGE_TEXTURE = YAPTBIT(12),
		RESOURCE_USAGE_RENDER_TARGET_TEXTURE = YAPTBIT(13),
		RESOURCE_USAGE_DEPTH_TEXTURE = YAPTBIT(14),
		RESOURCE_USAGE_STENCIL_TEXTURE = YAPTBIT(15),
		RESOURCE_USAGE_PRESENTABLE_TEXTURE = YAPTBIT(16),

		//composites
		RESOURCE_USAGE_DEPTH_STENCIL_TEXTURE = RESOURCE_USAGE_DEPTH_TEXTURE | RESOURCE_USAGE_STENCIL_TEXTURE
	};
	typedef Flags ResourceUsage;

	enum class QueueType
	{
		QUEUE_TYPE_GRAPHICS,
		QUEUE_TYPE_COMPUTE
	};

	enum class BindingPoint
	{
		BINDING_POINT_GRAPHICS,
		BINDING_POINT_COMPUTE,
		BINDING_POINT_RAYTRACE
	};

	enum class MemoryType
	{
		//common/idle
		DEFAULT = 0,
		CPU_MAPPABLE_UPLOAD = YAPTBIT(1),
		CPU_MAPPABLE_READBACK = YAPTBIT(0),
	};


	enum class ShaderModuleType
	{
		VERTEX_MODULE,
		HULL_MODULE,
		DOMAIN_MODULE,
		GEOMETRY_MODULE,
		FRAGMENT_MODULE,
		COMPUTE_MODULE,
		LIBRARY_MODULE,
		LAST
	};

	enum class ResourceDimension
	{
		BUFFER,
		TEXTURE_1D,
		TEXTURE_1D_ARRAY,
		TEXTURE_2D,
		TEXTURE_2D_ARRAY,
		TEXTURE_3D,
		TEXTURE_CUBEMAP,
		TEXTURE_CUBEMAP_ARRAY,
		UNDEFINED
	};

	enum AccessFlagsBits
	{
		ACCESS_FLAGS_WRITE = YAPTBIT(0),
		ACCESS_FLAGS_READ = YAPTBIT(1),
		ACCESS_FLAGS_READ_WRITE = ACCESS_FLAGS_WRITE | ACCESS_FLAGS_READ
	};
	typedef Flags AccessFlags;

	enum DescriptorSetLayoutFlagBits
	{
		DESCRIPTORSETLAYOUTFLAG_NONE,
		DESCRIPTORSETLAYOUTFLAG_BINDINGS_MAY_ALIAS
	};

	typedef Flags DescriptorSetLayoutFlags;


	enum BufferViewFlagBits
	{
		BUFFERVIEWFLAGS_NONE = 0,
		BUFFERVIEWFLAGS_RAW = YAPTBIT(0)
	};

	typedef Flags BufferViewFlags;

	enum ShaderStageBits
	{
		SHADERSTAGE_NONE = 0,
		SHADERSTAGE_VERTEX = YAPTBIT(0),
		SHADERSTAGE_FRAGMENT = YAPTBIT(1),
		SHADERSTAGE_HULL = YAPTBIT(2),
		SHADERSTAGE_DOMAIN = YAPTBIT(3),
		SHADERSTAGE_GEOMETRY = YAPTBIT(4),
		SHADERSTAGE_COMPUTE = YAPTBIT(5),

		SHADERSTAGE_RT_RAYGENERATION = YAPTBIT(6),
		SHADERSTAGE_RT_MISS = YAPTBIT(7),
		SHADERSTAGE_RT_ANY_HIT = YAPTBIT(8),
		SHADERSTAGE_RT_CLOSEST_HIT = YAPTBIT(9),
		SHADERSTAGE_RT_INTERSECTION = YAPTBIT(10)
	};
	typedef Flags ShaderStages;
	constexpr ShaderStages ALL_SHADER_STAGES = SHADERSTAGE_VERTEX | SHADERSTAGE_FRAGMENT | SHADERSTAGE_HULL | SHADERSTAGE_DOMAIN | SHADERSTAGE_GEOMETRY | SHADERSTAGE_COMPUTE |
		SHADERSTAGE_RT_RAYGENERATION | SHADERSTAGE_RT_MISS | SHADERSTAGE_RT_ANY_HIT | SHADERSTAGE_RT_CLOSEST_HIT | SHADERSTAGE_RT_INTERSECTION;

	enum ColorMaskBits {
		RED = YAPTBIT(0),
		GREEN = YAPTBIT(1),
		BLUE = YAPTBIT(2),
		ALPHA = YAPTBIT(3),
		ALL = RED | GREEN | BLUE | ALPHA
	};
	typedef uint8_t ColorMask;

	enum class DescriptorType
	{
		SAMPLER = 0,
		TEXTURE = 1,
		STORAGE_TEXTURE = 2,
		UNIFORM_BUFFER = 3,
		UNIFORM_TEXEL_BUFFER = 4,
		UNIFORM_BUFFER_DYNAMIC = 5,
		STORAGE_BUFFER = 6,
		STORAGE_TEXEL_BUFFER = 7,
		STORAGE_BUFFER_DYNAMIC = 8,
		ACCELERATION_STRUCTURE = 9

	};



	enum VertexInputRate
	{
		PER_VERTEX = 0,
		PER_INSTANCE = 1
	};

	enum class PrimitiveTopology
	{
		POINTLIST,
		LINELIST,
		LINESTRIP,
		TRIANGLELIST,
		TRIANGLESTRIP,
		LINELIST_WITH_ADJACENCY,
		LINESTRIP_WITH_ADJACENCY,
		TRIANGLELIST_WITH_ADJACENCY,
		TRIANGLESTRIP_WITH_ADJACENCY,
		PATCHLIST,
	};

	enum class LogicOp 
	{
		CLEAR,
		AND,
		AND_REVERSE,
		COPY,
		AND_INVERTED,
		NO_OP,
		XOR,
		OR,
		NOR,
		EQUIVALENT,
		INVERT,
		OR_REVERSE,
		COPY_INVERTED,
		OR_INVERTED,
		NAND,
		SET
	};

	enum class CompareOp 
	{
		NEVER,
		LESS,
		EQUAL,
		LESS_OR_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_OR_EQUAL,
		ALWAYS,
	};

	enum class StencilOp 
	{
		KEEP,
		ZERO,
		REPLACE,
		INCREMENT_AND_CLAMP,
		DECREMENT_AND_CLAMP,
		INVERT,
		INCREMENT_AND_WRAP,
		DECREMENT_AND_WRAP
	};

	enum class BlendFactor 
	{
		ZERO,
		ONE,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA,
		CONSTANT_COLOR,
		ONE_MINUS_CONSTANT_COLOR,
		CONSTANT_ALPHA,
		ONE_MINUS_CONSTANT_ALPHA,
		SRC_ALPHA_SATURATE,
		SRC1_COLOR,
		ONE_MINUS_SRC1_COLOR,
		SRC1_ALPHA,
		ONE_MINUS_SRC1_ALPHA
	};

	enum class BlendOp 
	{
		ADD,
		SUBTRACT,
		REVERSE_SUBTRACT,
		MIN,
		MAX
		
	};

	enum class PolygonMode
	{
		FILL,
		LINE,
		POINT,
	};

	enum class FrontFace
	{
		COUNTER_CLOCKWISE,
		CLOCKWISE
	};

	enum class CullMode
	{
		NONE,
		FRONT,
		BACK
	};

	enum class Filter
	{
		NEAREST,
		LINEAR,
	};


	enum class SamplerAddressMode 
	{
		REPEAT,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER,
		MIRROR_CLAMP_TO_EDGE,

	};

	enum class BorderColor
	{
		FLOAT_TRANSPARENT_BLACK,
		INT_TRANSPARENT_BLACK,
		FLOAT_OPAQUE_BLACK,
		INT_OPAQUE_BLACK,
		FLOAT_OPAQUE_WHITE,
		INT_OPAQUE_WHITE,
	};


	enum class SampleCount
	{
		SAMPLE_COUNT_1,
		SAMPLE_COUNT_2,
		SAMPLE_COUNT_4,
		SAMPLE_COUNT_8,
		SAMPLE_COUNT_16,
		SAMPLE_COUNT_32,
		SAMPLE_COUNT_64,
	};

	enum class IndexType
	{
		UINT16,
		UINT32
	};

	enum DynamicPipelineStateBits 
	{
		DYNAMIC_PIPELINESTATE_NONE = 0,
		DYNAMIC_PIPELINESTATE_VIEWPORT = YAPTBIT(0),
		DYNAMIC_PIPELINESTATE_SCISSOR = YAPTBIT(1),
		DYNAMIC_PIPELINESTATE_LINE_WIDTH = YAPTBIT(2),
		DYNAMIC_PIPELINESTATE_DEPTH_BIAS = YAPTBIT(3),
		DYNAMIC_PIPELINESTATE_BLEND_CONSTANTS = YAPTBIT(4),
		DYNAMIC_PIPELINESTATE_DEPTH_BOUNDS = YAPTBIT(5),
		DYNAMIC_PIPELINESTATE_STENCIL_COMPARE_MASK = YAPTBIT(6),
		DYNAMIC_PIPELINESTATE_STENCIL_WRITE_MASK = YAPTBIT(7),
		DYNAMIC_PIPELINESTATE_STENCIL_REFERENCE = YAPTBIT(8),
	};
	typedef Flags DynamicPipelineStates;

	enum class HitGroupType
	{
		TRIANGLE,
		BOUNDINGBOX
	};

	enum class ResourceFormat
	{
		UNKNOWN,

		R8_UNORM,
		R8_SNORM,
		R8_UINT,
		R8_SINT,
		R8_SRGB,

		R16_UNORM,
		R16_SNORM,
		R16_UINT,
		R16_SINT,
		R16_SFLOAT,

		R32_UINT,
		R32_SINT,
		R32_SFLOAT,

		RG8_UNORM,
		RG8_SNORM,
		RG8_UINT,
		RG8_SINT,
		RG8_SRGB,

		RG16_UNORM,
		RG16_SNORM,
		RG16_UINT,
		RG16_SINT,
		RG16_SFLOAT,

		RG32_UINT,
		RG32_SINT,
		RG32_SFLOAT,

		RGB8_UNORM,
		RGB8_SNORM,
		RGB8_UINT,
		RGB8_SINT,
		RGB8_SRGB,

		BGR8_UNORM,
		BGR8_SNORM,
		BGR8_UINT,
		BGR8_SINT,
		BGR8_SRGB,

		RGB32_UINT,
		RGB32_SINT,
		RGB32_SFLOAT,

		RGBA8_UNORM,
		RGBA8_SNORM,
		RGBA8_UINT,
		RGBA8_SINT,
		RGBA8_SRGB,

		BGRA8_UNORM,
		BGRA8_SNORM,
		BGRA8_UINT,
		BGRA8_SINT,
		BGRA8_SRGB,

		RGBA16_UNORM,
		RGBA16_SNORM,
		RGBA16_UINT,
		RGBA16_SINT,
		RGBA16_SFLOAT,

		RGBA32_UINT,
		RGBA32_SINT,
		RGBA32_SFLOAT,

		D16_UNORM,
		D32_SFLOAT,
		D24_UNORM_S8_UINT,


		BC4_UNORM,
		BC4_SNORM,
		BC6H_SFLOAT,
		BC6H_UFLOAT,
		BC7_UNORM,
		BC7_UNORM_SRGB
	};

	enum class AttributeSemanticName : uint16_t
	{
		UNKNOWN = 0,
		POSITION,
		TEXCOORD,
		NORMAL,
		TANGENT,
		COLOR
	};

	struct AttributeSemantic
	{
		AttributeSemantic()
		{
			set(AttributeSemanticName::UNKNOWN, 0);
		}
		AttributeSemantic(AttributeSemanticName type, uint16_t index)
		{
			set(type, index);
		}

		AttributeSemantic(AttributeSemanticName type)
		{
			set(type, 0);
		}

		void set(AttributeSemanticName type, uint16_t index)
		{
			semantic = index << 16 | static_cast<uint16_t>(type);
		}


		AttributeSemanticName getType() const
		{
			return static_cast<AttributeSemanticName>(semantic & 0xFFFF);
		}

		uint16_t getIndex() const
		{
			return semantic >> 16;
		}

		bool operator==(const AttributeSemantic& o)
		{
			return semantic == o.semantic;
		}

	private:
		uint32_t semantic;
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
				:uvec{ x, y, z, w }
			{}
			_ClearValue(float x, float y, float z, float w)
				:fvec{x, y, z, w}
			{}

			_ClearValue(float d, uint8_t s)
			{
				depthStencil.depth = d;
				depthStencil.stencil = s;
			}

			uint32_t uvec[4];
			float fvec[4];
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
			value.uvec[0] = x;
			value.uvec[1] = y;
			value.uvec[2] = z;
			value.uvec[3] = w;
		}

		void set(float x, float y, float z, float w)
		{
			type = _ClearValueType::FLOAT;
			value.fvec[0] = x;
			value.fvec[1] = y;
			value.fvec[2] = z;
			value.fvec[3] = w;
		}


	};

	struct Attribute
	{
		static constexpr uint32_t INFER_OFFSET_FROM_LAYOUT = 0xFFFFFFFF;
		ResourceFormat format;
		AttributeSemantic semantic;
		uint32_t offset;
	};

	struct TextureDataDefinition
	{
		size_t rowPitchInBytes;
		const void* data;
	};

	struct BufferDesc
	{
		BufferDesc() = default;
		BufferDesc(ResourceUsage usage, size_t size, MemoryType memType = MemoryType::DEFAULT)
			:resourceUsage(usage),
			sizeInBytes(size),
			memoryType(memType)
		{}
		ResourceUsage resourceUsage;
		size_t sizeInBytes;
		MemoryType memoryType = MemoryType::DEFAULT;
	};

	struct TextureDesc
	{
		TextureDesc() = default;
		TextureDesc(ResourceDimension dimensions, ResourceFormat format, ResourceUsage usage, uint32_t width, uint32_t height, uint32_t mips, uint32_t depthOrSlices, MemoryType memType = MemoryType::DEFAULT, ClearValue optimizedClearValue = {}, bool useOptimizedClearValue = false)
			:dimension(dimensions),
			format(format),
			resourceUsage(usage),
			width(width),
			height(height),
			mips(mips),
			depthOrSlices(depthOrSlices),
			optimizedClearValue(optimizedClearValue),
			useOptimizedClearValue(useOptimizedClearValue),
			memoryType(memType)
		{
		}

		ResourceDimension dimension;
		ResourceFormat format;
		ResourceUsage resourceUsage;
		uint32_t width;
		uint32_t height;
		uint32_t mips;
		uint32_t depthOrSlices;
		ClearValue optimizedClearValue;
		MemoryType memoryType = MemoryType::DEFAULT;
		bool useOptimizedClearValue;
	};

	struct ScissorRect
	{
		uint32_t x;
		uint32_t y;
		uint32_t width;
		uint32_t height;
	};

	struct ViewPort
	{
		float x;
		float y;
		float width;
		float height;
		float minDepth;
		float maxDepth;
	};

	struct TextureViewDesc
	{

		ResourceFormat format = ResourceFormat::UNKNOWN;
		ResourceUsage resourceUsage = RESOURCE_USAGE_UNKNOWN;
		ResourceDimension dimensions = ResourceDimension::UNDEFINED;
		uint32_t mipOffset = 0;
		uint32_t mipCount = YAPT_TEXTURE_VIEW_DESC_ALL_MIPS;
		uint32_t arraySliceOffset = 0;
		uint32_t arraySliceCount = YAPT_TEXTURE_VIEW_DESC_ALL_SLICES;

	};

	struct BufferViewDesc
	{
		size_t offsetInBytes = 0;
		size_t sizeInBytes = YAPT_BUFFER_WHOLE_RESOURCE;

		size_t structureStrideInBytes = 0;
		ResourceFormat nonStructuredFormat = ResourceFormat::UNKNOWN;
		BufferViewFlags flags = BufferViewFlagBits::BUFFERVIEWFLAGS_NONE;

	};




	//operators
	inline bool operator==(const TextureViewDesc& a, const TextureViewDesc& b)
	{
		return a.format == b.format &&
			a.dimensions == b.dimensions &&
			a.mipOffset == b.mipOffset &&
			a.mipCount == b.mipCount &&
			a.arraySliceOffset == b.arraySliceOffset &&
			a.arraySliceCount == b.arraySliceCount &&
			a.resourceUsage == b.resourceUsage;
	}

	inline bool operator==(const BufferViewDesc& a, const BufferViewDesc& b)
	{
		return a.offsetInBytes == b.offsetInBytes &&
			a.sizeInBytes == b.sizeInBytes &&
			a.structureStrideInBytes == b.structureStrideInBytes &&
			a.nonStructuredFormat == b.nonStructuredFormat &&
			a.flags == b.flags;
	}

}
#endif