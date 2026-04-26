#pragma once

#include <Scene/RenderableObject.h>
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
	class CRenderableObject : public RenderableObject
	{
	public:

		enum CRenderableObjectDirtyFlag : uint32_t
		{
			CRenderableObjectDirtyFlag_Transform = YAPTBIT(0),
			CRenderableObjectDirtyFlag_Mesh = YAPTBIT(1),
			CRenderableObjectDirtyFlag_Material = YAPTBIT(2)
		};

		CRenderableObject(Renderer* renderer, CScene* scene);
		~CRenderableObject();

		virtual void setMesh(Mesh* mesh) final;
		virtual void setMaterials(Material** material, size_t materialCount) final;
		virtual void setMaterial(Material* material, size_t materialIndex) final;

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
		uint32_t m_dirtyMask;
	};
}