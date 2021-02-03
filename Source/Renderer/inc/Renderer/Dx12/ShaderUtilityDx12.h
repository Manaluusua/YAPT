#pragma once

#include <Renderer/Dx12/Dx12CommonIncludes.h>
#include <Renderer/Shared/GfxTypes.h>

#include <dxcapi.h>
#include <string>
#include <Renderer/Dx12/Dx12CommonIncludes.h>
namespace YAPT
{

	void readReflectionData(IDxcBlob* blob, const char* entryPoint, ShaderReflectionDataDx12& reflOut);



}
