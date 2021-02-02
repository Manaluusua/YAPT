#ifndef YAPT_RENDEROBJECT_H
#define YAPT_RENDEROBJECT_H
#include <Common/RCObject.h>
#include <Common/RCObjectPtr.h>
#include <Math/Math.h>
namespace YAPT
{
	class Mesh;
	class Material;

	class RenderObject : public RCObject
	{
	public:
		virtual void setMesh(Mesh* mesh) = 0;
		virtual void setMaterial(Material* material) = 0;
		virtual void setTransform(const mat4& transform) = 0;

	};
}

#endif