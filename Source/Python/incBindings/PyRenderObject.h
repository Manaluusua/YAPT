#pragma once
#include <PyBindingsCommon.h>
#include <Scene/Scene.h>
#include <Scene/RenderableObject.h>
#include <PySceneObject.h>

#include <PyMesh.h>
#include <PyMaterial.h>
#include <memory>
#include <vector>

namespace YAPT
{
	class PyMesh;
	class PyMaterial;
	class PyRenderObject : public PySceneObject
	{
	public:
		DECLARE_BINDING_CLASS(PyRenderObject);

		PyRenderObject(const char* name, RenderableObject* obj);
		~PyRenderObject();

		void setMesh(std::shared_ptr<PyMesh> mesh);
		void setMaterial(std::shared_ptr<PyMaterial> material, size_t materialIndex);
		void setMaterials(std::vector<std::shared_ptr<PyMaterial>> materials);

		std::vector<std::shared_ptr<PyMaterial>> getMaterials();
		std::shared_ptr<PyMesh> getMesh();
		RenderableObject* getRenderable() { return static_cast<RenderableObject*>(getWrappedObject()); }

	private:
		std::shared_ptr<PyMesh> m_mesh;
		std::vector<std::shared_ptr<PyMaterial>> m_materials;
	};
}