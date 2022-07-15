#pragma once

#include <assert.h>

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


}