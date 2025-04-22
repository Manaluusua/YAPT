#include <Renderer/Shared/RenderObjectProxy.h>
#include <Renderer/Material.h>
#include <Renderer/Mesh.h>


namespace YAPT
{
	RenderObjectProxy::RenderObjectProxy(RenderObjectManager* mngr)
		:_id(InvalidRenderObjectId),
		m_mngr(mngr)

	{

	}
	RenderObjectProxy::~RenderObjectProxy()
	{

	}

	void RenderObjectProxy::setMesh(Mesh* mesh)
	{
		m_mesh = mesh;
		m_mngr->renderObjectChanged(this);
		_renderObjectState |= RENDEROBJECTSTATE_MESH_CHANGED;
	}

	void RenderObjectProxy::setMaterials(RCObjectPtr<Material>* materials, size_t materialCount)
	{
		m_materials.resize(materialCount);

		for (size_t i = 0; i < materialCount; ++i)
		{
			m_materials[i] = materials[i];
		}
		m_mngr->renderObjectChanged(this);
		_renderObjectState |= RENDEROBJECTSTATE_MATERIAL_CHANGED;
	}

	void RenderObjectProxy::setTransform(const mat4& transform)
	{
		m_transform = transform;
		m_mngr->renderObjectChanged(this);
		_renderObjectState |= RENDEROBJECTSTATE_TRANSFORM_CHANGED;
	}


	void RenderObjectProxy::allReferencesReleased()
	{
		m_mngr->renderObjectReleased(this);
	}
}