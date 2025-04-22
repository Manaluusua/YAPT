#pragma once

#include <Common/RCObject.h>
#include <Scene/Transform.h>
namespace YAPT
{
	class Mesh;
	class Material;

	class Model : public RCObject
	{
	public:
		virtual void setMesh(Mesh* mesh) = 0;
		virtual void setMaterials(Material** material, size_t materialCount) = 0;
		virtual void setMaterial(Material* material, size_t materialIndex) = 0;

		virtual const Transform& getTransform() const = 0;
		virtual Transform& getTransform() = 0;
	};
}