#ifndef YAPT_RENDERERCOMMONTYPESUTILITY_H
#define YAPT_RENDERERCOMMONTYPESUTILITY_H

#include "RendererCommonTypes.h"

namespace YAPT
{
	inline ResourceDimension getArrayType(ResourceDimension a)
	{
		switch (a)
		{
		case YAPT::ResourceDimension::TEXTURE_1D:
			return YAPT::ResourceDimension::TEXTURE_1D_ARRAY;
		case YAPT::ResourceDimension::TEXTURE_2D:
			return YAPT::ResourceDimension::TEXTURE_2D_ARRAY;
		case YAPT::ResourceDimension::TEXTURE_CUBEMAP:
			return YAPT::ResourceDimension::TEXTURE_CUBEMAP_ARRAY;
		default:
			return YAPT::ResourceDimension::UNDEFINED;
		}
	}

	inline bool isArrayDimension(ResourceDimension dim)
	{
		switch (dim)
		{
		case YAPT::ResourceDimension::TEXTURE_2D_ARRAY:
		case YAPT::ResourceDimension::TEXTURE_1D_ARRAY:
		case YAPT::ResourceDimension::TEXTURE_CUBEMAP_ARRAY:
			return true;
		default:
			return false;
		}
	}

	inline uint32_t getNumberOfDimensions(ResourceDimension dim)
	{
		switch (dim)
		{
		case YAPT::ResourceDimension::BUFFER:
			return 0;
		case YAPT::ResourceDimension::TEXTURE_1D:
			return 1;
		case YAPT::ResourceDimension::TEXTURE_1D_ARRAY:
			return 1;
		case YAPT::ResourceDimension::TEXTURE_2D:
			return 2;
		case YAPT::ResourceDimension::TEXTURE_2D_ARRAY:
			return 2;
		case YAPT::ResourceDimension::TEXTURE_3D:
			return 3;
		case YAPT::ResourceDimension::TEXTURE_CUBEMAP:
			return 3;
		case YAPT::ResourceDimension::TEXTURE_CUBEMAP_ARRAY:
			return 3;
		case YAPT::ResourceDimension::UNDEFINED:
		default:
			return 0;
		}
	}

	/**
		Returns the size of uncompressed format in bytes.
	*/
	inline uint32_t getFormatSizeInBytes(ResourceFormat format)
	{
		switch (format)
		{
		case YAPT::ResourceFormat::R8_UNORM:
		case YAPT::ResourceFormat::R8_SNORM:
		case YAPT::ResourceFormat::R8_UINT:
		case YAPT::ResourceFormat::R8_SINT:
		case YAPT::ResourceFormat::R8_SRGB:
			return 1;

		case YAPT::ResourceFormat::RG8_UNORM:
		case YAPT::ResourceFormat::RG8_SNORM:
		case YAPT::ResourceFormat::RG8_UINT:
		case YAPT::ResourceFormat::RG8_SINT:
		case YAPT::ResourceFormat::RG8_SRGB:

		case YAPT::ResourceFormat::R16_UNORM:
		case YAPT::ResourceFormat::R16_SNORM:
		case YAPT::ResourceFormat::R16_UINT:
		case YAPT::ResourceFormat::R16_SINT:
		case YAPT::ResourceFormat::R16_SFLOAT:
			return 2;

		case YAPT::ResourceFormat::RGBA8_UNORM:
		case YAPT::ResourceFormat::RGBA8_SNORM:
		case YAPT::ResourceFormat::RGBA8_UINT:
		case YAPT::ResourceFormat::RGBA8_SINT:
		case YAPT::ResourceFormat::RGBA8_SRGB:

		case YAPT::ResourceFormat::RG16_UNORM:
		case YAPT::ResourceFormat::RG16_SNORM:
		case YAPT::ResourceFormat::RG16_UINT:
		case YAPT::ResourceFormat::RG16_SINT:
		case YAPT::ResourceFormat::RG16_SFLOAT:

		case YAPT::ResourceFormat::R32_UINT:
		case YAPT::ResourceFormat::R32_SINT:
		case YAPT::ResourceFormat::R32_SFLOAT:
			return 4;

		case YAPT::ResourceFormat::RGBA16_UNORM:
		case YAPT::ResourceFormat::RGBA16_SNORM:
		case YAPT::ResourceFormat::RGBA16_UINT:
		case YAPT::ResourceFormat::RGBA16_SINT:
		case YAPT::ResourceFormat::RGBA16_SFLOAT:

		case YAPT::ResourceFormat::RG32_UINT:
		case YAPT::ResourceFormat::RG32_SINT:
		case YAPT::ResourceFormat::RG32_SFLOAT:
			return 8;

		case YAPT::ResourceFormat::RGB32_UINT:
		case YAPT::ResourceFormat::RGB32_SINT:
		case YAPT::ResourceFormat::RGB32_SFLOAT:
			return 12;

		case YAPT::ResourceFormat::RGBA32_UINT:
		case YAPT::ResourceFormat::RGBA32_SINT:
		case YAPT::ResourceFormat::RGBA32_SFLOAT:
			return 16;

		default:
			return 0;
		}
	}

