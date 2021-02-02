#ifndef YAPT_DX12_DX12COMMONINCLUDES_H
#define YAPT_DX12_DX12COMMONINCLUDES_H

#include <d3d12.h>
#include <cassert>
#include <Common/RCObjectPtr.h>
#include <Common/Logger.h>


#if defined(DEBUG) || defined(_DEBUG)
#define ENABLE_DEBUG_UTILITIES_DX12
#endif




inline void checkHResult(HRESULT res)
{
	if (FAILED(res))																			
	{ 																						
		YAPT_LOG_FATAL_ERROR("DX12 Error (%d)", res);	
	}
}

#define checkForDxError(res) (checkHResult(res))

typedef ID3D12GraphicsCommandList4 GraphicsCommandListDx12;

#endif