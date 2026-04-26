#include <Scene/Impl/CRenderableObject.h>
#include <Scene/Impl/CScene.h>
#include <Renderer/Renderer.h>

namespace YAPT
{
	

	CRenderableObject::CRenderableObject(Renderer* renderer, CScene* scene)
		:m_scene(scene),
		m_renderer(renderer),
		m_rendererObject(nullptr),
		m_dirtyMask(0)
	{
		getTransform().setDirtyCallback(CRenderableObject::transformChanged, this);
	}
	CRenderableObject::~CRenderableObject()
	{

	}

	void CRenderableObject::setMesh(Mesh* mesh)
	{
		m_mesh = mesh;
		setDirty(CRenderableObjectDirtyFlag_Mesh);
	}

	void CRenderableObject::setMaterials(Material** material, size_t materialCount)
	{
		m_materials.resize(materialCount);

		for (size_t i = 0; i < materialCount; ++i)
		{
			m_materials[i] = material[i];
		}

		setDirty(CRenderableObjectDirtyFlag_Material);
	}
	void CRenderableObject::setMaterial(Material* material, size_t materialIndex)
	{
		if (m_materials.size() <= materialIndex)
		{
			m_materials.resize(materialIndex + 1, nullptr);
		}

		m_materials[materialIndex] = material;
	}


	void CRenderableObject::setDirty(uint32_t reason)
	{
		if (m_dirtyMask == 0)
		{
			m_scene->needsRefresh(this);
		}
		m_dirtyMask |= reason;
	}

	void CRenderableObject::transformChanged(void* ptr)
	{
		static_cast<CRenderableObject*>(ptr)->setDirty(CRenderableObjectDirtyFlag_Transform);
	}

	void CRenderableObject::refresh()
	{
		if ((CRenderableObjectDirtyFlag_Mesh | CRenderableObjectDirtyFlag_Material) & m_dirtyMask)
		{
			if (m_mesh.get() && m_materials.size() > 0)
			{
				if (!m_rendererObject)
				{
					m_rendererObject = m_renderer->createRenderObject();
					m_rendererObject->setMesh(m_mesh.get());
					m_rendererObject->setMaterials(m_materials.data(), m_materials.size());
					m_rendererObject->setTransform(getTransform().getMatrixWS());
					m_rendererObject->Release();
				}
				else
				{
					if (CRenderableObjectDirtyFlag_Mesh & m_dirtyMask)
					{
						m_rendererObject->setMesh(m_mesh.get());
					}
					if (CRenderableObjectDirtyFlag_Material & m_dirtyMask)
					{
						m_rendererObject->setMaterials(m_materials.data(), m_materials.size());
					}
				}
			}
			else
			{
				m_rendererObject = nullptr;
			}
		}

		if (CRenderableObjectDirtyFlag_Transform & m_dirtyMask)
		{
			if (m_rendererObject)
			{
				m_rendererObject->setTransform(getTransform().getMatrixWS());
			}
		}


		m_dirtyMask = 0;
	}
}