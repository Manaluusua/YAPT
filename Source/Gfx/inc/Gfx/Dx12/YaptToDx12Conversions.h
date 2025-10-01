#ifndef YAPT_DX12_CONVERSIONS_H
#define YAPT_DX12_CONVERSIONS_H

#include <Gfx/GfxBasicTypes.h>
#include <Gfx/Dx12/Dx12CommonIncludes.h>
#include <Gfx/GfxBasicTypesUtility.h>
#include <Gfx/RenderGraph/RenderGraphResourceRequirements.h>

#define YAPT_FORMAT_CONV(yaptFormat, dx12Format) case ResourceFormat::yaptFormat: return dx12Format;



#define YAPT_FLAG_CONV(input, output, from, to) if(((input) & (from)) == (from)) (output) |= (to)

namespace YAPT
{
	inline uint32_t getMinimumAlignmentForBufferUsage(ResourceUsage resourceUsage)
	{
		int maxAlignment = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT;
		if ((resourceUsage & (RESOURCE_USAGE_UNIFORM_BUFFER)) != 0)
		{
			maxAlignment = max(maxAlignment, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
		}
		if ((resourceUsage & RESOURCE_USAGE_ACCELERATION_STRUCTURE_BUFFER) != 0)
		{
			maxAlignment = max(maxAlignment, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT);
		}
		if ((resourceUsage & RESOURCE_USAGE_SHADERTABLE_BUFFER) != 0)
		{
			maxAlignment = max(maxAlignment, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

		}


		return maxAlignment;

	}

	inline D3D12_RESOURCE_DIMENSION yaptToDx12ResourceDimensions(ResourceDimension dim)
	{
		switch (dim)
		{
		case ResourceDimension::TEXTURE_1D:
		case ResourceDimension::TEXTURE_1D_ARRAY:
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;

		case ResourceDimension::TEXTURE_2D:
		case ResourceDimension::TEXTURE_2D_ARRAY:
		case ResourceDimension::TEXTURE_CUBEMAP:
		case ResourceDimension::TEXTURE_CUBEMAP_ARRAY:
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		case ResourceDimension::TEXTURE_3D:
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		default:
			return D3D12_RESOURCE_DIMENSION_UNKNOWN;
		}
	}


	inline D3D12_SHADER_VISIBILITY yaptToDx12ShaderVisibility(ShaderStages stages)
	{
		//TODO: implement
		return D3D12_SHADER_VISIBILITY_ALL;
	}
	inline D3D12_DESCRIPTOR_RANGE_TYPE yaptToDx12DescriptorRangeType(const DescriptorSetLayoutBinding& binding)
	{
		if (binding.type == DescriptorType::UNIFORM_BUFFER || binding.type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
		{
			return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		}
		else if (binding.type == DescriptorType::SAMPLER)
		{
			return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		}
		else if (binding.type == DescriptorType::STORAGE_BUFFER || binding.type == DescriptorType::STORAGE_TEXEL_BUFFER||
			binding.type == DescriptorType::STORAGE_BUFFER_DYNAMIC || binding.type == DescriptorType::STORAGE_TEXTURE )
		{
			if ((binding.accessFlags & ACCESS_FLAGS_WRITE) == 0)
			{
				return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			}
			else
			{
				return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
			}
		}

		return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

	}

	inline D3D12_PRIMITIVE_TOPOLOGY_TYPE yaptToDx12PrimitiveTopologyType(PrimitiveTopology top)
	{
		switch (top)
		{
		case YAPT::PrimitiveTopology::POINTLIST:

			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;

		case YAPT::PrimitiveTopology::LINELIST:
		case YAPT::PrimitiveTopology::LINESTRIP:
		case YAPT::PrimitiveTopology::LINELIST_WITH_ADJACENCY:
		case YAPT::PrimitiveTopology::LINESTRIP_WITH_ADJACENCY:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		
			
		case YAPT::PrimitiveTopology::TRIANGLELIST:
		case YAPT::PrimitiveTopology::TRIANGLESTRIP:
		case YAPT::PrimitiveTopology::TRIANGLELIST_WITH_ADJACENCY:
		case YAPT::PrimitiveTopology::TRIANGLESTRIP_WITH_ADJACENCY:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		case YAPT::PrimitiveTopology::PATCHLIST:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
		default:
			return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
		}
	}

#define PATCHPRIMTOPOLOGYNAME(number) case number: return D3D_PRIMITIVE_TOPOLOGY_##number##_CONTROL_POINT_PATCHLIST

	inline D3D12_PRIMITIVE_TOPOLOGY toDx12PathPrimitiveTopology(uint32_t patchCount)
	{
		switch (patchCount)
		{
			PATCHPRIMTOPOLOGYNAME(1);
			PATCHPRIMTOPOLOGYNAME(2);
			PATCHPRIMTOPOLOGYNAME(3);
			PATCHPRIMTOPOLOGYNAME(4);
			PATCHPRIMTOPOLOGYNAME(5);
			PATCHPRIMTOPOLOGYNAME(6);
			PATCHPRIMTOPOLOGYNAME(7);
			PATCHPRIMTOPOLOGYNAME(8);
			PATCHPRIMTOPOLOGYNAME(9);
			PATCHPRIMTOPOLOGYNAME(10);
			PATCHPRIMTOPOLOGYNAME(11);
			PATCHPRIMTOPOLOGYNAME(12);
			PATCHPRIMTOPOLOGYNAME(13);
			PATCHPRIMTOPOLOGYNAME(14);
			PATCHPRIMTOPOLOGYNAME(15);
			PATCHPRIMTOPOLOGYNAME(16);
			PATCHPRIMTOPOLOGYNAME(17);
			PATCHPRIMTOPOLOGYNAME(18);
			PATCHPRIMTOPOLOGYNAME(19);
			PATCHPRIMTOPOLOGYNAME(21);
			PATCHPRIMTOPOLOGYNAME(22);
			PATCHPRIMTOPOLOGYNAME(23);
			PATCHPRIMTOPOLOGYNAME(24);
			PATCHPRIMTOPOLOGYNAME(25);
			PATCHPRIMTOPOLOGYNAME(26);
			PATCHPRIMTOPOLOGYNAME(27);
			PATCHPRIMTOPOLOGYNAME(28);
			PATCHPRIMTOPOLOGYNAME(29);
			PATCHPRIMTOPOLOGYNAME(30);
			PATCHPRIMTOPOLOGYNAME(31);
			PATCHPRIMTOPOLOGYNAME(32);
		default:
		{
			YAPT_LOG_FATAL_ERROR("UNSUPPORTED PATCH TOPOLOGY ENCOUNTERED!");
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
			
		}
	}
#undef PATCHPRIMTOPOLOGYNAME
	inline D3D12_PRIMITIVE_TOPOLOGY yaptToDx12PrimitiveTopology(PrimitiveTopology top, uint32_t patchCount)
	{
		switch (top)
		{
		case YAPT::PrimitiveTopology::POINTLIST:
			return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case YAPT::PrimitiveTopology::LINELIST:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case YAPT::PrimitiveTopology::LINESTRIP:
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case YAPT::PrimitiveTopology::LINELIST_WITH_ADJACENCY:
			return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
		case YAPT::PrimitiveTopology::LINESTRIP_WITH_ADJACENCY:
			return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
		case YAPT::PrimitiveTopology::TRIANGLELIST:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case YAPT::PrimitiveTopology::TRIANGLESTRIP:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case YAPT::PrimitiveTopology::TRIANGLELIST_WITH_ADJACENCY:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
		case YAPT::PrimitiveTopology::TRIANGLESTRIP_WITH_ADJACENCY:
			return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
		case YAPT::PrimitiveTopology::PATCHLIST:
		{
			return toDx12PathPrimitiveTopology(patchCount);
		}

		default:
			return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
		}
	}

	inline DXGI_FORMAT yaptToDx12Format(ResourceFormat format)
	{
		switch (format)
		{
			YAPT_FORMAT_CONV(R8_UNORM, DXGI_FORMAT_R8_UNORM)
			YAPT_FORMAT_CONV(R8_SNORM, DXGI_FORMAT_R8_SNORM)
			YAPT_FORMAT_CONV(R8_UINT, DXGI_FORMAT_R8_UINT)
			YAPT_FORMAT_CONV(R8_SINT, DXGI_FORMAT_R8_SINT)
			YAPT_FORMAT_CONV(R8_SRGB, DXGI_FORMAT_R8_UNORM)

			YAPT_FORMAT_CONV(R16_UNORM, DXGI_FORMAT_R16_UNORM)
			YAPT_FORMAT_CONV(R16_SNORM, DXGI_FORMAT_R16_SNORM)
			YAPT_FORMAT_CONV(R16_UINT,  DXGI_FORMAT_R16_UINT)
			YAPT_FORMAT_CONV(R16_SINT,  DXGI_FORMAT_R16_SINT)
			YAPT_FORMAT_CONV(R16_SFLOAT,DXGI_FORMAT_R16_FLOAT)

			YAPT_FORMAT_CONV(R32_UINT, DXGI_FORMAT_R32_UINT)
			YAPT_FORMAT_CONV(R32_SINT, DXGI_FORMAT_R32_SINT)
			YAPT_FORMAT_CONV(R32_SFLOAT, DXGI_FORMAT_R32_FLOAT)

			YAPT_FORMAT_CONV(RG8_UNORM, DXGI_FORMAT_R8G8_UNORM)
			YAPT_FORMAT_CONV(RG8_SNORM, DXGI_FORMAT_R8G8_SNORM)
			YAPT_FORMAT_CONV(RG8_UINT, DXGI_FORMAT_R8G8_UINT)
			YAPT_FORMAT_CONV(RG8_SINT, DXGI_FORMAT_R8G8_SINT)
			YAPT_FORMAT_CONV(RG8_SRGB, DXGI_FORMAT_R8G8_UNORM)

			YAPT_FORMAT_CONV(RG16_UNORM, DXGI_FORMAT_R16G16_UNORM)
			YAPT_FORMAT_CONV(RG16_SNORM, DXGI_FORMAT_R16G16_SNORM)
			YAPT_FORMAT_CONV(RG16_UINT, DXGI_FORMAT_R16G16_UINT)
			YAPT_FORMAT_CONV(RG16_SINT, DXGI_FORMAT_R16G16_SINT)
			YAPT_FORMAT_CONV(RG16_SFLOAT, DXGI_FORMAT_R16G16_FLOAT)

			YAPT_FORMAT_CONV(RG32_UINT, DXGI_FORMAT_R32G32_UINT)
			YAPT_FORMAT_CONV(RG32_SINT, DXGI_FORMAT_R32G32_SINT)
			YAPT_FORMAT_CONV(RG32_SFLOAT, DXGI_FORMAT_R32G32_FLOAT)

			YAPT_FORMAT_CONV(RGBA8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM)
			YAPT_FORMAT_CONV(RGBA8_SNORM, DXGI_FORMAT_R8G8B8A8_SNORM)
			YAPT_FORMAT_CONV(RGBA8_UINT, DXGI_FORMAT_R8G8B8A8_UINT)
			YAPT_FORMAT_CONV(RGBA8_SINT, DXGI_FORMAT_R8G8B8A8_SINT)
			YAPT_FORMAT_CONV(RGBA8_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)

			YAPT_FORMAT_CONV(BGRA8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM)
			YAPT_FORMAT_CONV(BGRA8_SRGB, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB)

			YAPT_FORMAT_CONV(RGBA16_UNORM , DXGI_FORMAT_R16G16B16A16_UNORM)
			YAPT_FORMAT_CONV(RGBA16_SNORM, DXGI_FORMAT_R16G16B16A16_SNORM)
			YAPT_FORMAT_CONV(RGBA16_UINT, DXGI_FORMAT_R16G16B16A16_UINT)
			YAPT_FORMAT_CONV(RGBA16_SINT, DXGI_FORMAT_R16G16B16A16_SINT)
			YAPT_FORMAT_CONV(RGBA16_SFLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT)

			YAPT_FORMAT_CONV(RGBA32_UINT, DXGI_FORMAT_R32G32B32A32_UINT)
			YAPT_FORMAT_CONV(RGBA32_SINT, DXGI_FORMAT_R32G32B32A32_SINT)
			YAPT_FORMAT_CONV(RGBA32_SFLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT)

			YAPT_FORMAT_CONV(RGB32_UINT, DXGI_FORMAT_R32G32B32_UINT)
			YAPT_FORMAT_CONV(RGB32_SINT, DXGI_FORMAT_R32G32B32_SINT)
			YAPT_FORMAT_CONV(RGB32_SFLOAT, DXGI_FORMAT_R32G32B32_FLOAT)

			YAPT_FORMAT_CONV(D16_UNORM, DXGI_FORMAT_D16_UNORM)
			YAPT_FORMAT_CONV(D32_SFLOAT, DXGI_FORMAT_D32_FLOAT)
			YAPT_FORMAT_CONV(D24_UNORM_S8_UINT, DXGI_FORMAT_D24_UNORM_S8_UINT)

			YAPT_FORMAT_CONV(BC4_UNORM, DXGI_FORMAT_BC4_UNORM)
			YAPT_FORMAT_CONV(BC4_SNORM, DXGI_FORMAT_BC4_SNORM)
			YAPT_FORMAT_CONV(BC6H_UFLOAT, DXGI_FORMAT_BC6H_SF16)
			YAPT_FORMAT_CONV(BC6H_SFLOAT, DXGI_FORMAT_BC6H_UF16)
			YAPT_FORMAT_CONV(BC7_UNORM, DXGI_FORMAT_BC7_UNORM)
			YAPT_FORMAT_CONV(BC7_UNORM_SRGB, DXGI_FORMAT_BC7_UNORM_SRGB)
			YAPT_FORMAT_CONV(UNKNOWN, DXGI_FORMAT_UNKNOWN)
		default:
			YAPT_LOG_FATAL_ERROR("UNABLE TO CONVERT FROM YAPT TO DX12 FORMAT!");
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	inline D3D12_RESOURCE_FLAGS yaptToDx12ResourceFlags(ResourceUsage resourceUsage)
	{
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

		if (resourceUsage & (RESOURCE_USAGE_STORAGE_BUFFER | RESOURCE_USAGE_STORAGE_TEXEL_BUFFER | RESOURCE_USAGE_STORAGE_TEXTURE))
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (resourceUsage & RESOURCE_USAGE_DEPTH_STENCIL_TEXTURE)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}
		if (resourceUsage & RESOURCE_USAGE_RENDER_TARGET_TEXTURE)
		{
			flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		return flags;
	}



	inline D3D12_RESOURCE_STATES yaptUsageToDx12ResourceStates(ResourceUsage resourceUsage, AccessFlags accessFlags, ShaderStages shaderStages)
	{
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON;
		
		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_PRESENTABLE_TEXTURE))
		{
			state |= D3D12_RESOURCE_STATE_PRESENT;
		}

		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_VERTEX_BUFFER | RESOURCE_USAGE_UNIFORM_BUFFER))
		{
			state |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
		}
		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_COPY_DESTINATION))
		{
			state |= D3D12_RESOURCE_STATE_COPY_DEST;
		}
		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_COPY_SOURCE))
		{
			state |= D3D12_RESOURCE_STATE_COPY_SOURCE;
		}
		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_INDEX_BUFFER))
		{
			state |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
		}

		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_STORAGE_BUFFER | RESOURCE_USAGE_STORAGE_TEXEL_BUFFER | RESOURCE_USAGE_STORAGE_TEXTURE))
		{
			if (YAPT_AREBITSSET(accessFlags, ACCESS_FLAGS_WRITE))
			{
				state |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}
			else
			{
				if (YAPT_AREBITSSET(shaderStages, SHADERSTAGE_FRAGMENT))
				{
					state |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
				}
				if (YAPT_AREBITSSET(shaderStages, ~SHADERSTAGE_FRAGMENT))
				{
					state |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
				}
			}
			
		}

		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_UNIFORM_TEXEL_BUFFER))
		{
			if (YAPT_AREBITSSET(shaderStages, SHADERSTAGE_FRAGMENT))
			{
				state |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}
			if (YAPT_AREBITSSET(shaderStages, ~SHADERSTAGE_FRAGMENT))
			{
				state |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			}
		}

		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_SHADERTABLE_BUFFER))
		{
			state |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		}
		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_ACCELERATION_STRUCTURE_BUFFER))
		{
			state |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
		}

		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_RENDER_TARGET_TEXTURE))
		{
			state |= D3D12_RESOURCE_STATE_RENDER_TARGET;
		}

		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_DEPTH_STENCIL_TEXTURE))
		{
			if (YAPT_AREBITSSET(accessFlags, ACCESS_FLAGS_WRITE))
			{
				state |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
			}
			else
			{
				state |= D3D12_RESOURCE_STATE_DEPTH_READ;
			}
		}
		if (YAPT_AREBITSSET(resourceUsage, RESOURCE_USAGE_SAMPLED_TEXTURE))
		{
			if (YAPT_AREBITSSET(shaderStages, SHADERSTAGE_FRAGMENT))
			{
				state |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			}
			if (YAPT_AREBITSSET(shaderStages, ~SHADERSTAGE_FRAGMENT))
			{
				state |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
			}
		}

		return state;

	}


	inline D3D12_FILTER yaptFilterToDx12(const SamplerDescription& d)
	{
		if (d.enableAnisotropy)
		{
			return D3D12_FILTER_ANISOTROPIC;
		}

		uint32_t filter = d.mipmapMode == Filter::LINEAR ? 1 : 0;

		filter |= d.minFilter == Filter::LINEAR ? 1 << 4 : 0;
		filter |= d.magFilter == Filter::LINEAR ? 1 << 2 : 0;

		return (D3D12_FILTER)filter;
	}

	inline D3D12_TEXTURE_ADDRESS_MODE yaptAddressModeToDx12(SamplerAddressMode m)
	{
		switch (m)
		{
		case YAPT::SamplerAddressMode::REPEAT:
			return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		case YAPT::SamplerAddressMode::MIRRORED_REPEAT:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
		case YAPT::SamplerAddressMode::CLAMP_TO_EDGE:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		case YAPT::SamplerAddressMode::CLAMP_TO_BORDER:
			return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		case YAPT::SamplerAddressMode::MIRROR_CLAMP_TO_EDGE:
			return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
		default:
			return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		}

		
	}

	inline D3D12_COMPARISON_FUNC yaptComparisonFunctionToDx12(CompareOp c)
	{
		switch (c)
		{
		case YAPT::CompareOp::NEVER:
			return D3D12_COMPARISON_FUNC_NEVER;
		case YAPT::CompareOp::LESS:
			return D3D12_COMPARISON_FUNC_LESS;
		case YAPT::CompareOp::EQUAL:
			return D3D12_COMPARISON_FUNC_EQUAL;
		case YAPT::CompareOp::LESS_OR_EQUAL:
			return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case YAPT::CompareOp::GREATER:
			return D3D12_COMPARISON_FUNC_GREATER;
		case YAPT::CompareOp::NOT_EQUAL:
			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case YAPT::CompareOp::GREATER_OR_EQUAL:
			return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case YAPT::CompareOp::ALWAYS:
			return D3D12_COMPARISON_FUNC_ALWAYS;
		default:
			return D3D12_COMPARISON_FUNC_NEVER;
		}
	}

	inline D3D12_BLEND_OP yaptToDx12BlendOp(BlendOp op)
	{
		switch (op)
		{
		case YAPT::BlendOp::ADD:
			return D3D12_BLEND_OP_ADD;
		case YAPT::BlendOp::SUBTRACT:
			return D3D12_BLEND_OP_SUBTRACT;
		case YAPT::BlendOp::REVERSE_SUBTRACT:
			return D3D12_BLEND_OP_REV_SUBTRACT;
		case YAPT::BlendOp::MIN:
			return D3D12_BLEND_OP_MIN;
		case YAPT::BlendOp::MAX:
			return D3D12_BLEND_OP_MAX;
		default:
			return D3D12_BLEND_OP_ADD;
		}
	}

	inline D3D12_BLEND yaptToDx12BlendFactor(BlendFactor op)
	{
		switch (op)
		{
		case YAPT::BlendFactor::ZERO:
			return D3D12_BLEND_ZERO;
		case YAPT::BlendFactor::ONE:
			return D3D12_BLEND_ONE;
		case YAPT::BlendFactor::SRC_COLOR:
			return D3D12_BLEND_SRC_COLOR;
		case YAPT::BlendFactor::ONE_MINUS_SRC_COLOR:
			return D3D12_BLEND_INV_SRC_COLOR;
		case YAPT::BlendFactor::DST_COLOR:
			return D3D12_BLEND_DEST_COLOR;
		case YAPT::BlendFactor::ONE_MINUS_DST_COLOR:
			return D3D12_BLEND_INV_DEST_COLOR;
		case YAPT::BlendFactor::SRC_ALPHA:
			return D3D12_BLEND_SRC_ALPHA;
		case YAPT::BlendFactor::ONE_MINUS_SRC_ALPHA:
			return D3D12_BLEND_INV_SRC_ALPHA;
		case YAPT::BlendFactor::DST_ALPHA:
			return D3D12_BLEND_DEST_ALPHA;
		case YAPT::BlendFactor::ONE_MINUS_DST_ALPHA:
			return D3D12_BLEND_INV_DEST_ALPHA;
		case YAPT::BlendFactor::CONSTANT_COLOR:
			return D3D12_BLEND_BLEND_FACTOR;
		case YAPT::BlendFactor::ONE_MINUS_CONSTANT_COLOR:
			return D3D12_BLEND_INV_BLEND_FACTOR;
		case YAPT::BlendFactor::CONSTANT_ALPHA:
			return D3D12_BLEND_BLEND_FACTOR;
		case YAPT::BlendFactor::ONE_MINUS_CONSTANT_ALPHA:
			return D3D12_BLEND_INV_BLEND_FACTOR;
		case YAPT::BlendFactor::SRC_ALPHA_SATURATE:
			return D3D12_BLEND_SRC_ALPHA_SAT;
		case YAPT::BlendFactor::SRC1_COLOR:
			return D3D12_BLEND_SRC1_COLOR;
		case YAPT::BlendFactor::ONE_MINUS_SRC1_COLOR:
			return D3D12_BLEND_INV_SRC1_COLOR;
		case YAPT::BlendFactor::SRC1_ALPHA:
			return D3D12_BLEND_SRC1_ALPHA;
		case YAPT::BlendFactor::ONE_MINUS_SRC1_ALPHA:
			return D3D12_BLEND_INV_SRC1_ALPHA;
		default:
			return D3D12_BLEND_ZERO;
		}
	}

	inline D3D12_COMPARISON_FUNC yaptToDx12ComparisonOp(CompareOp op)
	{
		switch (op)
		{
		case YAPT::CompareOp::NEVER:
			return D3D12_COMPARISON_FUNC_NEVER;
		case YAPT::CompareOp::LESS:
			return D3D12_COMPARISON_FUNC_LESS;
		case YAPT::CompareOp::EQUAL:
			return D3D12_COMPARISON_FUNC_EQUAL;
		case YAPT::CompareOp::LESS_OR_EQUAL:
			return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case YAPT::CompareOp::GREATER:
			return D3D12_COMPARISON_FUNC_GREATER;
		case YAPT::CompareOp::NOT_EQUAL:
			return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case YAPT::CompareOp::GREATER_OR_EQUAL:
			return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case YAPT::CompareOp::ALWAYS:
			return D3D12_COMPARISON_FUNC_ALWAYS;
		default:
			return D3D12_COMPARISON_FUNC_NEVER;
		}
	}

	inline D3D12_STENCIL_OP yaptToDx12StencilOp(StencilOp op)
	{
		switch (op)
		{
		case YAPT::StencilOp::KEEP:
			return D3D12_STENCIL_OP_KEEP;
		case YAPT::StencilOp::ZERO:
			return D3D12_STENCIL_OP_ZERO;
		case YAPT::StencilOp::REPLACE:
			return D3D12_STENCIL_OP_REPLACE;
		case YAPT::StencilOp::INCREMENT_AND_CLAMP:
			return D3D12_STENCIL_OP_INCR_SAT;
		case YAPT::StencilOp::DECREMENT_AND_CLAMP:
			return D3D12_STENCIL_OP_DECR_SAT;
		case YAPT::StencilOp::INVERT:
			return D3D12_STENCIL_OP_INVERT;
		case YAPT::StencilOp::INCREMENT_AND_WRAP:
			return D3D12_STENCIL_OP_INCR;
		case YAPT::StencilOp::DECREMENT_AND_WRAP:
			return D3D12_STENCIL_OP_DECR;
		default:
			return D3D12_STENCIL_OP_KEEP;
		}
	}

	inline D3D12_LOGIC_OP yaptToDx12LogicOp(LogicOp op)
	{
		switch (op)
		{
		case YAPT::LogicOp::CLEAR:
			return D3D12_LOGIC_OP_CLEAR;
		case YAPT::LogicOp::AND:
			return D3D12_LOGIC_OP_AND;
		case YAPT::LogicOp::AND_REVERSE:
			return D3D12_LOGIC_OP_AND_REVERSE;
		case YAPT::LogicOp::COPY:
			return D3D12_LOGIC_OP_COPY;
		case YAPT::LogicOp::AND_INVERTED:
			return D3D12_LOGIC_OP_AND_INVERTED;
		case YAPT::LogicOp::NO_OP:
			return D3D12_LOGIC_OP_NOOP;
		case YAPT::LogicOp::XOR:
			return D3D12_LOGIC_OP_XOR;
		case YAPT::LogicOp::OR:
			return D3D12_LOGIC_OP_OR;
		case YAPT::LogicOp::NOR:
			return D3D12_LOGIC_OP_NOR;
		case YAPT::LogicOp::EQUIVALENT:
			return D3D12_LOGIC_OP_EQUIV;
		case YAPT::LogicOp::INVERT:
			return D3D12_LOGIC_OP_INVERT;
		case YAPT::LogicOp::OR_REVERSE:
			return D3D12_LOGIC_OP_OR_REVERSE;
		case YAPT::LogicOp::COPY_INVERTED:
			return D3D12_LOGIC_OP_COPY_INVERTED;
		case YAPT::LogicOp::OR_INVERTED:
			return D3D12_LOGIC_OP_OR_INVERTED;
		case YAPT::LogicOp::NAND:
			return D3D12_LOGIC_OP_NAND;
		case YAPT::LogicOp::SET:
			return D3D12_LOGIC_OP_SET;
		default:
			return D3D12_LOGIC_OP_NOOP;
		}
	}

	inline D3D12_CULL_MODE yaptToDx12CullMode(CullMode cull)
	{
		switch (cull)
		{
		case YAPT::CullMode::NONE:
			return D3D12_CULL_MODE_NONE;
		case YAPT::CullMode::FRONT:
			return D3D12_CULL_MODE_FRONT;
		case YAPT::CullMode::BACK:
			return D3D12_CULL_MODE_BACK;
		default:
			return D3D12_CULL_MODE_NONE;
		}
	}

	inline D3D12_STATIC_BORDER_COLOR dx12BorderColorToStaticBorderColor(const FLOAT borderColor[4])
	{
		if (borderColor[0] == 0.f && borderColor[1] == 0.f && borderColor[2] == 0.f && borderColor[3] == 0.f)
		{
			return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		}

		if (borderColor[0] == 1.f && borderColor[1] == 1.f && borderColor[2] == 1.f && borderColor[3] == 1.f)
		{
			return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		}

		return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	}

	inline void yaptBorderColorToDx12(BorderColor bc, float dst[4])
	{
		switch (bc)
		{
		case YAPT::BorderColor::FLOAT_TRANSPARENT_BLACK:
			dst[0] = dst[1] = dst[2] = dst[3] = 0;
			break;
		case YAPT::BorderColor::INT_TRANSPARENT_BLACK:
			dst[0] = dst[1] = dst[2] = dst[3] = 0;
			break;
		case YAPT::BorderColor::FLOAT_OPAQUE_BLACK:
			dst[0] = dst[1] = dst[2] = 0;
			dst[3] = 1;
			break;
		case YAPT::BorderColor::INT_OPAQUE_BLACK:
			dst[0] = dst[1] = dst[2] = 0;
			dst[3] = 1;
			break;
		case YAPT::BorderColor::FLOAT_OPAQUE_WHITE:
			dst[0] = dst[1] = dst[2] = 1;
			dst[3] = 1;
			break;
		case YAPT::BorderColor::INT_OPAQUE_WHITE:
			dst[0] = dst[1] = dst[2] = 1;
			dst[3] = 1;
			break;
		default:
			dst[0] = dst[1] = dst[2] = dst[3] = 0;
		}
	}

	inline void yaptSamplerDescriptionToDx12(const SamplerDescription& src, D3D12_SAMPLER_DESC& dst)
	{
		dst.AddressU = yaptAddressModeToDx12(src.addressModeU);
		dst.AddressV = yaptAddressModeToDx12(src.addressModeV);
		dst.AddressW = yaptAddressModeToDx12(src.addressModeW);
		dst.ComparisonFunc = yaptComparisonFunctionToDx12(src.compareOp);
		dst.Filter = yaptFilterToDx12(src);
		dst.MipLODBias = src.mipLodBias;
		dst.MinLOD = src.minLod;
		dst.MaxLOD = src.maxLod;
		dst.MaxAnisotropy = src.enableAnisotropy ? (UINT)src.maxAnisotropy : 1;
		yaptBorderColorToDx12(src.borderColor, dst.BorderColor);
		
	}

	inline void textureDescToDx12ResourceDesc(const TextureDesc& textureDesc, D3D12_RESOURCE_DESC& resourceDesc)
	{
		resourceDesc.Dimension = yaptToDx12ResourceDimensions(textureDesc.dimension);
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = (UINT)textureDesc.width;
		resourceDesc.Height = (UINT)textureDesc.height;
		resourceDesc.DepthOrArraySize = (UINT)textureDesc.depthOrSlices;
		resourceDesc.MipLevels = (UINT16)textureDesc.mips;
		resourceDesc.Format = yaptToDx12Format(textureDesc.format);
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = yaptToDx12ResourceFlags(textureDesc.resourceUsage);

	}


	inline void bufferDescToDx12ResourceDesc(const BufferDesc& bufferDesc, D3D12_RESOURCE_DESC& resourceDesc)
	{
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = bufferDesc.sizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = yaptToDx12ResourceFlags(bufferDesc.resourceUsage);
	}

	inline void fillGraphicsPipelineStateShaderStages(const ShaderStageCreateInfo* shaderStages, uint32_t numberOfShaderStages, D3D12_GRAPHICS_PIPELINE_STATE_DESC& descOut)
	{
		descOut.VS.BytecodeLength = 0;
		descOut.VS.pShaderBytecode = nullptr;

		descOut.HS.BytecodeLength = 0;
		descOut.HS.pShaderBytecode = nullptr;

		descOut.DS.BytecodeLength = 0;
		descOut.DS.pShaderBytecode = nullptr;

		descOut.GS.BytecodeLength = 0;
		descOut.GS.pShaderBytecode = nullptr;

		descOut.PS.BytecodeLength = 0;
		descOut.PS.pShaderBytecode = nullptr;

		for (size_t i = 0; i < numberOfShaderStages; ++i)
		{
			const ShaderStageCreateInfo& info = shaderStages[i];
			IDxcBlob* blob = info.shaderModule->shaderBlob;
		
			switch (info.stage)
			{
			
			case YAPT::SHADERSTAGE_VERTEX:
				descOut.VS.pShaderBytecode = blob->GetBufferPointer();
				descOut.VS.BytecodeLength = blob->GetBufferSize();
				break;
			case YAPT::SHADERSTAGE_FRAGMENT:
				descOut.PS.pShaderBytecode = blob->GetBufferPointer();
				descOut.PS.BytecodeLength = blob->GetBufferSize();
				break;
			case YAPT::SHADERSTAGE_HULL:
				descOut.HS.pShaderBytecode = blob->GetBufferPointer();
				descOut.HS.BytecodeLength = blob->GetBufferSize();
				break;
			case YAPT::SHADERSTAGE_DOMAIN:
				descOut.DS.pShaderBytecode = blob->GetBufferPointer();
				descOut.DS.BytecodeLength = blob->GetBufferSize();
				break;
			case YAPT::SHADERSTAGE_GEOMETRY:
				descOut.GS.pShaderBytecode = blob->GetBufferPointer();
				descOut.GS.BytecodeLength = blob->GetBufferSize();
				break;
			
			default:
				YAPT_LOG_ERROR("Tried to use invalid shaderstage in graphics pso");
			}
		}
	}
	inline void fillStreamOutputDesc(D3D12_STREAM_OUTPUT_DESC& out)
	{
		out.pSODeclaration = nullptr;
		out.pBufferStrides = nullptr;
		out.NumEntries = 0;
		out.NumStrides = 0;
		out.RasterizedStream = 0;
	}

	inline void fillRTBlendDesc(const BlendTargetDescription& desc, bool enableLogicOp, LogicOp logicOp, D3D12_RENDER_TARGET_BLEND_DESC& out)
	{
		out.BlendEnable = desc.blendEnable;
		out.LogicOpEnable = enableLogicOp;
		out.BlendOp = yaptToDx12BlendOp(desc.colorBlendOp);
		out.BlendOpAlpha = yaptToDx12BlendOp(desc.alphaBlendOp);

		out.SrcBlend = yaptToDx12BlendFactor(desc.srcColor);
		out.SrcBlendAlpha = yaptToDx12BlendFactor(desc.srcAlpha);
		out.DestBlend = yaptToDx12BlendFactor(desc.dstColor);
		out.DestBlendAlpha = yaptToDx12BlendFactor(desc.dstAlpha);

		out.LogicOp = yaptToDx12LogicOp(logicOp);

		out.RenderTargetWriteMask = desc.colorWriteMask;
	}

	inline void fillBlendDesc(const BlendStateDescription& blendDesc, bool alphaToCoverageEnable, D3D12_BLEND_DESC& out)
	{
		for (size_t i = 0; i < blendDesc.numberOfBlendTargets; ++i)
		{
			fillRTBlendDesc(blendDesc.blendTargetDescriptions[i], blendDesc.enableLogicalOp, blendDesc.logicalOp, out.RenderTarget[i]);
		}

		out.IndependentBlendEnable = blendDesc.enableIndependentBlend;
		out.AlphaToCoverageEnable = alphaToCoverageEnable;

	}


	inline void fillRasterizerDesc(const RasterizerStateDescription& desc, bool enableMultiSample, D3D12_RASTERIZER_DESC& out)
	{
		constexpr uint32_t DEPTH_BIAS_MULTIPLIER =  1 << 24;  //in reality should take into account the depth format. For now just do this.


		out.FillMode = desc.polygonMode == PolygonMode::FILL ? D3D12_FILL_MODE_SOLID : D3D12_FILL_MODE_WIREFRAME;
		out.CullMode = yaptToDx12CullMode(desc.cullMode);

		out.FrontCounterClockwise = desc.frontFace == FrontFace::COUNTER_CLOCKWISE;
		out.DepthBias = desc.depthBiasEnable ? INT(desc.depthBiasConstantFactor * DEPTH_BIAS_MULTIPLIER) : 0;
		out.DepthBiasClamp = desc.depthBiasEnable ? desc.depthBiasClamp : 0;
		out.SlopeScaledDepthBias = desc.depthBiasEnable ? desc.depthBiasSlopeFactor : 0;
		out.DepthClipEnable = desc.depthClampEnable;
		out.MultisampleEnable = enableMultiSample;
		out.AntialiasedLineEnable = false;
		out.ForcedSampleCount = 0;
		out.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	}

	inline void fillDepthStencilDesc(const DepthStencilStateDescription& desc, D3D12_DEPTH_STENCIL_DESC& out)
	{
		out.DepthEnable = desc.depthTestEnable;
		out.DepthWriteMask = desc.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
		out.DepthFunc = yaptToDx12ComparisonOp(desc.depthCompareOp);

		out.StencilEnable = desc.stencilTestEnable;
		out.StencilWriteMask = desc.writeMask;
		out.StencilReadMask = desc.compareMask;

		out.FrontFace.StencilFunc = yaptToDx12ComparisonOp(desc.stencilFront.compareOp);
		out.FrontFace.StencilPassOp = yaptToDx12StencilOp(desc.stencilFront.passOp);
		out.FrontFace.StencilFailOp = yaptToDx12StencilOp(desc.stencilFront.failOp);
		out.FrontFace.StencilDepthFailOp = yaptToDx12StencilOp(desc.stencilFront.depthFailOp);

		out.BackFace.StencilFunc = yaptToDx12ComparisonOp(desc.stencilBack.compareOp);
		out.BackFace.StencilPassOp = yaptToDx12StencilOp(desc.stencilBack.passOp);
		out.BackFace.StencilFailOp = yaptToDx12StencilOp(desc.stencilBack.failOp);
		out.BackFace.StencilDepthFailOp = yaptToDx12StencilOp(desc.stencilBack.depthFailOp);


	}

	inline void fillDepthStencilViewDesc(ResourceDimension resourceDimensions, ResourceFormat resourceFormat, AccessFlags accessFlags, UINT mipOffset,
		UINT arraySliceOffset, UINT arraySliceCount, bool isMultisample, D3D12_DEPTH_STENCIL_VIEW_DESC& descOut)
	{
		descOut.Format = yaptToDx12Format(resourceFormat);
		descOut.Flags = (accessFlags & ACCESS_FLAGS_WRITE) == 0 ? D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL : D3D12_DSV_FLAG_NONE;

		switch (resourceDimensions)
		{
		
		case YAPT::ResourceDimension::TEXTURE_1D:
			descOut.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
			descOut.Texture1D.MipSlice = mipOffset;
			break;
		case YAPT::ResourceDimension::TEXTURE_1D_ARRAY:
			descOut.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
			descOut.Texture1DArray.ArraySize = arraySliceCount;
			descOut.Texture1DArray.FirstArraySlice = arraySliceOffset;
			descOut.Texture1DArray.MipSlice = mipOffset;
			break;
		case YAPT::ResourceDimension::TEXTURE_2D:
			descOut.ViewDimension = isMultisample ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
			if (!isMultisample)
			{
				descOut.Texture2D.MipSlice = mipOffset;
			}
			break;
		case YAPT::ResourceDimension::TEXTURE_2D_ARRAY:
			descOut.ViewDimension = isMultisample ? D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY : D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			if (isMultisample)
			{
				descOut.Texture2DMSArray.ArraySize = arraySliceCount;
				descOut.Texture2DMSArray.FirstArraySlice = arraySliceOffset;
			}
			else
			{
				descOut.Texture2DArray.ArraySize = arraySliceCount;
				descOut.Texture2DArray.FirstArraySlice = arraySliceOffset;
				descOut.Texture2DArray.MipSlice = mipOffset;
			}

			break;
		default:
			assert(!"unknown/uncompatible resource dimension");
		}
	}

	

	inline void fillRenderTargetViewDesc(ResourceDimension resourceDimensions, ResourceFormat resourceFormat, AccessFlags accessFlags, UINT mipOffset, UINT arraySliceOrBufferElementOffset,
		UINT arraySliceOrBufferElementCount, bool isMultisample, D3D12_RENDER_TARGET_VIEW_DESC& descOut)
	{
		descOut.Format = yaptToDx12Format(resourceFormat);

		switch (resourceDimensions)
		{

		case YAPT::ResourceDimension::TEXTURE_1D:
			descOut.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
			descOut.Texture1D.MipSlice = mipOffset;
			break;
		case YAPT::ResourceDimension::TEXTURE_1D_ARRAY:
			descOut.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
			descOut.Texture1DArray.ArraySize = arraySliceOrBufferElementCount;
			descOut.Texture1DArray.FirstArraySlice = arraySliceOrBufferElementOffset;
			descOut.Texture1DArray.MipSlice = mipOffset;
			break;
		case YAPT::ResourceDimension::TEXTURE_2D:
			descOut.ViewDimension = isMultisample ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
			if (!isMultisample)
			{
				descOut.Texture2D.MipSlice = mipOffset;
				descOut.Texture2D.PlaneSlice = 0;
			}
			break;
		case YAPT::ResourceDimension::TEXTURE_2D_ARRAY:
			descOut.ViewDimension = isMultisample ? D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY : D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			if (isMultisample)
			{
				descOut.Texture2DMSArray.ArraySize = arraySliceOrBufferElementCount;
				descOut.Texture2DMSArray.FirstArraySlice = arraySliceOrBufferElementOffset;
			}
			else
			{
				descOut.Texture2DArray.ArraySize = arraySliceOrBufferElementCount;
				descOut.Texture2DArray.FirstArraySlice = arraySliceOrBufferElementOffset;
				descOut.Texture2DArray.MipSlice = mipOffset;
				descOut.Texture2DArray.PlaneSlice = 0;
			}

			break;

		case YAPT::ResourceDimension::BUFFER:
			descOut.ViewDimension = D3D12_RTV_DIMENSION_BUFFER;
			descOut.Buffer.FirstElement = arraySliceOrBufferElementOffset;
			descOut.Buffer.NumElements = arraySliceOrBufferElementCount;
			break;
		case YAPT::ResourceDimension::TEXTURE_3D:
			descOut.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			descOut.Texture3D.MipSlice = mipOffset;
			descOut.Texture3D.FirstWSlice = arraySliceOrBufferElementOffset;
			descOut.Texture3D.WSize = arraySliceOrBufferElementCount;
			break;
		default:
			assert(!"unknown/uncompatible resource dimension");
		}
	}

	inline void fillShaderResourceView(const TextureViewDesc& texViewDesc, const DescriptorSetLayoutBinding& bindingDef, D3D12_SHADER_RESOURCE_VIEW_DESC& srvOut)
	{
		srvOut.Format = yaptToDx12Format(texViewDesc.format);
		srvOut.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		switch (texViewDesc.dimensions)
		{
		case YAPT::ResourceDimension::TEXTURE_1D:
			srvOut.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
			srvOut.Texture1D.MipLevels = (UINT)texViewDesc.mipCount;
			srvOut.Texture1D.MostDetailedMip = (UINT)texViewDesc.mipOffset;
			srvOut.Texture1D.ResourceMinLODClamp = 0.f;
			break;
		case YAPT::ResourceDimension::TEXTURE_1D_ARRAY:
			srvOut.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
			srvOut.Texture1DArray.MipLevels = (UINT)texViewDesc.mipCount;
			srvOut.Texture1DArray.MostDetailedMip = (UINT)texViewDesc.mipOffset;
			srvOut.Texture1DArray.FirstArraySlice = (UINT)texViewDesc.arraySliceOffset;
			srvOut.Texture1DArray.ArraySize = (UINT)texViewDesc.arraySliceCount;
			srvOut.Texture1DArray.ResourceMinLODClamp = 0.f;
			break;
		case YAPT::ResourceDimension::TEXTURE_2D:
			srvOut.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvOut.Texture2D.MipLevels = (UINT)texViewDesc.mipCount;
			srvOut.Texture2D.MostDetailedMip = (UINT)texViewDesc.mipOffset;
			srvOut.Texture2D.PlaneSlice = 0;
			srvOut.Texture2D.ResourceMinLODClamp = 0.f;
			break;
		case YAPT::ResourceDimension::TEXTURE_2D_ARRAY:
			srvOut.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
			srvOut.Texture2DArray.MipLevels = (UINT)texViewDesc.mipCount;
			srvOut.Texture2DArray.MostDetailedMip = (UINT)texViewDesc.mipOffset;
			srvOut.Texture2DArray.FirstArraySlice = (UINT)texViewDesc.arraySliceOffset;
			srvOut.Texture2DArray.ArraySize = (UINT)texViewDesc.arraySliceCount;
			srvOut.Texture2DArray.PlaneSlice = 0;
			srvOut.Texture2DArray.ResourceMinLODClamp = 0.f;
			break;
		case YAPT::ResourceDimension::TEXTURE_3D:
			srvOut.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvOut.Texture3D.MipLevels = (UINT)texViewDesc.mipCount;
			srvOut.Texture3D.MostDetailedMip = (UINT)texViewDesc.mipOffset;
			srvOut.Texture3D.ResourceMinLODClamp = 0.f;
			break;
		case YAPT::ResourceDimension::TEXTURE_CUBEMAP:
			srvOut.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
			srvOut.TextureCube.MipLevels = (UINT)texViewDesc.mipCount;
			srvOut.TextureCube.MostDetailedMip = (UINT)texViewDesc.mipOffset;
			srvOut.TextureCube.ResourceMinLODClamp = 0.f;
			break;
		case YAPT::ResourceDimension::TEXTURE_CUBEMAP_ARRAY:
			srvOut.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
			srvOut.TextureCubeArray.MipLevels = (UINT)texViewDesc.mipCount;
			srvOut.TextureCubeArray.MostDetailedMip = (UINT)texViewDesc.mipOffset;
			srvOut.TextureCubeArray.First2DArrayFace = (UINT)texViewDesc.arraySliceOffset;
			srvOut.TextureCubeArray.NumCubes = (UINT)texViewDesc.arraySliceCount;
			srvOut.TextureCubeArray.ResourceMinLODClamp = 0.f;
			break;
		case YAPT::ResourceDimension::UNDEFINED:
		case YAPT::ResourceDimension::BUFFER:
		default:
			YAPT_LOG_ERROR("can't fill D3D12_SHADER_RESOURCE_VIEW_DESC from TextureViewDesc, incompatible dimension");
			break;
		}
	}

	inline void fillShaderResourceView(const BufferViewDesc& bufViewDesc, const DescriptorSetLayoutBinding& bindingDef, ID3D12Resource* res, D3D12_SHADER_RESOURCE_VIEW_DESC& srvOut)
	{
		
		srvOut.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		if (bindingDef.type == DescriptorType::ACCELERATION_STRUCTURE)
		{
			srvOut.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
			srvOut.Format = DXGI_FORMAT_UNKNOWN;
			srvOut.RaytracingAccelerationStructure.Location = res->GetGPUVirtualAddress() + bufViewDesc.offsetInBytes;
			
		}
		else
		{
			bool isRawView = (bufViewDesc.flags & BufferViewFlagBits::BUFFERVIEWFLAGS_RAW) != 0;

			D3D12_BUFFER_SRV_FLAGS flags = D3D12_BUFFER_SRV_FLAG_NONE;
			UINT sizeInBytes;
			srvOut.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			if (isRawView)
			{
				sizeInBytes = 4;
				srvOut.Format = DXGI_FORMAT_R32_TYPELESS;
				flags = D3D12_BUFFER_SRV_FLAG_RAW;
			}
			else if (bufViewDesc.structureStrideInBytes != 0)
			{
				sizeInBytes = (UINT)bufViewDesc.structureStrideInBytes;
				srvOut.Format = DXGI_FORMAT_UNKNOWN;
			}
			else if(bufViewDesc.nonStructuredFormat != ResourceFormat::UNKNOWN)
			{
				sizeInBytes = (UINT)getFormatSizeInBytes(bufViewDesc.nonStructuredFormat);
				srvOut.Format = yaptToDx12Format(bufViewDesc.nonStructuredFormat);
			} 
			 
			else
			{
				assert(!"not sure what kind of view to create");
			}
			
			srvOut.Buffer.Flags = flags;
			srvOut.Buffer.FirstElement = (UINT64)bufViewDesc.offsetInBytes / sizeInBytes;
			srvOut.Buffer.NumElements = (UINT)bufViewDesc.sizeInBytes / sizeInBytes;
			srvOut.Buffer.StructureByteStride = isRawView ? 0 : (UINT)bufViewDesc.structureStrideInBytes;

			assert((bufViewDesc.offsetInBytes % sizeInBytes) == 0);
			assert((bufViewDesc.sizeInBytes % sizeInBytes) == 0);
		}
		
	}

	inline void fillUnorderedAccessViewForClearing(const RenderGraphResourceUsage& def, size_t bufferSize, D3D12_UNORDERED_ACCESS_VIEW_DESC& uavOut)
	{
		uavOut.Format = yaptToDx12Format(def.resourceDescription.resourceFormat);
		switch (def.resourceDescription.resourceDimensions)
		{
		case YAPT::ResourceDimension::BUFFER:
			uavOut.Format = DXGI_FORMAT_UNKNOWN;
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
			uavOut.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			uavOut.Buffer.CounterOffsetInBytes = 0;
			uavOut.Buffer.FirstElement = 0;
			uavOut.Buffer.StructureByteStride = 4;
			uavOut.Buffer.NumElements = static_cast<UINT>(bufferSize/(size_t)uavOut.Buffer.StructureByteStride);
			break;
		case YAPT::ResourceDimension::TEXTURE_1D:
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavOut.Texture1D.MipSlice = (UINT)def.mipOffset;
			break;
		case YAPT::ResourceDimension::TEXTURE_1D_ARRAY:
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
			uavOut.Texture1DArray.MipSlice = (UINT)def.mipOffset;
			uavOut.Texture1DArray.FirstArraySlice = (UINT)def.arraySliceOffset;
			uavOut.Texture1DArray.ArraySize = (UINT)def.resourceDescription.arraySliceCount;
			break;
		case YAPT::ResourceDimension::TEXTURE_2D:
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavOut.Texture2D.MipSlice = (UINT)def.mipOffset;
			uavOut.Texture2D.PlaneSlice = 0;
			break;
		case YAPT::ResourceDimension::TEXTURE_2D_ARRAY:
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavOut.Texture2DArray.MipSlice = (UINT)def.mipOffset;
			uavOut.Texture2DArray.FirstArraySlice = (UINT)def.arraySliceOffset;
			uavOut.Texture2DArray.ArraySize = (UINT)def.resourceDescription.arraySliceCount;
			uavOut.Texture2DArray.PlaneSlice = 0;

			break;
		case YAPT::ResourceDimension::TEXTURE_3D:
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavOut.Texture3D.MipSlice = (UINT)def.mipOffset;
			uavOut.Texture3D.FirstWSlice = (UINT)def.arraySliceOffset;
			uavOut.Texture3D.WSize = (UINT)def.resourceDescription.arraySliceCount;
			break;

		default:
			YAPT_LOG_ERROR("can't fill D3D12_UNORDERED_ACCESS_VIEW_DESC from TextureViewDesc, incompatible dimension");
			break;
		}
	}
	

	inline void fillUnorderedAccessView(const TextureViewDesc& texViewDesc, const DescriptorSetLayoutBinding& bindingDef, D3D12_UNORDERED_ACCESS_VIEW_DESC& uavOut)
	{
		uavOut.Format = yaptToDx12Format(texViewDesc.format);
		switch (texViewDesc.dimensions)
		{
		case YAPT::ResourceDimension::TEXTURE_1D:
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
			uavOut.Texture1D.MipSlice = (UINT)texViewDesc.mipOffset;
			break;
		case YAPT::ResourceDimension::TEXTURE_1D_ARRAY:
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
			uavOut.Texture1DArray.MipSlice = (UINT)texViewDesc.mipOffset;
			uavOut.Texture1DArray.FirstArraySlice = (UINT)texViewDesc.arraySliceOffset;
			uavOut.Texture1DArray.ArraySize = (UINT)texViewDesc.arraySliceCount;
			break;
		case YAPT::ResourceDimension::TEXTURE_2D:
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavOut.Texture2D.MipSlice = (UINT)texViewDesc.mipOffset;
			uavOut.Texture2D.PlaneSlice = 0;
			break;
		case YAPT::ResourceDimension::TEXTURE_2D_ARRAY:
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
			uavOut.Texture2DArray.MipSlice = (UINT)texViewDesc.mipOffset;
			uavOut.Texture2DArray.FirstArraySlice = (UINT)texViewDesc.arraySliceOffset;
			uavOut.Texture2DArray.ArraySize = (UINT)texViewDesc.arraySliceCount;
			uavOut.Texture2DArray.PlaneSlice = 0;

			break;
		case YAPT::ResourceDimension::TEXTURE_3D:
			uavOut.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavOut.Texture3D.MipSlice = (UINT)texViewDesc.mipOffset;
			uavOut.Texture3D.FirstWSlice = (UINT)texViewDesc.arraySliceOffset;
			uavOut.Texture3D.WSize = (UINT)texViewDesc.arraySliceCount;
			break;
		
		default:
			YAPT_LOG_ERROR("can't fill D3D12_UNORDERED_ACCESS_VIEW_DESC from TextureViewDesc, incompatible dimension");
			break;
		}
	}

	

	inline void fillUnorderedAccessView(const BufferViewDesc& bufViewDesc, const DescriptorSetLayoutBinding& bindingDef, D3D12_UNORDERED_ACCESS_VIEW_DESC& uavOut)
	{
		uavOut.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavOut.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		uavOut.Buffer.CounterOffsetInBytes = 0;

		UINT sizeInBytes;
		bool isRawView = (bufViewDesc.flags & BufferViewFlagBits::BUFFERVIEWFLAGS_RAW) != 0;

		if (isRawView)
		{
			sizeInBytes = 4;
			uavOut.Format = DXGI_FORMAT_R32_TYPELESS;
			uavOut.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		}
		else if (bufViewDesc.structureStrideInBytes != 0)
		{

			sizeInBytes = (UINT)bufViewDesc.structureStrideInBytes;
			uavOut.Format = DXGI_FORMAT_UNKNOWN;
		}
		else if (bufViewDesc.nonStructuredFormat != ResourceFormat::UNKNOWN)
		{

			sizeInBytes = (UINT)getFormatSizeInBytes(bufViewDesc.nonStructuredFormat);
			uavOut.Format = yaptToDx12Format(bufViewDesc.nonStructuredFormat);
		}
		else
		{
			assert(!"not sure what kind of view to create");
		}

		uavOut.Buffer.FirstElement = (UINT64)bufViewDesc.offsetInBytes / sizeInBytes;
		uavOut.Buffer.NumElements = (UINT)bufViewDesc.sizeInBytes / sizeInBytes;
		uavOut.Buffer.StructureByteStride = isRawView ? 0 : (UINT)bufViewDesc.structureStrideInBytes;

		//assert(bufViewDesc.offsetInBytes % sizeInBytes);
		//assert(bufViewDesc.sizeInBytes % sizeInBytes);
	}

	inline void fillConstantBufferView(const BufferViewDesc& bufViewDesc, const DescriptorSetLayoutBinding& bindingDef, ID3D12Resource* res, D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvOut)
	{
		cbvOut.BufferLocation = res->GetGPUVirtualAddress() + bufViewDesc.offsetInBytes;
		cbvOut.SizeInBytes = (UINT)align(bufViewDesc.sizeInBytes, getMinimumAlignmentForBufferUsage(RESOURCE_USAGE_UNIFORM_BUFFER));

	}

}

#endif