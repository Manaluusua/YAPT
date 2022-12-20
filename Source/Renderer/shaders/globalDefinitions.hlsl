
#ifndef INCL_GLOBALDEFINITIONS_HLSL
#define INCL_GLOBALDEFINITIONS_HLSL

#ifdef DX12

#define SHADERTABLE_EXTRADATA_DECLARE(DataType) ConstantBuffer<DataType> ___ShaderTableExtraData___ : register(b0, space0)
#define SHADERTABLE_EXTRADATA (___ShaderTableExtraData___)

#elif VK


#endif

#endif