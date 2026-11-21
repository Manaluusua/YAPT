#ifndef SCENE_INSTANCE_DATA_HLSL_INCL
#define SCENE_INSTANCE_DATA_HLSL_INCL

#include "../materials/materialsCommonResources.hlsl"

//keep in sync with SceneInstanceData
StructuredBuffer<RenderObjectTransformDataGPU> g_renderObjectTransforms : register(t3, space0);
ByteAddressBuffer g_renderObjectMatAndMeshIndices : register(t4, space0);

uint2 getMaterialAndMeshIndicesForInstance(uint instanceIndex)
{
    uint readOffset = instanceIndex << 3;
    uint2 matMeshIndices = g_renderObjectMatAndMeshIndices.Load2(readOffset);
    return matMeshIndices;
}

RenderObjectTransformDataGPU getTransformDataForInstance(uint instanceIndex)
{
    return g_renderObjectTransforms[instanceIndex];
}

#endif
