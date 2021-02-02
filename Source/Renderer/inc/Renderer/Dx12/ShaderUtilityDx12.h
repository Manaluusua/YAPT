#pragma once

#include <Renderer/Dx12/Dx12CommonIncludes.h>
#include <Renderer/Shared/GfxTypes.h>

#include <dxcapi.h>
#include <string>
#include <Renderer/Dx12/Dx12CommonIncludes.h>
namespace YAPT
{
	LPCWSTR shaderTypeToProfilePrefix(ShaderModuleType shdType);
	void shaderTypeToProfile(ShaderModuleType shdType, std::wstring& profileOut);

	RCPtr<IDxcBlob> compileFromFile(LPCWSTR profile, LPCWSTR dbgName, LPCWSTR entryPoint, LPCWSTR filePath, const DxcDefine* defines = nullptr, UINT32 defineCount = 0);
	void readReflectionData(IDxcBlob* blob, const char* entryPoint, ShaderReflectionDataDx12& reflOut);



}
