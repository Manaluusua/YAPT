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

	
}