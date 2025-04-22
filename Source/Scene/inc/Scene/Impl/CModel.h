#pragma once

#include <Scene/Model.h>
#include <Common/RCObjectPtr.h>
#include <Renderer/Material.h>
#include <Renderer/Mesh.h>
#include <Scene/Transform.h>
#include <vector>

namespace YAPT
{
	class CScene;
	class Renderer;
	class RenderObject;
	class CModel : public Model
	{
	public:

		enum CModelDirtyFlag : uint32_t
		{
			CModelDirtyFlag_Transform = YAPTBIT(0),
			CModelDirtyFlag_Mesh = YAPTBIT(1),
			CModelDirtyFlag_Material = YAPTBIT(2)
		};

		CModel(Renderer* renderer, CScene* scene);
		~CModel();

		virtual void setMesh(Mesh* mesh) final;
		virtual void setMaterials(Material** material, size_t materialCount) final;
		virtual void setMaterial(Material* material, size_t materialIndex) final;

		virtual const Transform& getTransform() const final { return m_transform; }
		virtual Transform& getTransform() final { return m_transform; }

		bool isDirty() const { return m_dirtyMask != 0; }

		void refresh();

	private:

		void setDirty(uint32_t reason);
		static void transformChanged(void* ptr);
		
		Renderer* m_renderer;
		CScene* m_scene;
		RCObjectPtr<RenderObject> m_rendererObject;
		RCObjectPtr<Mesh> m_mesh;
		std::vector<RCObjectPtr<Material>> m_materials;

		Transform m_transform;
		uint32_t m_dirtyMask;
	};
}