	inline AttributeSemanticName getAttributeSemanticNameFromString(const char* str)
	{
		if (strcmp(str, "POSITION") == 0)
		{
			return AttributeSemanticName::POSITION;
		}
		else if (strcmp(str, "TEXCOORD") == 0)
		{
			return AttributeSemanticName::TEXCOORD;
		}
		else if (strcmp(str, "NORMAL") == 0)
		{
			return AttributeSemanticName::NORMAL;
		}
		else if (strcmp(str, "TANGENT") == 0)
		{
			return AttributeSemanticName::TANGENT;
		}
		else if (strcmp(str, "COLOR") == 0)
		{
			return AttributeSemanticName::COLOR;
		}
		else
		{
			return AttributeSemanticName::UNKNOWN;
		}
	}
	inline const char* getStringFromAttributeSemanticName(AttributeSemanticName type)
	{
		
		switch (type)
		{
		case YAPT::AttributeSemanticName::POSITION:
			return "POSITION";
		case YAPT::AttributeSemanticName::TEXCOORD:
			return "TEXCOORD";
		case YAPT::AttributeSemanticName::NORMAL:
			return "NORMAL";
		case YAPT::AttributeSemanticName::TANGENT:
			return "TANGENT";
		case YAPT::AttributeSemanticName::COLOR:
			return "COLOR";
		default:
			return nullptr;
		}
	}


	inline ShaderStages shaderModuleTypeToShaderStagesFlag(ShaderModuleType type)
	{
		switch (type)
		{
		case YAPT::ShaderModuleType::VERTEX_MODULE:
			return SHADERSTAGE_VERTEX;
		case YAPT::ShaderModuleType::HULL_MODULE:
			return SHADERSTAGE_HULL;
		case YAPT::ShaderModuleType::DOMAIN_MODULE:
			return SHADERSTAGE_DOMAIN;
		case YAPT::ShaderModuleType::GEOMETRY_MODULE:
			return SHADERSTAGE_GEOMETRY;
		case YAPT::ShaderModuleType::FRAGMENT_MODULE:
			return SHADERSTAGE_FRAGMENT;
		case YAPT::ShaderModuleType::COMPUTE_MODULE:
			return SHADERSTAGE_COMPUTE;
		case YAPT::ShaderModuleType::LIBRARY_MODULE:
			return SHADERSTAGE_RT_RAYGENERATION | SHADERSTAGE_RT_MISS | SHADERSTAGE_RT_ANY_HIT | SHADERSTAGE_RT_CLOSEST_HIT; //for now just assume library is always RT shader and add all potential stages.
		case YAPT::ShaderModuleType::LAST:
		default:
			return SHADERSTAGE_NONE;
		}
	}

	inline ShaderStageBits shaderModuleTypeToShaderStageBit(ShaderModuleType type)
	{
		switch (type)
		{
		case YAPT::ShaderModuleType::VERTEX_MODULE:
			return SHADERSTAGE_VERTEX;
		case YAPT::ShaderModuleType::HULL_MODULE:
			return SHADERSTAGE_HULL;
		case YAPT::ShaderModuleType::DOMAIN_MODULE:
			return SHADERSTAGE_DOMAIN;
		case YAPT::ShaderModuleType::GEOMETRY_MODULE:
			return SHADERSTAGE_GEOMETRY;
		case YAPT::ShaderModuleType::FRAGMENT_MODULE:
			return SHADERSTAGE_FRAGMENT;
		case YAPT::ShaderModuleType::COMPUTE_MODULE:
			return SHADERSTAGE_COMPUTE;
		case YAPT::ShaderModuleType::LIBRARY_MODULE:
		case YAPT::ShaderModuleType::LAST:
		default:
			return SHADERSTAGE_NONE;
		}
	}
}

#endif