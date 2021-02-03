#pragma once

#include <Renderer/Shared/GfxTypes.h>
#include <Common/RCObjectPtr.h>
#include <unknwn.h>
#include <dxcapi.h>

namespace YAPT
{
	inline LPCWSTR shaderTypeToProfilePrefix(ShaderModuleType shdType)
	{
		switch (shdType)
		{
		case ShaderModuleType::VERTEX_MODULE:
			return L"vs_";
		case ShaderModuleType::FRAGMENT_MODULE:
			return L"ps_";
		case ShaderModuleType::HULL_MODULE:
			return L"hs_";
		case ShaderModuleType::DOMAIN_MODULE:
			return L"ds_";
		case ShaderModuleType::GEOMETRY_MODULE:
			return L"gs_";
		case ShaderModuleType::COMPUTE_MODULE:
			return L"cs_";
		case ShaderModuleType::LIBRARY_MODULE:
			return L"lib_";
		default:
			return L"lib_";
		}
	}

	inline void shaderTypeToProfile(ShaderModuleType shdType, std::wstring& profileOut)
	{
		profileOut = shaderTypeToProfilePrefix(shdType);
		profileOut += L"6_3";
	}

	RCPtr<IDxcBlob> compileFromFile(LPCWSTR profile, LPCWSTR dbgName, LPCWSTR entryPoint, LPCWSTR filePath, const DxcDefine* defines, UINT32 defineCount,const LPCWSTR* extraParameters = nullptr, size_t extraParamsCount = 0);
}