#pragma once

#include <assert.h>
#include <bitset>
#include <Common/CommonUtilities.h>
#define FLAGS_CONVERT(source, target, from, to) if((source & from) != 0) { target |= to; }

namespace YAPT
{
	inline VkBufferUsageFlags yaptUsageToVk(ResourceUsage usage)
	{
		VkBufferUsageFlags flags = 0;

		//make sure our assumptions on these haven't changed
		assert(VK_BUFFER_USAGE_TRANSFER_DST_BIT == VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		assert(VK_BUFFER_USAGE_TRANSFER_SRC_BIT == VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_COPY_DESTINATION, VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_COPY_SOURCE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_UNIFORM_BUFFER, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_UNIFORM_TEXEL_BUFFER, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_STORAGE_BUFFER, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_STORAGE_TEXEL_BUFFER, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_VERTEX_BUFFER, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_INDEX_BUFFER, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_SHADERTABLE_BUFFER, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_ACCELERATION_STRUCTURE_BUFFER, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR);

		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_SAMPLED_TEXTURE, VK_IMAGE_USAGE_SAMPLED_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_STORAGE_TEXTURE, VK_IMAGE_USAGE_STORAGE_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_RENDER_TARGET_TEXTURE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_DEPTH_TEXTURE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_STENCIL_TEXTURE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_PRESENTABLE_TEXTURE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

		return flags;
	}

	inline VkShaderStageFlags yaptShaderStagesToVk(ShaderStages stages)
	{
		VkShaderStageFlags flags = 0;;
		FLAGS_CONVERT(stages, flags, SHADERSTAGE_VERTEX, VK_SHADER_STAGE_VERTEX_BIT);
		FLAGS_CONVERT(stages, flags, SHADERSTAGE_FRAGMENT, VK_SHADER_STAGE_FRAGMENT_BIT);
		FLAGS_CONVERT(stages, flags, SHADERSTAGE_HULL, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
		FLAGS_CONVERT(stages, flags, SHADERSTAGE_DOMAIN, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
		FLAGS_CONVERT(stages, flags, SHADERSTAGE_GEOMETRY, VK_SHADER_STAGE_GEOMETRY_BIT);
		FLAGS_CONVERT(stages, flags, SHADERSTAGE_COMPUTE, VK_SHADER_STAGE_COMPUTE_BIT);

		FLAGS_CONVERT(stages, flags, SHADERSTAGE_RT_RAYGENERATION, VK_SHADER_STAGE_RAYGEN_BIT_KHR);
		FLAGS_CONVERT(stages, flags, SHADERSTAGE_RT_MISS, VK_SHADER_STAGE_MISS_BIT_KHR);
		FLAGS_CONVERT(stages, flags, SHADERSTAGE_RT_ANY_HIT, VK_SHADER_STAGE_ANY_HIT_BIT_KHR);
		FLAGS_CONVERT(stages, flags, SHADERSTAGE_RT_CLOSEST_HIT, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
		FLAGS_CONVERT(stages, flags, SHADERSTAGE_RT_INTERSECTION, VK_SHADER_STAGE_INTERSECTION_BIT_KHR);
		return flags;
	}

	inline VkShaderStageFlagBits yaptShaderStageBitstoVk(ShaderStageBits stage)
	{
		switch (stage)
		{
		case YAPT::SHADERSTAGE_NONE:
			return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		case YAPT::SHADERSTAGE_VERTEX:
			return VK_SHADER_STAGE_VERTEX_BIT;
		case YAPT::SHADERSTAGE_FRAGMENT:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
		case YAPT::SHADERSTAGE_HULL:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case YAPT::SHADERSTAGE_DOMAIN:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case YAPT::SHADERSTAGE_GEOMETRY:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
		case YAPT::SHADERSTAGE_COMPUTE:
			return VK_SHADER_STAGE_COMPUTE_BIT;
		case YAPT::SHADERSTAGE_RT_RAYGENERATION:
			return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		case YAPT::SHADERSTAGE_RT_MISS:
			return VK_SHADER_STAGE_MISS_BIT_KHR;
		case YAPT::SHADERSTAGE_RT_ANY_HIT:
			return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		case YAPT::SHADERSTAGE_RT_CLOSEST_HIT:
			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		case YAPT::SHADERSTAGE_RT_INTERSECTION:
			return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		default:
			return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
		}
	}

	inline VkDescriptorType yaptDescriptorTypeToVk(DescriptorType type)
	{
		switch (type)
		{
		case YAPT::DescriptorType::SAMPLER:
			return VK_DESCRIPTOR_TYPE_SAMPLER;
		case YAPT::DescriptorType::TEXTURE:
			return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case YAPT::DescriptorType::STORAGE_TEXTURE:
			return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case YAPT::DescriptorType::UNIFORM_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case YAPT::DescriptorType::UNIFORM_TEXEL_BUFFER:
			return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case YAPT::DescriptorType::UNIFORM_BUFFER_DYNAMIC:
			return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case YAPT::DescriptorType::STORAGE_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case YAPT::DescriptorType::STORAGE_TEXEL_BUFFER:
			return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case YAPT::DescriptorType::STORAGE_BUFFER_DYNAMIC:
			return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		case YAPT::DescriptorType::ACCELERATION_STRUCTURE:
			return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		default:
			assert(!"unknown descriptor type");
			return VK_DESCRIPTOR_TYPE_MAX_ENUM;
		}
	}

	inline VkImageType YaptResourceDimensionToVk(ResourceDimension dim)
	{
		switch (dim)
		{
		case YAPT::ResourceDimension::TEXTURE_1D:
			return VK_IMAGE_TYPE_1D;
		case YAPT::ResourceDimension::TEXTURE_1D_ARRAY:
			return VK_IMAGE_TYPE_1D;
		case YAPT::ResourceDimension::TEXTURE_2D:
			return VK_IMAGE_TYPE_2D;
		case YAPT::ResourceDimension::TEXTURE_2D_ARRAY:
			return VK_IMAGE_TYPE_2D;
		case YAPT::ResourceDimension::TEXTURE_3D:
			return VK_IMAGE_TYPE_3D;
		case YAPT::ResourceDimension::TEXTURE_CUBEMAP:
			return VK_IMAGE_TYPE_2D;
		case YAPT::ResourceDimension::TEXTURE_CUBEMAP_ARRAY:
			return VK_IMAGE_TYPE_2D;
		default:
			return VK_IMAGE_TYPE_MAX_ENUM;
		}

	}

	inline VkImageViewType yaptResourceDimensionToVkViewType(ResourceDimension dim)
	{
		switch (dim)
		{
		case YAPT::ResourceDimension::TEXTURE_1D:
			return VK_IMAGE_VIEW_TYPE_1D;
		case YAPT::ResourceDimension::TEXTURE_1D_ARRAY:
			return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
		case YAPT::ResourceDimension::TEXTURE_2D:
			return VK_IMAGE_VIEW_TYPE_2D;
		case YAPT::ResourceDimension::TEXTURE_2D_ARRAY:
			return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		case YAPT::ResourceDimension::TEXTURE_3D:
			return VK_IMAGE_VIEW_TYPE_3D;
		case YAPT::ResourceDimension::TEXTURE_CUBEMAP:
			return VK_IMAGE_VIEW_TYPE_CUBE;
		case YAPT::ResourceDimension::TEXTURE_CUBEMAP_ARRAY:
			return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
		default:
			return VK_IMAGE_VIEW_TYPE_MAX_ENUM;
		}

	}

	inline bool isResourceArray(ResourceDimension dim)
	{
		if (dim == ResourceDimension::TEXTURE_1D_ARRAY || dim == ResourceDimension::TEXTURE_2D_ARRAY || dim == ResourceDimension::TEXTURE_CUBEMAP_ARRAY)
		{
			return true;
		}

		return false;
	}

	inline bool isCubemap(ResourceDimension dim)
	{
		return (dim == ResourceDimension::TEXTURE_CUBEMAP || dim == ResourceDimension::TEXTURE_CUBEMAP_ARRAY);
	}

	inline VkFormat yaptFormatToVk(ResourceFormat format)
	{
#define FORMAT_CONV(YAPT, VK) case (ResourceFormat::YAPT): return VK_FORMAT_##VK;

		switch (format)
		{
			FORMAT_CONV(UNKNOWN, UNDEFINED);

			FORMAT_CONV(R8_UNORM, R8_UNORM);
			FORMAT_CONV(R8_SNORM, R8_SNORM);
			FORMAT_CONV(R8_UINT, R8_UINT);
			FORMAT_CONV(R8_SINT, R8_SINT);
			FORMAT_CONV(R8_SRGB, R8_SRGB);

			FORMAT_CONV(R16_UNORM, R16_UNORM);
			FORMAT_CONV(R16_SNORM, R16_SNORM);
			FORMAT_CONV(R16_UINT, R16_UINT);
			FORMAT_CONV(R16_SINT, R16_SINT);
			FORMAT_CONV(R16_SFLOAT, R16_SFLOAT);

			FORMAT_CONV(R32_UINT, R32_UINT);
			FORMAT_CONV(R32_SINT, R32_SINT);
			FORMAT_CONV(R32_SFLOAT, R32_SFLOAT);

			FORMAT_CONV(RG8_UNORM, R8G8_UNORM);
			FORMAT_CONV(RG8_SNORM, R8G8_SNORM);
			FORMAT_CONV(RG8_UINT, R8G8_UINT);
			FORMAT_CONV(RG8_SINT, R8G8_SINT);
			FORMAT_CONV(RG8_SRGB, R8G8_SRGB);

			FORMAT_CONV(RG16_UNORM, R16G16_UNORM);
			FORMAT_CONV(RG16_SNORM, R16G16_SNORM);
			FORMAT_CONV(RG16_UINT, R16G16_UINT);
			FORMAT_CONV(RG16_SINT, R16G16_SINT);
			FORMAT_CONV(RG16_SFLOAT, R16G16_SFLOAT);

			FORMAT_CONV(RG32_UINT, R32G32_UINT);
			FORMAT_CONV(RG32_SINT, R32G32_SINT);
			FORMAT_CONV(RG32_SFLOAT, R32G32_SFLOAT);

			FORMAT_CONV(RGB32_UINT, R32G32B32_UINT);
			FORMAT_CONV(RGB32_SINT, R32G32B32_UINT);
			FORMAT_CONV(RGB32_SFLOAT, R32G32B32_UINT);

			FORMAT_CONV(RGBA8_UNORM, R8G8B8A8_UNORM);
			FORMAT_CONV(RGBA8_SNORM, R8G8B8A8_SNORM);
			FORMAT_CONV(RGBA8_UINT, R8G8B8A8_UINT);
			FORMAT_CONV(RGBA8_SINT, R8G8B8A8_SINT);
			FORMAT_CONV(RGBA8_SRGB, R8G8B8A8_SRGB);

			FORMAT_CONV(RGBA16_UNORM, R16G16B16A16_UNORM);
			FORMAT_CONV(RGBA16_SNORM, R16G16B16A16_SNORM);
			FORMAT_CONV(RGBA16_UINT, R16G16B16A16_UINT);
			FORMAT_CONV(RGBA16_SINT, R16G16B16A16_SINT);
			FORMAT_CONV(RGBA16_SFLOAT, R16G16B16A16_SFLOAT);

			FORMAT_CONV(RGBA32_UINT, R32G32B32A32_UINT);
			FORMAT_CONV(RGBA32_SINT, R32G32B32A32_SINT);
			FORMAT_CONV(RGBA32_SFLOAT, R32G32B32A32_SFLOAT);

			FORMAT_CONV(D16_UNORM, D16_UNORM);
			FORMAT_CONV(D32_SFLOAT, D32_SFLOAT);
			FORMAT_CONV(D24_UNORM_S8_UINT, D24_UNORM_S8_UINT);

			FORMAT_CONV(BC4_UNORM, BC4_UNORM_BLOCK);
			FORMAT_CONV(BC4_SNORM, BC4_SNORM_BLOCK);
			FORMAT_CONV(BC6H_SFLOAT, BC6H_SFLOAT_BLOCK);
			FORMAT_CONV(BC6H_UFLOAT, BC6H_UFLOAT_BLOCK);
			FORMAT_CONV(BC7_UNORM, BC7_UNORM_BLOCK);
			FORMAT_CONV(BC7_UNORM_SRGB, BC7_SRGB_BLOCK);

			default:
				return VK_FORMAT_UNDEFINED;
		}
		

#undef FORMAT_CONV
	}

	inline VkFilter yaptFilterToVk(Filter filter)
	{
		switch (filter)
		{
		case YAPT::Filter::NEAREST:
			return VK_FILTER_NEAREST;
		case YAPT::Filter::LINEAR:
			return VK_FILTER_LINEAR;
		default:
			return VK_FILTER_MAX_ENUM;
		}
	}

	inline VkSamplerAddressMode yaptSamplerAddressModeToVk(SamplerAddressMode mode)
	{
		switch (mode)
		{
		case YAPT::SamplerAddressMode::REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_REPEAT;
		case YAPT::SamplerAddressMode::MIRRORED_REPEAT:
			return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		case YAPT::SamplerAddressMode::CLAMP_TO_EDGE:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		case YAPT::SamplerAddressMode::CLAMP_TO_BORDER:
			return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		case YAPT::SamplerAddressMode::MIRROR_CLAMP_TO_EDGE:
			return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
		default:
			return VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
		}
	}

	inline VkCompareOp yaptCompareOpToVk(CompareOp op)
	{
		switch (op)
		{
		case YAPT::CompareOp::NEVER:
			return VK_COMPARE_OP_NEVER;
		case YAPT::CompareOp::LESS:
			return VK_COMPARE_OP_LESS;
		case YAPT::CompareOp::EQUAL:
			return VK_COMPARE_OP_EQUAL;
		case YAPT::CompareOp::LESS_OR_EQUAL:
			return VK_COMPARE_OP_LESS_OR_EQUAL;
		case YAPT::CompareOp::GREATER:
			return VK_COMPARE_OP_GREATER;
		case YAPT::CompareOp::NOT_EQUAL:
			return VK_COMPARE_OP_NOT_EQUAL;
		case YAPT::CompareOp::GREATER_OR_EQUAL:
			return VK_COMPARE_OP_GREATER_OR_EQUAL;
		case YAPT::CompareOp::ALWAYS:
			return VK_COMPARE_OP_ALWAYS;
		default:
			return VK_COMPARE_OP_NEVER;
		}
	}

	inline VkStencilOp yaptStencilOpToVk(StencilOp ss)
	{
		switch (ss)
		{
		case YAPT::StencilOp::KEEP:
			return VK_STENCIL_OP_KEEP;
		case YAPT::StencilOp::ZERO:
			return VK_STENCIL_OP_ZERO;
		case YAPT::StencilOp::REPLACE:
			return VK_STENCIL_OP_REPLACE;
		case YAPT::StencilOp::INCREMENT_AND_CLAMP:
			return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
		case YAPT::StencilOp::DECREMENT_AND_CLAMP:
			return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
		case YAPT::StencilOp::INVERT:
			return VK_STENCIL_OP_INVERT;
		case YAPT::StencilOp::INCREMENT_AND_WRAP:
			return VK_STENCIL_OP_INCREMENT_AND_WRAP;
		case YAPT::StencilOp::DECREMENT_AND_WRAP:
			return VK_STENCIL_OP_DECREMENT_AND_WRAP;
		default:
			return VK_STENCIL_OP_KEEP;
		}
	}

	inline VkLogicOp yaptLogicOpToVk(LogicOp lo)
	{
		switch (lo)
		{
		case YAPT::LogicOp::CLEAR:
			return VK_LOGIC_OP_CLEAR;
		case YAPT::LogicOp::AND:
			return VK_LOGIC_OP_AND;
		case YAPT::LogicOp::AND_REVERSE:
			return VK_LOGIC_OP_AND_REVERSE;
		case YAPT::LogicOp::COPY:
			return VK_LOGIC_OP_COPY;
		case YAPT::LogicOp::AND_INVERTED:
			return VK_LOGIC_OP_AND_INVERTED;
		case YAPT::LogicOp::NO_OP:
			return VK_LOGIC_OP_NO_OP;
		case YAPT::LogicOp::XOR:
			return VK_LOGIC_OP_XOR;
		case YAPT::LogicOp::OR:
			return VK_LOGIC_OP_OR;
		case YAPT::LogicOp::NOR:
			return VK_LOGIC_OP_NOR;
		case YAPT::LogicOp::EQUIVALENT:
			return VK_LOGIC_OP_EQUIVALENT;
		case YAPT::LogicOp::INVERT:
			return VK_LOGIC_OP_INVERT;
		case YAPT::LogicOp::OR_REVERSE:
			return VK_LOGIC_OP_OR_REVERSE;
		case YAPT::LogicOp::COPY_INVERTED:
			return VK_LOGIC_OP_COPY_INVERTED;
		case YAPT::LogicOp::OR_INVERTED:
			return VK_LOGIC_OP_OR_INVERTED;
		case YAPT::LogicOp::NAND:
			return VK_LOGIC_OP_NAND;
		case YAPT::LogicOp::SET:
			return VK_LOGIC_OP_SET;
		default:
			return VK_LOGIC_OP_NO_OP;
		}
	}

	inline VkBlendOp yaptBlendOpToVk(BlendOp bo)
	{
		switch (bo)
		{
		case YAPT::BlendOp::ADD:
			return VK_BLEND_OP_ADD;
		case YAPT::BlendOp::SUBTRACT:
			return VK_BLEND_OP_SUBTRACT;
		case YAPT::BlendOp::REVERSE_SUBTRACT:
			return VK_BLEND_OP_REVERSE_SUBTRACT;
		case YAPT::BlendOp::MIN:
			return VK_BLEND_OP_MIN;
		case YAPT::BlendOp::MAX:
			return VK_BLEND_OP_MAX;
		default:
			return VK_BLEND_OP_ADD;
		}
	}

	inline VkBlendFactor yaptBlendFactorToVk(BlendFactor bf)
	{
		switch (bf)
		{
		case YAPT::BlendFactor::ZERO:
			return VK_BLEND_FACTOR_ZERO;
		case YAPT::BlendFactor::ONE:
			return VK_BLEND_FACTOR_ONE;
		case YAPT::BlendFactor::SRC_COLOR:
			return VK_BLEND_FACTOR_SRC_COLOR;
		case YAPT::BlendFactor::ONE_MINUS_SRC_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		case YAPT::BlendFactor::DST_COLOR:
			return VK_BLEND_FACTOR_DST_COLOR;
		case YAPT::BlendFactor::ONE_MINUS_DST_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
		case YAPT::BlendFactor::SRC_ALPHA:
			return VK_BLEND_FACTOR_SRC_ALPHA;
		case YAPT::BlendFactor::ONE_MINUS_SRC_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		case YAPT::BlendFactor::DST_ALPHA:
			return VK_BLEND_FACTOR_DST_ALPHA;
		case YAPT::BlendFactor::ONE_MINUS_DST_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
		case YAPT::BlendFactor::CONSTANT_COLOR:
			return VK_BLEND_FACTOR_CONSTANT_COLOR;
		case YAPT::BlendFactor::ONE_MINUS_CONSTANT_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
		case YAPT::BlendFactor::CONSTANT_ALPHA:
			return VK_BLEND_FACTOR_CONSTANT_ALPHA;
		case YAPT::BlendFactor::ONE_MINUS_CONSTANT_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
		case YAPT::BlendFactor::SRC_ALPHA_SATURATE:
			return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
		case YAPT::BlendFactor::SRC1_COLOR:
			return VK_BLEND_FACTOR_SRC1_COLOR;
		case YAPT::BlendFactor::ONE_MINUS_SRC1_COLOR:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
		case YAPT::BlendFactor::SRC1_ALPHA:
			return VK_BLEND_FACTOR_SRC1_ALPHA;
		case YAPT::BlendFactor::ONE_MINUS_SRC1_ALPHA:
			return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
		default:
			return VK_BLEND_FACTOR_ONE;
		}
	}

	inline VkColorComponentFlags yaptColorMaskToVk(ColorMask cm)
	{
		VkColorComponentFlags mask = 0;
		FLAGS_CONVERT(cm, mask, ColorMaskBits::RED, VK_COLOR_COMPONENT_R_BIT);
		FLAGS_CONVERT(cm, mask, ColorMaskBits::GREEN, VK_COLOR_COMPONENT_G_BIT);
		FLAGS_CONVERT(cm, mask, ColorMaskBits::BLUE, VK_COLOR_COMPONENT_B_BIT);
		FLAGS_CONVERT(cm, mask, ColorMaskBits::ALPHA, VK_COLOR_COMPONENT_A_BIT);
		return mask;
	}

	inline VkStencilOpState yaptStencilStateToVk(StencilState ss, uint32_t writeMask, uint32_t compareMask)
	{
		VkStencilOpState state;
		state.compareOp = yaptCompareOpToVk(ss.compareOp);
		state.depthFailOp = yaptStencilOpToVk(ss.depthFailOp);
		state.failOp = yaptStencilOpToVk(ss.failOp);
		state.passOp = yaptStencilOpToVk(ss.passOp);
		state.reference = ss.reference;
		state.writeMask = writeMask;
		state.compareMask = compareMask;

		return state;
	}

	inline VkBorderColor yaptBorderColorToVk(BorderColor color)
	{
		switch (color)
		{
		case YAPT::BorderColor::FLOAT_TRANSPARENT_BLACK:
			return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
		case YAPT::BorderColor::INT_TRANSPARENT_BLACK:
			return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
		case YAPT::BorderColor::FLOAT_OPAQUE_BLACK:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
		case YAPT::BorderColor::INT_OPAQUE_BLACK:
			return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		case YAPT::BorderColor::FLOAT_OPAQUE_WHITE:
			return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		case YAPT::BorderColor::INT_OPAQUE_WHITE:
			return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
		default:
			return VK_BORDER_COLOR_MAX_ENUM;
		}
	}

	inline VkImageAspectFlags yaptUsageToAspectFlags(ResourceUsage usage)
	{
		VkImageAspectFlags flags = 0;
		if (usage & RESOURCE_USAGE_DEPTH_TEXTURE)
			flags |= VK_IMAGE_ASPECT_DEPTH_BIT;
		if (usage & RESOURCE_USAGE_STENCIL_TEXTURE)
			flags |= VK_IMAGE_ASPECT_STENCIL_BIT;
		if (usage & (RESOURCE_USAGE_RENDER_TARGET_TEXTURE | RESOURCE_USAGE_STORAGE_TEXTURE | RESOURCE_USAGE_SAMPLED_TEXTURE | RESOURCE_USAGE_PRESENTABLE_TEXTURE))
			flags |= VK_IMAGE_ASPECT_COLOR_BIT;

		return flags;
	}

	inline VkAccessFlags yaptUsageAccessToVkAccess(ResourceUsage usage, AccessFlags accessFlags)
	{
		VkAccessFlags shaderReadWriteFlags = 0;
		FLAGS_CONVERT(accessFlags, shaderReadWriteFlags, ACCESS_FLAGS_WRITE, VK_ACCESS_SHADER_WRITE_BIT);
		FLAGS_CONVERT(accessFlags, shaderReadWriteFlags, ACCESS_FLAGS_READ, VK_ACCESS_SHADER_READ_BIT);

		VkAccessFlags flags = 0;
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_COPY_DESTINATION, VK_ACCESS_TRANSFER_WRITE_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_COPY_SOURCE, VK_ACCESS_TRANSFER_READ_BIT);

		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_UNIFORM_BUFFER, VK_ACCESS_UNIFORM_READ_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_UNIFORM_TEXEL_BUFFER, VK_ACCESS_UNIFORM_READ_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_STORAGE_BUFFER, shaderReadWriteFlags);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_STORAGE_TEXEL_BUFFER, shaderReadWriteFlags);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_VERTEX_BUFFER, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_INDEX_BUFFER, VK_ACCESS_INDEX_READ_BIT);

		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_SHADERTABLE_BUFFER, shaderReadWriteFlags); 

