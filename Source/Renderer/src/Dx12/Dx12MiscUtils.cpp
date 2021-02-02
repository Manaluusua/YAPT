#include <Renderer/Dx12/Dx12MiscUtils.h>

namespace YAPT
{

	bool d3d12StatePromotableFromCommon(D3D12_RESOURCE_STATES firstStateInExecute, bool isBufferOrSimultanousAccessTexture)
	{
		if ((firstStateInExecute & (D3D12_RESOURCE_STATE_DEPTH_WRITE | D3D12_RESOURCE_STATE_DEPTH_READ))) return false;
		if (isBufferOrSimultanousAccessTexture) return true;

		constexpr D3D12_RESOURCE_STATES nonPromotableStates = 
			~(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
			D3D12_RESOURCE_STATE_COPY_DEST | D3D12_RESOURCE_STATE_COPY_SOURCE);

		return  (firstStateInExecute & nonPromotableStates) != 0;
	}

	void stringToWString(const std::string& str, std::wstring& out)
	{
		int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), NULL, 0);
		if (size == 0) return;
		out.resize(size);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &out[0], size);
	}

	void stringToWString(const char* str, std::wstring& out)
	{
		size_t len = strlen(str);
		int size = MultiByteToWideChar(CP_UTF8, 0, str, (int)len, NULL, 0);
		if (size == 0) return;
		out.resize(size);
		MultiByteToWideChar(CP_UTF8, 0, str, (int)len, &out[0], size);
	}
}