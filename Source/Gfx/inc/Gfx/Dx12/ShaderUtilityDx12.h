#pragma once

#include <Gfx/Dx12/Dx12CommonIncludes.h>
#include <Gfx/GfxTypes.h>

#include <dxcapi.h>
#include <string>
#include <Gfx/Dx12/Dx12CommonIncludes.h>
namespace YAPT
{

	void readReflectionData(IDxcBlob* blob, const char* entryPoint, ShaderReflectionDataDx12& reflOut);



}
