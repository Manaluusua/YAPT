#pragma once
#include <PyBindingsCommon.h>
#include <Renderer/Material.h>
#include <Scene/Scene.h>
#include <Scene/Model.h>
#include <PyTransform.h>
#include <memory>

namespace YAPT
{
	class PyMesh;
	class PyMaterial;
	class PyRenderObject
	{
	public:
		DECLARE_BINDING_CLASS(PyRenderObject);

		PyRenderObject(const char* name, Scene* scene);
		~PyRenderObject();

		void setMesh(PyMesh* mesh);
		void setMaterial(PyMaterial* material, size_t materialIndex);
		void setMaterials(std::vector<PyMaterial*> materials);
		PyTransform* getTransform();

		const char* getName() const;
		Model* getModel() { return m_obj; }

	private:
		std::string m_name;
		RCObjectPtr<Model> m_obj;
		PyTransform m_transform;
	};
}