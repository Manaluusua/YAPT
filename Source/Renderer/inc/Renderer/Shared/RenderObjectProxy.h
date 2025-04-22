#pragma once

#include <Renderer/RenderObject.h>
#include <Renderer/Shared/RenderObjectManager.h>
#include <Gfx/GfxBasicTypes.h>

namespace YAPT
{
	class RenderObjectProxy : public RenderObject
	{
		friend class RenderObjectManager;
	public:

		enum RenderObjectState
		{
			RENDEROBJECTSTATE_NOCHANGES = 0,
			RENDEROBJECTSTATE_CREATED = YAPTBIT(1),
			RENDEROBJECTSTATE_MODIFIED = YAPTBIT(2),
			RENDEROBJECTSTATE_DESTROYED = YAPTBIT(3),

			RENDEROBJECTSTATE_MATERIAL_CHANGED = YAPTBIT(29),
			RENDEROBJECTSTATE_MESH_CHANGED = YAPTBIT(30),
			RENDEROBJECTSTATE_TRANSFORM_CHANGED = YAPTBIT(31)
		};

		RenderObjectProxy(RenderObjectManager* mngr);
		virtual ~RenderObjectProxy();

		virtual void setMesh(Mesh* mesh) final;
		virtual void setTransform(const mat4& transform) final;

		virtual void setMaterials(RCObjectPtr<Material>* materials, size_t materialCount) final;


	protected:
		virtual void allReferencesReleased() final;

	private:

		RCObjectPtr<Mesh> m_mesh;
		std::vector<RCObjectPtr<Material>> m_materials;
		mat4 m_transform;

		RenderObjectManager* m_mngr;

		size_t _renderObjectState;
		RenderObjectId _id;
	};
}
