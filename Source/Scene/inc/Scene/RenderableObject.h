#pragma once

#include "SceneObject.h"
namespace YAPT
{
	class Mesh;
	class Material;

	class RenderableObject : public SceneObject
	{
	public:
		virtual void setMesh(Mesh* mesh) = 0;
		virtual void setMaterials(Material** material, size_t materialCount) = 0;
		virtual void setMaterial(Material* material, size_t materialIndex) = 0;
		virtual ~RenderableObject(){}
	};
}