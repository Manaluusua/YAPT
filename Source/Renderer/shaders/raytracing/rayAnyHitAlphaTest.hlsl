#include "BPTShared.hlsl"
#include "hitShadersCommon.hlsl"
#include "payload.hlsl"

//alpha tested materials: their instances are non opaque, ignore the hits that fail the test. Kept in its own library,
//the pipeline reflection of a library module only covers one of its functions
[shader("anyhit")]
void rayAnyHitAlphaTest(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    if (!alphaTestPasses(InstanceID(), PrimitiveIndex(), attr.barycentrics))
    {
        IgnoreHit();
    }
}