		if ((usage & RESOURCE_USAGE_ACCELERATION_STRUCTURE_BUFFER) != 0)
		{
			if ((accessFlags & ACCESS_FLAGS_WRITE))
				flags |= VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			if ((accessFlags & ACCESS_FLAGS_READ))
				flags |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
		}
		
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_SAMPLED_TEXTURE, VK_ACCESS_UNIFORM_READ_BIT);
		FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_STORAGE_TEXTURE, shaderReadWriteFlags);

		if ((usage & RESOURCE_USAGE_RENDER_TARGET_TEXTURE) != 0)
		{
			if ((accessFlags & ACCESS_FLAGS_WRITE))
				flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			if ((accessFlags & ACCESS_FLAGS_READ))
				flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		}

		if ((usage & (RESOURCE_USAGE_DEPTH_TEXTURE | RESOURCE_USAGE_STENCIL_TEXTURE)) != 0)
		{
			if ((accessFlags & ACCESS_FLAGS_WRITE))
				flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			if ((accessFlags & ACCESS_FLAGS_READ))
				flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		}

		//FLAGS_CONVERT(usage, flags, RESOURCE_USAGE_PRESENTABLE_TEXTURE, VK_ACCESS_UNIFORM_READ_BIT); //???
		return flags;
	}

	inline VkImageLayout yaptUsageToVkImageLayout(ResourceUsage usage, AccessFlags accessFlags)
	{
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		bool hasWrite = (accessFlags & ACCESS_FLAGS_WRITE) != 0;
		switch (usage)
		{
		
			case RESOURCE_USAGE_COPY_DESTINATION:
				layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				break;
			case RESOURCE_USAGE_COPY_SOURCE:
				layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				break;
			case RESOURCE_USAGE_SAMPLED_TEXTURE:
				layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				break;
			case RESOURCE_USAGE_STORAGE_TEXTURE:
				layout = VK_IMAGE_LAYOUT_GENERAL;
				/*
				if (hasWrite)
				{
					layout = VK_IMAGE_LAYOUT_GENERAL;
				}
				else
				{
					layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				}*/
				break;
			case RESOURCE_USAGE_RENDER_TARGET_TEXTURE:
				layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				break;
			case RESOURCE_USAGE_DEPTH_TEXTURE:
				if (hasWrite)
				{
					layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				}
				else
				{
					layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
				}
				break;
			case RESOURCE_USAGE_STENCIL_TEXTURE:
				if (hasWrite)
				{
					layout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else
				{
					layout = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
				}
				break;
			case RESOURCE_USAGE_DEPTH_STENCIL_TEXTURE:
				if (hasWrite)
				{
					layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				}
				else
				{
					layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				}
				break;
			case RESOURCE_USAGE_PRESENTABLE_TEXTURE:
				layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
				break;
		}
		return layout;
	}

	inline VkImageLayout vkDescriptorTypeToVkImageLayout(VkDescriptorType descType)
	{
		VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
		switch (descType)
		{
			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
				layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				break;
			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
				layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				break;
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
				layout = VK_IMAGE_LAYOUT_GENERAL;
				break;
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
				layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				break;
		
		}
		return layout;
	}

	inline VkSampleCountFlagBits yaptSampleCountToVk(SampleCount sampleCount)
	{
		switch (sampleCount)
		{
		case YAPT::SampleCount::SAMPLE_COUNT_1:
			return VK_SAMPLE_COUNT_1_BIT;
		case YAPT::SampleCount::SAMPLE_COUNT_2:
			return VK_SAMPLE_COUNT_2_BIT;
		case YAPT::SampleCount::SAMPLE_COUNT_4:
			return VK_SAMPLE_COUNT_4_BIT;
		case YAPT::SampleCount::SAMPLE_COUNT_8:
			return VK_SAMPLE_COUNT_8_BIT;
		case YAPT::SampleCount::SAMPLE_COUNT_16:
			return VK_SAMPLE_COUNT_16_BIT;
		case YAPT::SampleCount::SAMPLE_COUNT_32:
			return VK_SAMPLE_COUNT_32_BIT;
		case YAPT::SampleCount::SAMPLE_COUNT_64:
			return VK_SAMPLE_COUNT_64_BIT;
		default:
			return VK_SAMPLE_COUNT_1_BIT;
		}
	}

	inline VkPolygonMode yaptPolygonModeToVk(PolygonMode polyMode)
	{
		switch (polyMode)
		{
		case YAPT::PolygonMode::FILL:
			return VK_POLYGON_MODE_FILL;
		case YAPT::PolygonMode::LINE:
			return VK_POLYGON_MODE_LINE;
		case YAPT::PolygonMode::POINT:
			return VK_POLYGON_MODE_POINT;
		default:
			return VK_POLYGON_MODE_FILL;
		}
	}

	inline VkCullModeFlags yaptCullModeToVk(CullMode cullMode)
	{
		switch (cullMode)
		{
		case YAPT::CullMode::NONE:
			return VK_CULL_MODE_NONE;
		case YAPT::CullMode::FRONT:
			return VK_CULL_MODE_FRONT_BIT;
		case YAPT::CullMode::BACK:
			return VK_CULL_MODE_BACK_BIT;
		default:
			return VK_CULL_MODE_NONE;
		}
	}

	inline VkFrontFace yaptWindingOrderToVk(FrontFace frontFace)
	{
		switch (frontFace)
		{
		case YAPT::FrontFace::COUNTER_CLOCKWISE:
			return VK_FRONT_FACE_CLOCKWISE;
		case YAPT::FrontFace::CLOCKWISE:
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		default:
			return VK_FRONT_FACE_COUNTER_CLOCKWISE;
		}
	}

	inline VkPrimitiveTopology yaptPrimitiveTopologyToVk(PrimitiveTopology pt)
	{
		switch (pt)
		{
		case YAPT::PrimitiveTopology::POINTLIST:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		case YAPT::PrimitiveTopology::LINELIST:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		case YAPT::PrimitiveTopology::LINESTRIP:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
		case YAPT::PrimitiveTopology::TRIANGLELIST:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		case YAPT::PrimitiveTopology::TRIANGLESTRIP:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
		case YAPT::PrimitiveTopology::LINELIST_WITH_ADJACENCY:
			return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
		case YAPT::PrimitiveTopology::LINESTRIP_WITH_ADJACENCY:
			return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
		case YAPT::PrimitiveTopology::TRIANGLELIST_WITH_ADJACENCY:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
		case YAPT::PrimitiveTopology::TRIANGLESTRIP_WITH_ADJACENCY:
			return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
		case YAPT::PrimitiveTopology::PATCHLIST:
			return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		default:
			return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
		}
	}
	
	inline VkDynamicState yaptDynamicStateToVk(DynamicPipelineStateBits bitt)
	{
		switch (bitt)
		{
		case YAPT::DYNAMIC_PIPELINESTATE_VIEWPORT:
			return VK_DYNAMIC_STATE_VIEWPORT;
		case YAPT::DYNAMIC_PIPELINESTATE_SCISSOR:
			return VK_DYNAMIC_STATE_SCISSOR;
		case YAPT::DYNAMIC_PIPELINESTATE_LINE_WIDTH:
			return VK_DYNAMIC_STATE_LINE_WIDTH;
		case YAPT::DYNAMIC_PIPELINESTATE_DEPTH_BIAS:
			return VK_DYNAMIC_STATE_DEPTH_BIAS;
		case YAPT::DYNAMIC_PIPELINESTATE_BLEND_CONSTANTS:
			return VK_DYNAMIC_STATE_BLEND_CONSTANTS;
		case YAPT::DYNAMIC_PIPELINESTATE_DEPTH_BOUNDS:
			return VK_DYNAMIC_STATE_DEPTH_BOUNDS;
		case YAPT::DYNAMIC_PIPELINESTATE_STENCIL_COMPARE_MASK:
			return VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
		case YAPT::DYNAMIC_PIPELINESTATE_STENCIL_WRITE_MASK:
			return VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
		case YAPT::DYNAMIC_PIPELINESTATE_STENCIL_REFERENCE:
			return VK_DYNAMIC_STATE_STENCIL_REFERENCE;
		default:
			return VK_DYNAMIC_STATE_MAX_ENUM;
		}
	}

	inline VkPipelineBindPoint yaptBindingPointToVk(BindingPoint bind)
	{
		switch (bind)
		{
		case YAPT::BindingPoint::BINDING_POINT_GRAPHICS:
			return VK_PIPELINE_BIND_POINT_GRAPHICS;
		case YAPT::BindingPoint::BINDING_POINT_COMPUTE:
			return VK_PIPELINE_BIND_POINT_COMPUTE;
		case YAPT::BindingPoint::BINDING_POINT_RAYTRACE:
			return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
		default:
			break;
		}
	}

	inline uint32_t getDynamicStatesCount(DynamicPipelineStates dps)
	{
		std::bitset<sizeof(uint32_t) * 8> bs(dps);
		return (uint32_t)bs.count();
	}

	inline void yaptDynamicPipelineStateToVk(DynamicPipelineStates dps, VkDynamicState* dynamicStatesOut)
	{
		uint32_t count = getDynamicStatesCount(dps);
		for (uint32_t i = 0; i < count; ++i)
		{
			uint32_t bit = getLSB(dps);
			dynamicStatesOut[i] = yaptDynamicStateToVk((DynamicPipelineStateBits)bit);
			dps = ~bit & dps;
		}
	}

	inline void yaptBufferDescToVk(const YAPT::BufferDesc& desc, VkBufferCreateInfo& infoOut)
	{
		infoOut ={};
		infoOut.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		infoOut.size = desc.sizeInBytes;
		infoOut.usage = yaptUsageToVk(desc.resourceUsage);
		infoOut.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	inline void yaptTextureDescToVk(const YAPT::TextureDesc& desc, VkImageCreateInfo& infoOut, bool useLinearMemory = false)
	{
		infoOut = {};
		infoOut.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		infoOut.usage = yaptUsageToVk(desc.resourceUsage);
		infoOut.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		infoOut.format = yaptFormatToVk(desc.format);
		infoOut.imageType = YaptResourceDimensionToVk(desc.dimension);
		if (desc.dimension == ResourceDimension::TEXTURE_3D)
		{
			infoOut.arrayLayers = 1;
			infoOut.extent.depth = desc.depthOrSlices;
			infoOut.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;

		}
		else
		{
			infoOut.arrayLayers = desc.depthOrSlices;
			infoOut.extent.depth = 1;
		}

		if (isCubemap(desc.dimension))
		{
			infoOut.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
		}

		infoOut.extent.width = desc.width;
		infoOut.extent.height = desc.height;
		
		infoOut.mipLevels = desc.mips;
		infoOut.tiling = useLinearMemory ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
		infoOut.samples = VK_SAMPLE_COUNT_1_BIT;

	
	}

	inline void yaptSamplerDescToVk(const YAPT::SamplerDescription& desc, VkSamplerCreateInfo& infoOut)
	{
		infoOut = {};
		infoOut.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		infoOut.magFilter = yaptFilterToVk(desc.magFilter);
		infoOut.minFilter = yaptFilterToVk(desc.minFilter);
		infoOut.mipmapMode = desc.mipmapMode == Filter::NEAREST ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
		infoOut.addressModeU = yaptSamplerAddressModeToVk(desc.addressModeU);
		infoOut.addressModeV = yaptSamplerAddressModeToVk(desc.addressModeV);
		infoOut.addressModeW = yaptSamplerAddressModeToVk(desc.addressModeW);
		infoOut.mipLodBias = desc.mipLodBias;
		infoOut.anisotropyEnable = desc.enableAnisotropy;
		infoOut.maxAnisotropy = desc.maxAnisotropy;
		infoOut.compareEnable = desc.enableCompare;
		infoOut.compareOp = yaptCompareOpToVk(desc.compareOp);
		infoOut.minLod = desc.minLod;
		infoOut.maxLod = desc.maxLod;
		infoOut.borderColor = yaptBorderColorToVk(desc.borderColor);
		infoOut.unnormalizedCoordinates = false;
	}

	inline void yaptDepthStencilStateToVk(const YAPT::DepthStencilStateDescription& desc, VkPipelineDepthStencilStateCreateInfo& infoOut)
	{
		infoOut = {};
		infoOut.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		infoOut.depthCompareOp = yaptCompareOpToVk(desc.depthCompareOp);
		infoOut.depthBoundsTestEnable = desc.depthBoundsTestEnable;
		infoOut.depthTestEnable = desc.depthTestEnable;
		infoOut.depthWriteEnable = desc.depthWriteEnable;
		infoOut.maxDepthBounds = desc.maxDepthBounds;
		infoOut.minDepthBounds = desc.minDepthBounds;
		infoOut.stencilTestEnable = desc.stencilTestEnable;
		infoOut.front = yaptStencilStateToVk(desc.stencilFront, desc.writeMask, desc.compareMask);
		infoOut.back = yaptStencilStateToVk(desc.stencilBack, desc.writeMask, desc.compareMask);
		infoOut.flags = 0;

	}

	inline void yaptColorBlendStateToVk(const YAPT::BlendStateDescription& desc, VkPipelineColorBlendStateCreateInfo& infoOut, VkPipelineColorBlendAttachmentState* attachmentStateOut)
	{
		infoOut = {};
		infoOut.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		infoOut.flags = 0;

		for (uint32_t i = 0; i < 4; ++i)
		{
			infoOut.blendConstants[i] = desc.blendConstants[i];
		}
		infoOut.logicOpEnable = desc.enableLogicalOp;
		infoOut.logicOp = yaptLogicOpToVk(desc.logicalOp);

		infoOut.attachmentCount = desc.numberOfBlendTargets;
		for (uint32_t i = 0; i < desc.numberOfBlendTargets; ++i)
		{
			VkPipelineColorBlendAttachmentState& stateOut = attachmentStateOut[i];
			BlendTargetDescription& stateIn = desc.blendTargetDescriptions[i];

			stateOut.blendEnable = stateIn.blendEnable;
			stateOut.colorWriteMask = yaptColorMaskToVk(stateIn.colorWriteMask);
			stateOut.alphaBlendOp = yaptBlendOpToVk(stateIn.alphaBlendOp);
			stateOut.colorBlendOp = yaptBlendOpToVk(stateIn.colorBlendOp);
			stateOut.dstAlphaBlendFactor = yaptBlendFactorToVk(stateIn.dstAlpha);
			stateOut.srcAlphaBlendFactor = yaptBlendFactorToVk(stateIn.srcAlpha);
			stateOut.dstColorBlendFactor = yaptBlendFactorToVk(stateIn.dstColor);
			stateOut.srcColorBlendFactor = yaptBlendFactorToVk(stateIn.srcColor);


		}
	}

	inline void yaptRasterizerStateToVk(const YAPT::RasterizerStateDescription& desc, VkPipelineRasterizationStateCreateInfo& infoOut)
	{
		infoOut = {};
		infoOut.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		infoOut.flags = 0;
		infoOut.depthClampEnable = desc.depthClampEnable;
		infoOut.rasterizerDiscardEnable = desc.rasterizerDiscardEnable;
		infoOut.polygonMode = yaptPolygonModeToVk(desc.polygonMode);
		infoOut.cullMode = yaptCullModeToVk(desc.cullMode);
		infoOut.frontFace = yaptWindingOrderToVk(desc.frontFace);
		infoOut.depthBiasEnable = desc.depthBiasEnable;
		infoOut.depthBiasConstantFactor = desc.depthBiasConstantFactor;
		infoOut.depthBiasClamp = desc.depthBiasClamp;
		infoOut.depthBiasSlopeFactor = desc.depthBiasSlopeFactor;
		infoOut.lineWidth = desc.lineWidth;

	}

	inline void yaptMultisampleStateToVk(const YAPT::MultisampleStateDescription& desc, VkPipelineMultisampleStateCreateInfo& infoOut, VkSampleMask sampleMask[2])
	{
		infoOut = {};
		infoOut.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		infoOut.flags = 0;
		infoOut.alphaToCoverageEnable = desc.enableAlphaToCoverage;
		infoOut.alphaToOneEnable = desc.enableAlphaToOne;
		infoOut.minSampleShading = desc.minSampleShading;
		infoOut.sampleShadingEnable = desc.enableSampleShading;
		infoOut.rasterizationSamples = yaptSampleCountToVk(desc.sampleCount);
		infoOut.pSampleMask = sampleMask;

		sampleMask[0] = uint32_t(desc.sampleMask & 0xFFFFFFFF);
		sampleMask[1] = uint32_t(desc.sampleMask >> 32);
	}

	inline void yaptViewportToVk(const YAPT::ViewPort& desc, VkViewport& viewportOut)
	{
		viewportOut.x = desc.x;
		viewportOut.y = desc.y;
		viewportOut.width = desc.width;
		viewportOut.height = desc.height;
		viewportOut.minDepth = desc.minDepth;
		viewportOut.maxDepth = desc.maxDepth;

	}

	inline void yaptScissorsToVk(const YAPT::ScissorRect& desc, VkRect2D& scissorsOut)
	{
		scissorsOut.offset.x = desc.x;
		scissorsOut.offset.y = desc.y;
		scissorsOut.extent.width = desc.width;
		scissorsOut.extent.height= desc.height;

	}

}